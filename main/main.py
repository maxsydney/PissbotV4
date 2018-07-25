import sys
import json
import time
import math
import dateutil
import requests
import socket
from PyQt4 import QtGui, QtCore
import matplotlib.pyplot as plt
from matplotlib import style
import matplotlib.animation as animation
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QT as NavigationToolbar
style.use('ggplot')

class MyPopup(QtGui.QWidget):
    def __init__(self, socket, setpoint, P_gain, I_gain, D_gain):
        QtGui.QWidget.__init__(self)
        self.socket = socket
        self.setpoint = setpoint
        self.P_gain = P_gain
        self.I_gain = I_gain
        self.D_gain = D_gain
        self.draw_ui()
        
    def draw_ui(self):
        grid = QtGui.QGridLayout()

        btn = QtGui.QPushButton("Set gains", self)
        btn.resize(btn.minimumSizeHint())

        P_gain_label = QtGui.QLabel()
        P_gain_label.setText("P Gain: {:.2f}".format(self.P_gain))
        P_gain = QtGui.QLineEdit()
        P_gain.setFixedWidth(btn.width())

        I_gain_label = QtGui.QLabel()
        I_gain_label.setText("I Gain: {:.2f}".format(self.I_gain))
        I_gain = QtGui.QLineEdit()
        I_gain.setFixedWidth(btn.width())

        D_gain_label = QtGui.QLabel()
        D_gain_label.setText("D Gain: {:.2f}".format(self.D_gain))
        D_gain = QtGui.QLineEdit()
        D_gain.setFixedWidth(btn.width())

        btn.clicked.connect(lambda: self.update_gains(P_gain.text(), I_gain.text(), D_gain.text()))

        grid.addWidget(P_gain_label, 0, 0, 2, 1)
        grid.addWidget(P_gain, 0, 1, 2, 1)
        grid.addWidget(I_gain_label, 1, 0, 2, 1)
        grid.addWidget(I_gain, 1, 1, 2, 1)
        grid.addWidget(D_gain_label, 2, 0, 2, 1)
        grid.addWidget(D_gain, 2, 1, 2, 1)
        grid.addWidget(btn, 2, 3, 2, 1)
        
        self.setLayout(grid)
    
    def update_gains(self, P_gain, I_gain, D_gain):
        if P_gain == "":
            P_gain = self.P_gain
        else:
            P_gain = float(P_gain)
        if I_gain == "":
            I_gain = self.I_gain
        else:
            I_gain = float(I_gain)
        if D_gain == "":
            D_gain = self.D_gain
        else:
            D_gain = float(D_gain)

        self.socket.send(bytes("INFO&setpoint:{},P:{},I:{},D:{}".format(self.setpoint, P_gain, I_gain, D_gain), 'utf-8'))
        self.close()

        

class Window(QtGui.QWidget):

    def __init__(self):
        super(Window, self).__init__()
        self.time = []
        self.recorded_temp = []
        self.recorded_setpoint = []
        self.runtime = 0
        self.element_status = ""
        self.temp = 0
        self.setpoint = 0
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(1.5)
        self.setWindowTitle("Distillation Control Panel")
        self.stacked_layout = QtGui.QStackedLayout()

        self.connect_page = QtGui.QWidget()
        self.home_page = QtGui.QWidget()
        self.home_UI()
        self.connect_UI()
        self.stacked_layout.addWidget(self.connect_page)
        self.stacked_layout.addWidget(self.home_page)
        self.setLayout(self.stacked_layout)

        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update_gui)
        self.show()

    def main(self):
        self.timer.start(200)
        self.ani = animation.FuncAnimation(self.fig, self.animate, interval=50)
        
    def home_UI(self):
        grid = QtGui.QGridLayout()
        runtime_indicator_label = QtGui.QLabel()
        self.runtime = QtGui.QLabel()
        runtime_indicator_label.setText("Runtime: ")
        element_status_indicator = QtGui.QLabel()
        self.element_status = QtGui.QLabel()
        element_status_indicator.setText("Element Status: ")
        temp_indicator_label = QtGui.QLabel()
        self.temp = QtGui.QLabel()
        temp_indicator_label.setText("Temperature: ")
        setpoint_label = QtGui.QLabel()
        self.setpoint = QtGui.QLabel()
        setpoint_label.setText("Setpoint: ")

        btn = QtGui.QPushButton("Adjust", self)
        btn.resize(btn.minimumSizeHint())
        self.setpoint_value = QtGui.QLineEdit()
        self.setpoint_value.setFixedWidth(btn.width())
        btn.clicked.connect(lambda: self.adjust_setpoint(self.setpoint_value.text()))

        btn2 = QtGui.QPushButton("Tune PID", self)
        btn2.resize(btn.minimumSizeHint())
        btn2.clicked.connect(lambda: self.tune_PID())

        self.fig = plt.figure(figsize=(15, 5))
        self.ax = self.fig.add_subplot(111)
        self.canvas = FigureCanvas(self.fig)
        self.toolbar = NavigationToolbar(self.canvas, self)
        
        grid.addWidget(runtime_indicator_label, 0, 0, 2, 1)
        grid.addWidget(self.runtime, 0, 1, 2, 1)
        grid.addWidget(element_status_indicator, 0, 2, 2, 1)
        grid.addWidget(self.element_status, 0, 3, 2, 1)
        grid.addWidget(temp_indicator_label, 0, 4, 2, 1)
        grid.addWidget(self.temp, 0, 5, 2, 1)
        grid.addWidget(setpoint_label, 0, 6, 2, 1)
        grid.addWidget(self.setpoint, 0, 7, 2, 1)
        grid.addWidget(self.setpoint_value, 0, 8, 1, 1)
        grid.addWidget(btn, 1, 8, 1, 1)
        grid.addWidget(btn2, 2, 8, 1, 1)
        grid.addWidget(self.canvas, 3, 0, 1, 9)
        grid.addWidget(self.toolbar, 4, 0, 1, 9)
        self.home_page.setLayout(grid)
        self.setFocus()

    def connect_UI(self):
        '''
        User interface for connecting to distillation controller server. Enter local IP address for ESP8266 
        assigned by router
        '''
        grid = QtGui.QGridLayout()
        addr = QtGui.QLineEdit()
        addr.setPlaceholderText("Enter IP address for distiller server")
        grid.addWidget(addr, 1, 1)

        btn = QtGui.QPushButton("Connect!", self)
        btn.clicked.connect(lambda: self.connect(addr.text()))
        grid.addWidget(btn, 1, 2)
        self.connect_page.setLayout(grid)
        self.setFocus()

    def connect(self, addr):
        '''
        Check connection with controller server and verify address
        '''
        try:
            self.sock.connect(("192.168.1.134" , 8001))
            self.sock.send(bytes("CONN", 'utf-8'))
            QtGui.QMessageBox.information(self, 'test', "Connected!", QtGui.QMessageBox.Ok)
            self.address = addr
            self.stacked_layout.setCurrentIndex(1)
             
        except Exception:
            QtGui.QMessageBox.information(self, 'test', "Device not found at {}. Please enter a valid address".format(addr), QtGui.QMessageBox.Ok)
            self.connect_UI()
            return
        
        self.main()

    def update_gui(self):
        '''
        This function needs to be made much more robust to work correctly. Implement proper 
        incoming message buffer and read messages correctly
        '''
        try:
            for line in readlines(self.sock):
                if line == "BEAT":
                    continue
                print(line, flush=True)

                temp, setpoint, runtime, element_status, P_gain, I_gain, D_gain = line.split(",")

                self.P_gain = float(P_gain)
                self.I_gain = float(I_gain)
                self.D_gain = float(D_gain)
            
                # runtime = convertMillis(float(runtime))
                self.runtime.setText(runtime)
                self.element_status.setText(element_status)
                self.temp.setText(str(temp))
                self.setpoint.setText(str(setpoint))

                # self.time.append(dateutil.parser.parse(runtime))
                self.time.append(float(runtime))
                self.recorded_temp.append((float(temp)))
                self.recorded_setpoint.append(float(setpoint))
        except Exception:
            print("Socket receive timed out", flush=True)

    def adjust_setpoint(self, value):
        if value != "":
            self.sock.send(bytes("INFO&setpoint:{},P:{},I:{},D:{}".format(value, self.P_gain, self.I_gain, self.D_gain), 'utf-8'))
            self.setpoint_value.setText("")

    def animate(self, i):
        plt.cla()
        self.ax.plot(self.time, self.recorded_temp, label="Outflow Temperature")
        self.ax.plot(self.time, self.recorded_setpoint, label="Controller Setpoint")
        self.ax.set_xlabel("Running Time", fontsize=8)
        self.ax.set_ylabel("Temperature (°C)", fontsize=8)
        plt.xticks(fontsize=8, rotation=45)
        plt.yticks(fontsize=8)
        plt.tight_layout()
        plt.legend(loc="upper left")
        self.canvas.draw()

    def tune_PID(self):
        print("Opening a new popup window...")
        self.w = MyPopup(self.sock, self.recorded_setpoint[-1], self.P_gain, self.I_gain, self.D_gain)
        self.w.setGeometry(QtCore.QRect(100, 100, 400, 200))
        self.w.show()

def convertMillis(seconds):
    minutes, seconds = divmod(seconds, 60)
    hours, minutes = divmod(minutes, 60)
    return "{:0>2d}:{:0>2d}:{:0>2d}".format(int(hours), int(minutes), int(seconds))

def readlines(sock, recv_buffer=4096, delim='\n'):
    buffer = ''
    data = sock.recv(recv_buffer).decode()
    buffer += data
    while buffer.find(delim) != -1:
        line, buffer = buffer.split('\n', 1)
        yield line
    return

def main():
    app = QtGui.QApplication(sys.argv)
    GUI = Window()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()