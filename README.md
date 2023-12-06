# TranscriberGlasses 
### EECS 473 - Group 14
#### Daniel Calco (dcalco), Jorge Garcia (jorgegar), Tom Jiao (tomjiao), Daniel Kennedy (danken) & Tiancheng Zhang(zhangtc)

![Placeholder...](https://raw.githubusercontent.com/tom21487/TranscriberGlasses/master/Images%20%26%20Demos/Menu-Monitor.gif?token=GHSAT0AAAAAACKQMVRM2MLXI27UBI3MBR6GZLHCVKQ)

Created for the University of Michigan's EECS 473 (Advanced Embedded Systems) final project. You can view a working demo, read some background information, and view our poster at [our carrd.co website](https://transcriberglasses.carrd.co).

In this repo, you'll find individual tests for hardware parts, our PCB & schematic, the FPGA code for 720p HDMI output, and the code for the ESP32.

## Guide for compilation & flashing on ESP32-WROOM-DA modules:
 1. Clone this repo
 2. On the Arduino Board Manager, install` ESP32` from `expressif systems` from the board manager ![](https://raw.githubusercontent.com/tom21487/TranscriberGlasses/master/Images%20%26%20Demos/Workspace%20Instructions/board.png?token=GHSAT0AAAAAACKQMVRNY4SYNLNS4HZRJAL2ZLHCVZQ)
 3. On the Arduino Library Manager, install `arduinojson` ![](https://raw.githubusercontent.com/tom21487/TranscriberGlasses/master/Images%20%26%20Demos/Workspace%20Instructions/arduinojson.png?token=GHSAT0AAAAAACKQMVRMYBUTO6C3AHBEWVDOZLHCWHQ)
 4. Set board to `ESP32-WROOM-DA` module ![](https://raw.githubusercontent.com/tom21487/TranscriberGlasses/master/Images%20%26%20Demos/Workspace%20Instructions/board-select.png?token=GHSAT0AAAAAACKQMVRMWN6B7DUIHOBMKKQCZLHCWVQ)
 6. Compile & Upload!
 
## Acknowledgements
- We would like to thank Daniel Hepper for making his 8 by 8 bitmap fonts available in the public domain. His work is here: https://github.com/dhepper/font8x8
- We would like to thank Clifford Wolfe for making his FPGA HDMI Verilog code available under the ISC license. We used his code for the TMDS encoding needed in generating the HDMI signals. His work is here: https://github.com/cliffordwolf/SimpleVOut
- We would like to thank Benoît Blanchon for providing his ArduinoJSON library, which is under the MIT license. We used the library’s JSON decoder to parse our API results. His work is here: https://arduinojson.org/
