module top(
  input clk,
  input resetn, // user_rst     | FPGA1 | gpio14
  input gpio43, // user_clk     | FPGA2
  input gpio20, // user_data[2] | FPGA7
  input gpio39, // user_data[1] | FPGA9
  input gpio40, // user_data[0] | FPGA10

  output       tmds_clk_n,
  output       tmds_clk_p,
  output [2:0] tmds_d_n,
  output [2:0] tmds_d_p
);

// We want a pixel clock of 74.68032 MHz
Gowin_CLKDIV u_div_5 (
    .clkout(clk_p),
    .hclkin(clk_p5),
    .resetn(pll_lock)
);

Gowin_PLLVR Gowin_PLLVR_inst(
    .clkout(clk_p5),
    .lock(pll_lock),
    .clkin(clk)
);

Reset_Sync u_Reset_Sync (
  .resetn(sys_resetn),
  .ext_reset(resetn & pll_lock),
  .clk(clk_p)
);


svo_hdmi svo_hdmi_inst (
	.clk(clk_p),
	.resetn(sys_resetn),
    .user_clk(gpio43),
    .user_data_3({ gpio20, gpio39, gpio40 }),
	// video clocks
	.clk_pixel(clk_p),
	.clk_5x_pixel(clk_p5),
	.locked(pll_lock),

	// output signals
	.tmds_clk_n(tmds_clk_n),
	.tmds_clk_p(tmds_clk_p),
	.tmds_d_n(tmds_d_n),
	.tmds_d_p(tmds_d_p)
);

endmodule

module Reset_Sync (
 input clk,
 input ext_reset,
 output resetn
);

 reg [3:0] reset_cnt = 0;
 
 always @(posedge clk or negedge ext_reset) begin
     if (~ext_reset)
         reset_cnt <= 4'b0;
     else
         reset_cnt <= reset_cnt + !resetn;
 end
 
 assign resetn = &reset_cnt;

endmodule
