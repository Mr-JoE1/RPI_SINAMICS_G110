## - RPI_SINAMICS_G110
C++ library communication to Siemens SINAMICS G110 inverter over USS protocol (RS-485) over UART GPIO or USB/RS485 modules 
It supports all Raspberry Pi boards , Only test on Raspberry PI 3B+ board
SINAMICS V20 can be also compatible but its not tested.

You need a level shifter with MAX485/MAX484 chip for RS-485 voltage levels and must wire it to 3.3/GND and  TX/RX GPIO and a pin you can choose 
for switching between sending and receiving on MAX485/max484 Or use RS485/USB modules for more reliablity.

- This library wrapped from G110 ARDUINO library by **Merlin Kr�mmel** , [Original repo](https://github.com/zocker007/G110-USS-Arduino)

### - New Features for Raspberry Pi 
 - implemeted Pigpio C library for frindly using 
 - Suppot RS485 over usb 
 - Error logging framework comming soon
 - Using linux timers for timing and delays 

### - Dependecies :
- Make sure raspbery pi pigpio c library is installed on your pi before using this library.
- **to install pigio library :**
```
wget https://github.com/joan2937/pigpio/archive/master.zip
unzip master.zip
cd pigpio-master
make
sudo make install
```  
### - Usage :
 - Just clone the repo into your project directory.
 - Or copy the librart files to your **env $PATH**
 
 ### - Hints:
 - Check examples folder for library usaing 
 - refere to SINAMICS G110 Manules for better understanding of different commitiing modes and USS communications.
 - feel free to throw your thoughts in disscution section
 
 ### - if you found any bugs, Contact me  [Linkedin](https://www.linkedin.com/in/mohamed-maher-37b167a5/).
