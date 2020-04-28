// CSEE 4840 Lab 1: Display and modify the contents of a memory
//
// Spring 2020
//
// By: <your names here>
// Uni: <your unis here>

module lab1( input logic        CLOCK_50,
	     
	        output logic [7:0] VGA_R, VGA_G, VGA_B,
		output logic 	   VGA_CLK, VGA_HS, VGA_VS,
		                   VGA_BLANK_N,
		output logic 	   VGA_SYNC_N);


   logic 		      clk;

   logic reset;

   initial reset = 1;

   always @(posedge clk) reset <= 0;

   assign clk = CLOCK_50;
   logic [10:0]	   hcount;
   logic [9:0]     vcount;
   logic [23:0]    final_color;


   vga_counters counters(.clk50(clk), .*);
   tile_generator tiles(.hcount(hcount[10:1]), .vcount(vcount), .clk(clk), .*);

   always_comb begin
      {VGA_R, VGA_G, VGA_B} = {8'hff, 8'hff, 8'hff};
      if (VGA_BLANK_N )
	      {VGA_R, VGA_G, VGA_B} = {final_color[23:16], final_color[15:8], final_color[7:0]};
      	      //{VGA_R, VGA_G, VGA_B} = {8'hff, 8'hff, 8'h00};
	//if (hcount[10:6] == 5'd3 &&
	    //vcount[9:5] == 5'd3)
	  //{VGA_R, VGA_G, VGA_B} = {8'hff, 8'hff, 8'hff};
	//else
	  //{VGA_R, VGA_G, VGA_B} = {final_color[23:16], final_color[15:8], final_color[7:0]};
   end

endmodule

module vga_counters(
 input logic 	     clk50, reset,
 output logic [10:0] hcount,  // hcount[10:1] is pixel column
 output logic [9:0]  vcount,  // vcount[9:0] is pixel row
 output logic 	     VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_N, VGA_SYNC_N);

/*
 * 640 X 480 VGA timing for a 50 MHz clock: one pixel every other cycle
 *
 * HCOUNT 1599 0             1279       1599 0
 *             _______________              ________
 * ___________|    Video      |____________|  Video
 *
 *
 * |SYNC| BP |<-- HACTIVE -->|FP|SYNC| BP |<-- HACTIVE
 *       _______________________      _____________
 * |____|       VGA_HS          |____|
 */
   // Parameters for hcount
   parameter HACTIVE      = 11'd 1280,
             HFRONT_PORCH = 11'd 32,
             HSYNC        = 11'd 192,
             HBACK_PORCH  = 11'd 96,
             HTOTAL       = HACTIVE + HFRONT_PORCH + HSYNC +
                            HBACK_PORCH; // 1600

   // Parameters for vcount
   parameter VACTIVE      = 10'd 480,
             VFRONT_PORCH = 10'd 10,
             VSYNC        = 10'd 2,
             VBACK_PORCH  = 10'd 33,
             VTOTAL       = VACTIVE + VFRONT_PORCH + VSYNC +
                            VBACK_PORCH; // 525

   logic endOfLine;

   always_ff @(posedge clk50 or posedge reset)
     if (reset)          hcount <= 0;
     else if (endOfLine) hcount <= 0;
     else  	         hcount <= hcount + 11'd 1;

   assign endOfLine = hcount == HTOTAL - 1;

   logic endOfField;

   always_ff @(posedge clk50 or posedge reset)
     if (reset)          vcount <= 0;
     else if (endOfLine)
       if (endOfField)   vcount <= 0;
       else              vcount <= vcount + 10'd 1;

   assign endOfField = vcount == VTOTAL - 1;

   // Horizontal sync: from 0x520 to 0x5DF (0x57F)
   // 101 0010 0000 to 101 1101 1111
   assign VGA_HS = !( (hcount[10:8] == 3'b101) &
		      !(hcount[7:5] == 3'b111));
   assign VGA_VS = !( vcount[9:1] == (VACTIVE + VFRONT_PORCH) / 2);

   assign VGA_SYNC_N = 1'b0; // For putting sync on the green signal; unused

   // Horizontal active: 0 to 1279     Vertical active: 0 to 479
   // 101 0000 0000  1280	       01 1110 0000  480
   // 110 0011 1111  1599	       10 0000 1100  524
   assign VGA_BLANK_N = !( hcount[10] & (hcount[9] | hcount[8]) ) &
			!( vcount[9] | (vcount[8:5] == 4'b1111) );

   /* VGA_CLK is 25 MHz
    *             __    __    __
    * clk50    __|  |__|  |__|
    *
    *             _____       __
    * hcount[0]__|     |_____|
    */
   assign VGA_CLK = hcount[0]; // 25 MHz clock: rising edge sensitive

endmodule

module tile_generator (input logic [9:0] hcount, input logic [9:0] vcount, input logic clk, output logic [23:0] final_color);

     logic [7:0] pattern_name_table [767:0];
     logic [7:0] pattern_gen_table [2047:0];
     logic [47:0] color_table [31:0];
     
     logic [9:0] base_add;
     logic [7:0] tile_name;
     logic [10:0] gen_add;
     logic [7:0] pixel_row;
     logic [2:0] pixel_col;
     logic [47:0] pattern_colors;
     int i;
     int j; 
     
     initial
        begin
          //pattern_name_table[35]= 4;
	  for (i = 0; i < 255; i = i+1) pattern_name_table[i] = i;

          for (i = 0; i < 2048; i = i+1) pattern_gen_table[i] = i;
	  
	  pattern_gen_table[39]= 8'hAA;
	  pattern_gen_table[38] = 8'hAA;
	  pattern_gen_table[37] = 8'hAA;
	  pattern_gen_table[36] = 8'hAA;
	  pattern_gen_table[35] = 8'hAA;
	  pattern_gen_table[34] = 8'hAA;
	  pattern_gen_table[33] = 8'hAA;
	  pattern_gen_table[32] = 8'hAA;

          //color_table[4]={8'h80,8'h80,8'h80,8'hff,8'hff,8'h00};
	  for (i = 0; i < 32; i = i + 1) color_table[i] = {24'h0000ff, 24'hffff00};
	  //for (j = 16; j < 32; j = j + 1) color_table[j] = {24'hffff00, 24'h000000};

        end

      assign base_add={vcount[7:3], hcount[7:3]};
      assign gen_add = {tile_name[7:0], vcount[2:0]};
      //assign gen_add = vcount;
      assign pixel_col = hcount[2:0];
      assign final_color = pixel_row[pixel_col] == 1 ? pattern_colors[47:24] : pattern_colors[23:0]; 
      //assign base_add = 35;

      always_ff @(posedge clk) begin
	      tile_name <= pattern_name_table[base_add];
	      //tile_name <= 4;
	      //final_color <= 24'h0000ff;
	      //final_color <= {tile_name, tile_name, tile_name};
	      //if (tile_name == 0) final_color <= 24'hffffff;
	      //else final_color <= 24'h808080;
	      pixel_row <= pattern_gen_table[gen_add];
	      //pixel_row <= 8'hf0;
	      //pixel_row <= pattern_gen_table[vcount];
	      
	      pattern_colors <= color_table[tile_name];

      end

endmodule


