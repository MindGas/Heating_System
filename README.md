# XBee Heating System for Home Assistant
 ZigBee-Arduino heating system controls for Home Assistant with ZHA integration.
 
 This is a work in progress for home and hot water heating system. Two thermometers are used to check hot water boiler temperature at high and low points and send this information to Home Assistant over the ZigBee network. This temperature information then can be used in automations or creating new sensors. Another functionality is to turn on/off the electric water heater and central gas heating with two relays. Previously I have implemented this with different radios. Changing to ZigBee protocol to improve reliability and lessen maintenance by getting rid of another integration.
 
 Forked/reworked from [GitHub Gist prairiesnpr/water_heater.ino](https://gist.github.com/prairiesnpr/7a40b78e765044252a4799d328327f0a)
 
 ## Known Issues
 * Sending Endpoint temperature values fails with code 21 = Network ACK failure.
 * On/Off switch endpoint not tested, presuming not functioning.
 
 ## Hardware:
  Prototyping:
 * Digi XBee S2C (XB24C)
 * Waveshare XBee USB adapter
 * Arduino Uno R3
 * Arduino Nano
 * Tempereture Sensor DS18B20 x2
 * Resistor 4.7kΩ

 Prototyping / Debugging Setup:
 
![Protopyping setup](https://github.com/MindGas/Heating_System/blob/main/images/XBee_Heating_System_Prototyping.jpg?raw=true)
 
 ## XBee Parameters
 XBee needs to be configured before it can be used. Use Digi XCTU software to configure it. Not all parameters below are required and some could have other values, but this works for me. Parameters in bold are required with suggested values.
 * **ZS 2**
 * NJ 5A
 * NI heating
 * **EE 1**
 * **EO 1**
 * **KY 5a6967426565416c6c69616e63653039**
 * D7 0
 * **AP 2**
 * **AO 3**
 * P0 5
 * IR 1388
 * IC FFFF
 
 ## Screenshots
 Home Assistant with ZHA integration establishes a connection with XBee-Arduino device ower ZigBee protocol
![Home Assistant - ZHA](https://github.com/MindGas/Heating_System/blob/main/images/ZHA-ZigBee_device_added.jpg?raw=true)

Debugging output in Arduino Serial Monitor
![Serial Monitor](https://github.com/MindGas/Heating_System/blob/main/images/SerialMonitor-DebuggingInfo.jpg?raw=true)
 
 ## Good To Know
 * Disconnect TX/RX lines between XBee and Uno when uploading scetch
 * If using Arduino Serial Monitor don't forget to switch port to Nano port after uploading scetch to Uno.
 * Disconnect TX/RX lines between XBee and Uno when configuring XBee over XCTU
 
 ## Would Not Be Possible Without
 [GitHub Gist prairiesnpr/water_heater.ino](https://gist.github.com/prairiesnpr/7a40b78e765044252a4799d328327f0a)
 
 [Blog "Connecting an Xbee 24ZB to Smartthings Hub" by George](https://nzfalco.jimdofree.com/electronic-projects/xbee-to-smartthings/)
 
 [XBee-Arduino library by andrewrapp](https://github.com/andrewrapp/xbee-arduino)
  
 [Digi XBee/XBee-PRO® S2C Zigbee® RF Module Decommentation](https://www.digi.com/resources/documentation/Digidocs/90002002/Default.htm)
 
