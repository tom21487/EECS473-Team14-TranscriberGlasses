# TranscriberGlasses 
### EECS 473 - Group 14
#### Daniel Calco (dcalco), Jorge Garcia (jorgegar), Tom Jiao (tomjiao), Daniel Kennedy (danken) & Tiancheng Zhang(zhangtc)

![Menu Splash Screen](https://github.com/tom21487/EECS473-Team14-TranscriberGlasses/blob/master/Images%20&%20Demos/Menu-Monitor.gif?raw=true)

Created for the University of Michigan's EECS 473 (Advanced Embedded Systems) final project. You can view a working demo, read some background information, and view our poster at [our carrd.co website](https://transcriberglasses.carrd.co).

In this repo, you'll find individual tests for hardware parts, our PCB & schematic, the FPGA code for 720p HDMI output, and the code for the ESP32.

![Working demo!](https://github.com/tom21487/EECS473-Team14-TranscriberGlasses/blob/master/Images%20&%20Demos/Russian-Demo.gif?raw=true)

## Guide for compilation & flashing on ESP32-WROOM-DA modules:
 1. Clone this repo
 2. On the Arduino Board Manager, install` ESP32` from `expressif systems` from the board manager ![](https://github.com/tom21487/EECS473-Team14-TranscriberGlasses/blob/master/Images%20&%20Demos/Workspace%20Instructions/board.png?raw=true)
 3. On the Arduino Library Manager, install `arduinojson` ![](https://github.com/tom21487/EECS473-Team14-TranscriberGlasses/blob/master/Images%20&%20Demos/Workspace%20Instructions/arduinojson.png?raw=true)
 4. Set board to `ESP32-WROOM-DA` module ![](https://github.com/tom21487/EECS473-Team14-TranscriberGlasses/blob/master/Images%20&%20Demos/Workspace%20Instructions/board-select.png?raw=true)
 6. Compile & Upload!
 
## Acknowledgements
- We would like to thank Daniel Hepper for making his 8 by 8 bitmap fonts available in the public domain. His work is here: https://github.com/dhepper/font8x8
- We would like to thank Clifford Wolfe for making his FPGA HDMI Verilog code available under the ISC license. We used his code for the TMDS encoding needed in generating the HDMI signals. His work is here: https://github.com/cliffordwolf/SimpleVOut
- We would like to thank Benoît Blanchon for providing his ArduinoJSON library, which is under the MIT license. We used the library’s JSON decoder to parse our API results. His work is here: https://arduinojson.org/
