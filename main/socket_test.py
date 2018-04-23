import socket               
import time 
import struct

sock = socket.socket()

host = "192.168.1.193" #ESP32 IP in local network
port = 8001            #ESP32 Server Port    
 
sock.connect((host, port))
sock.send(bytes("{}".format("setpoint:24,P:28,I:1,D:18"), 'utf-8'))
i = 1
while True:
    data = sock.recv(256).decode()
    if data[0] == "B":
        continue
    print("-" * 80)
    print(data, flush=True)
    print("Length is {}".format(len(data)))
    if i % 10 == 0:
        sock.send(bytes("setpoint:{},P:28,I:1,D:18".format(i), 'utf-8'))
        print("Sent another message!", flush=True)
    # time.sleep(1)
    i += 1

print("done")
sock.close()
 
# for i in range(1):
#     sock.send(bytes("{}".format("setpoint:24,P:28,I:1,D:18"), 'utf-8'))
   

#     time.sleep(2)

# print("done")
# sock.close()