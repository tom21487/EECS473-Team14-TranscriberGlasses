//Copyright (C)2014-2020 Gowin Semiconductor Corporation.
//All rights reserved.
//File Title: Template file for instantiation
//GOWIN Version: V1.9.6Alpha
//Part Number: GW1NSR-LV4CQN48PC6/I5
//Created Time: Sun Oct 22 19:06:41 2023

//Change the instance name and port connections to the signal names
//--------Copy here to design--------

    Gowin_PLLVR your_instance_name(
        .clkout(clkout_o), //output clkout
        .lock(lock_o), //output lock
        .clkin(clkin_i) //input clkin
    );

//--------Copy end-------------------
