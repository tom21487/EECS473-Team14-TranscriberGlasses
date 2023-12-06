//Copyright (C)2014-2020 Gowin Semiconductor Corporation.
//All rights reserved.
//File Title: Template file for instantiation
//GOWIN Version: V1.9.6Alpha
//Part Number: GW1NSR-LV4CQN48PC6/I5
//Created Time: Tue Oct 31 16:16:24 2023

//Change the instance name and port connections to the signal names
//--------Copy here to design--------

    Gowin_SDPB your_instance_name(
        .dout(dout_o), //output [4:0] dout
        .clka(clka_i), //input clka
        .cea(cea_i), //input cea
        .reseta(reseta_i), //input reseta
        .clkb(clkb_i), //input clkb
        .ceb(ceb_i), //input ceb
        .resetb(resetb_i), //input resetb
        .oce(oce_i), //input oce
        .ada(ada_i), //input [6:0] ada
        .din(din_i), //input [4:0] din
        .adb(adb_i) //input [6:0] adb
    );

//--------Copy end-------------------
