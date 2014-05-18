// Copyright (C) 1991-2011 Altera Corporation
// Your use of Altera Corporation's design tools, logic functions 
// and other software and tools, and its AMPP partner logic 
// functions, and any output files from any of the foregoing 
// (including device programming or simulation files), and any 
// associated documentation or information are expressly subject 
// to the terms and conditions of the Altera Program License 
// Subscription Agreement, Altera MegaCore Function License 
// Agreement, or other applicable license agreement, including, 
// without limitation, that your use is for the sole purpose of 
// programming logic devices manufactured by Altera and sold by 
// Altera or its authorized distributors.  Please refer to the 
// applicable agreement for further details.

// PROGRAM		"Quartus II"
// VERSION		"Version 11.0 Build 208 07/03/2011 Service Pack 1 SJ Full Version"
// CREATED		"Tue Feb 18 11:39:28 2014"

module counting_matts_top(
	osc_clk,
	button,
	led
);


input wire	osc_clk;
input wire	[0:0] button;
output wire	[3:0] led;

wire	[31:0] counter_out;
wire	SYNTHESIZED_WIRE_0;





simple_counter	b2v_inst(
	.clock(SYNTHESIZED_WIRE_0),
	.counter_out(counter_out));


counter_bus_mux	b2v_inst1(
	.sel(button),
	.data0x(counter_out[24:21]),
	.data1x(counter_out[26:23]),
	.result(led));


pll	b2v_inst2(
	.inclk0(osc_clk),
	.c0(SYNTHESIZED_WIRE_0));


endmodule
