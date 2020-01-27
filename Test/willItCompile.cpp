#include <iostream>
#include "pump.h"
 
int main(int argc, char *argv[]){
    Pump p;
    p.setSpeed(500);
    std::cout << "Pump speed " << p.getSpeed() << std::endl;
    return 0;
}