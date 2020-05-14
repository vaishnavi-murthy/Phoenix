/*
 * Avalon memory-mapped peripheral that generates VGA
 *
 * Stephen A. Edwards
 * Columbia University
 */

module vga_ball(input logic        clk,
                input logic        reset,
                input logic [7:0]  writedata,
                input logic        write,
                input              chipselect,
                input logic [4:0]  address,

                output logic [7:0] VGA_R, VGA_G, VGA_B,
                output logic       VGA_CLK, VGA_HS, VGA_VS,
                                   VGA_BLANK_n,
                output logic       VGA_SYNC_n);

   logic [10:0]    hcount;
   logic [9:0]     vcount;
   logic [23:0]    final_color;
   logic [23:0]    sprite_color;

   logic [7:0]    sprite_name;
   logic [7:0]    new_y_row1;
   logic [7:0]    new_y_row2;
   logic [7:0]    new_x_column1;
   logic [7:0]    new_x_column2;
   logic [7:0]    sprite_change;
   logic [7:0]    new_name;
   logic [7:0]    new_tag;
   logic [1:0]     is_sprite;

   vga_counters counters(.clk50(clk), .*);

   tile_generator tiles(.hcount(hcount[10:0]), .vcount(vcount), .clk(clk), .*);
   sprite_generator sprites(.hcount(hcount[10:0]), .vcount(vcount), .clk(clk), .sprite_change(sprite_change), .sprite_name(sprite_name), .new_x_column1(new_x_column1), .new_x_column2(new_x_column2), .new_y_row1(new_y_row1), .new_y_row2(new_y_row2), .new_name(new_name), .new_tag(new_tag), .*);

   always_comb begin
      {VGA_R, VGA_G, VGA_B} = {8'hff, 8'hff, 8'hff};
      if (VGA_BLANK_n )
              //{VGA_R, VGA_G, VGA_B} = {sprite_color[23:16], sprite_color[15:8], sprite_color[7:0]};
              if (is_sprite == 2'b01)
                      {VGA_R, VGA_G, VGA_B} = {sprite_color[23:16], sprite_color[15:8], sprite_color[7:0]};
              else
                      {VGA_R, VGA_G, VGA_B} = {final_color[23:16], final_color[15:8], final_color[7:0]};

              //{VGA_R, VGA_G, VGA_B} = {8'hff, 8'hff, 8'h00};
        //if (hcount[10:6] == 5'd3 &&
            //vcount[9:5] == 5'd3)
          //{VGA_R, VGA_G, VGA_B} = {8'hff, 8'hff, 8'hff};
        //else
          //{VGA_R, VGA_G, VGA_B} = {final_color[23:16], final_color[15:8], final_color[7:0]};
   end


   always_ff @(posedge clk)
     if (reset) begin
        sprite_change <= 8'b0;
        sprite_name <= 8'b0;
        new_x_column1 <= 8'b0;
	new_x_column2 <= 8'b0;
        new_y_row1 <= 8'b0;
	new_y_row2 <= 8'b0;
	new_name <= 8'b0;
	new_tag <= 8'b0;
     end else if (chipselect && write)
       case (address)
	 3'h0 : sprite_change <= writedata;
	 3'h1 : sprite_name <= writedata;
	 3'h2 : new_x_column1 <= writedata;
	 3'h3 : new_x_column2 <= writedata;
	 3'h4 : new_y_row1 <= writedata;
	 3'h5 : new_y_row2 <= writedata;
	 3'h6 : new_name <= writedata;
         3'h7 : new_tag <= writedata;
       endcase


endmodule

module vga_counters(
 input logic 	     clk50, reset,
 output logic [10:0] hcount,  // hcount[10:1] is pixel column
 output logic [9:0]  vcount,  // vcount[9:0] is pixel row
 output logic 	     VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n, VGA_SYNC_n);

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

   assign VGA_SYNC_n = 1'b0; // For putting sync on the green signal; unused
   
   // Horizontal active: 0 to 1279     Vertical active: 0 to 479
   // 101 0000 0000  1280	       01 1110 0000  480
   // 110 0011 1111  1599	       10 0000 1100  524
   assign VGA_BLANK_n = !( hcount[10] & (hcount[9] | hcount[8]) ) &
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

module tile_generator (input logic [10:0] hcount, input logic [9:0] vcount, input logic clk, output logic [23:0] final_color);

     logic [7:0] pattern_name_table [4799:0];
     logic [7:0] pattern_gen_table [2047:0];
     logic [47:0] color_table [31:0];

     logic [13:0] base_add;
     logic [7:0] tile_name;
     logic [10:0] gen_add;
     logic [7:0] pixel_row;
     logic [3:0] pixel_col;
     logic [47:0] pattern_colors;
     int i;
     int j;

     initial
        begin
          for (i = 0; i < 4800; i = i+1) pattern_name_table[i] = 31;

          for (i = 12; i < 17; i = i+1) pattern_name_table[i] = i-1; // Score 1
          pattern_name_table[17] = 1;

          pattern_name_table[32] = 16;
          pattern_name_table[33] = 17;
          pattern_name_table[34] = 10;
          for (i = 35; i < 40; i = i+1) pattern_name_table[i] = i-24;  // Hi Score

          for (i = 55; i < 60; i = i+1) pattern_name_table[i] = i-44; // Score 2
          pattern_name_table[60] = 9;

          for (i = 93; i < 97; i = i+1) pattern_name_table[i] = 0;
          for (i = 114; i < 118; i = i+1) pattern_name_table[i] = 0;
          for (i = 136; i < 140; i = i+1) pattern_name_table[i] = 0;

          pattern_name_table[174] = 20; // Life ship for score 1
          pattern_name_table[175] = 3;

          //pattern_name_table[194] = 20;
          pattern_name_table[217] = 20; // Life ship for score 2
          pattern_name_table[218] = 3;
          for (i = 0; i < 2048; i = i+1) pattern_gen_table[i] = 8'hff;
          // Number 0
          pattern_gen_table[0] = 8'h08;
          pattern_gen_table[1] = 8'h14;
          pattern_gen_table[2] = 8'h22;
          pattern_gen_table[3] = 8'h22;
          pattern_gen_table[4] = 8'h22;
          pattern_gen_table[5] = 8'h22;
          pattern_gen_table[6] = 8'h14;
          pattern_gen_table[7] = 8'h08;

          // Number 1
          pattern_gen_table[15] = 8'h7C;
          pattern_gen_table[14] = 8'h10;
          pattern_gen_table[13] = 8'h10;
          pattern_gen_table[12] = 8'h10;
          pattern_gen_table[11] = 8'h10;
          pattern_gen_table[10] = 8'h14;
          pattern_gen_table[9] = 8'h18;
          pattern_gen_table[8] = 8'h10;

          // Number 2
          pattern_gen_table[16] = 8'h1E; //3c
          pattern_gen_table[17] = 8'h42; //44
          pattern_gen_table[18] = 8'h40; //40
          pattern_gen_table[19] = 8'h20; //20
          pattern_gen_table[20] = 8'h10; //10
          pattern_gen_table[21] = 8'h08; //08
          pattern_gen_table[22] = 8'h04; //04
          pattern_gen_table[23] = 8'h1E; //4c


          // Number 3
          pattern_gen_table[31] = 8'h3E;
          pattern_gen_table[30] = 8'h40; //20
          pattern_gen_table[29] = 8'h40; //10
          pattern_gen_table[28] = 8'h20; //08
          pattern_gen_table[27] = 8'h30; //10
          pattern_gen_table[26] = 8'h40; //20
          pattern_gen_table[25] = 8'h40; //20
          pattern_gen_table[24] = 8'h3E;

          // Number 9
	  pattern_gen_table[79] = 8'h00;
	  pattern_gen_table[78] = 8'hF8;
 	  pattern_gen_table[77] = 8'h88;
 	  pattern_gen_table[76] = 8'h88;
 	  pattern_gen_table[75] = 8'hF8;
 	  pattern_gen_table[74] = 8'h08;
 	  pattern_gen_table[73] = 8'h08;
	  pattern_gen_table[72] = 8'h08;

          // Dash --> 10
          pattern_gen_table[80] = 8'h00;
          pattern_gen_table[81] = 8'h00;
          pattern_gen_table[82] = 8'h00;
          pattern_gen_table[83] = 8'h00;
          pattern_gen_table[84] = 8'h7C;
          pattern_gen_table[85] = 8'h00;
          pattern_gen_table[86] = 8'h00;
          pattern_gen_table[87] = 8'h00;

          // S --> 11
          pattern_gen_table[95] = 8'h3E;
          pattern_gen_table[94] = 8'h40;
          pattern_gen_table[93] = 8'h20;
          pattern_gen_table[92] = 8'h10;
          pattern_gen_table[91] = 8'h08;
          pattern_gen_table[90] = 8'h04;
          pattern_gen_table[89] = 8'h02;
          pattern_gen_table[88] = 8'h7C;

          // C --> 12
          pattern_gen_table[103] = 8'h7C;
          pattern_gen_table[102] = 8'h02;
          pattern_gen_table[101] = 8'h02;
          pattern_gen_table[100] = 8'h02;
          pattern_gen_table[99] = 8'h02;
          pattern_gen_table[98] = 8'h02;
          pattern_gen_table[97] = 8'h02;
          pattern_gen_table[96] = 8'h7C;

          // R --> 14
          pattern_gen_table[112] = 8'h3E;
          pattern_gen_table[113] = 8'h22;
          pattern_gen_table[114] = 8'h22;
          pattern_gen_table[115] = 8'h3E;
          pattern_gen_table[116] = 8'h06;
          pattern_gen_table[117] = 8'h0A;
          pattern_gen_table[118] = 8'h12;
          pattern_gen_table[119] = 8'h22;

          // E --> 15
          pattern_gen_table[120] = 8'h3E;
          pattern_gen_table[121] = 8'h02;
          pattern_gen_table[122] = 8'h02;
          pattern_gen_table[123] = 8'h1E;
          pattern_gen_table[124] = 8'h02;
          pattern_gen_table[125] = 8'h02;
          pattern_gen_table[126] = 8'h02;
          pattern_gen_table[127] = 8'h3E;

          // O --> 13
          pattern_gen_table[111] = 8'h7E;
          pattern_gen_table[110] = 8'h42;
          pattern_gen_table[109] = 8'h42;
          pattern_gen_table[108] = 8'h42;
          pattern_gen_table[107] = 8'h42;
          pattern_gen_table[106] = 8'h42;
          pattern_gen_table[105] = 8'h42;
          pattern_gen_table[104] = 8'h7E;

          // H --> 16
          pattern_gen_table[128] = 8'h44;
          pattern_gen_table[129] = 8'h44;
          pattern_gen_table[130] = 8'h44;
          pattern_gen_table[131] = 8'h7C;
          pattern_gen_table[132] = 8'h44;
          pattern_gen_table[133] = 8'h44;
          pattern_gen_table[134] = 8'h44;
          pattern_gen_table[135] = 8'h44;

          // I --> 17
          pattern_gen_table[136] = 8'h7C;
          pattern_gen_table[137] = 8'h10;
          pattern_gen_table[138] = 8'h10;
          pattern_gen_table[139] = 8'h10;
          pattern_gen_table[140] = 8'h10;
          pattern_gen_table[141] = 8'h10;
          pattern_gen_table[142] = 8'h10;
          pattern_gen_table[143] = 8'h7C;

          // Life spaceship
          pattern_gen_table[167] = 8'hAA; //49
          pattern_gen_table[166] = 8'h6C; //2a
          pattern_gen_table[165] = 8'h38; //1c
          pattern_gen_table[164] = 8'h10; //08
          pattern_gen_table[163] = 8'h10; //08
          pattern_gen_table[162] = 8'h7C; //1c
          pattern_gen_table[161] = 8'h54; //3e
          pattern_gen_table[160] = 8'h94; //5d


          for (i = 0; i < 32; i = i + 1) color_table[i] = {24'h000000, 24'hffffff}; // Background is black and white
          for (i = 0; i < 11; i = i+1) color_table[i] = {24'hff0000, 24'h000000};  // Numbers are red and black
          for (i = 11; i < 18; i = i+1) color_table[i] = {24'h00ffff, 24'h000000}; // Letters are blue and black
          color_table[20] = {24'h00ff00, 24'h000000}; // Color scheme for spaceship

        end

      assign base_add={vcount[8:3], hcount[10:4]} - 48*vcount[8:3];
      assign gen_add = {tile_name, vcount[2:0]};
      assign pixel_col = hcount[3:1];
      assign final_color = pixel_row[pixel_col] == 1 ? pattern_colors[47:24] : pattern_colors[23:0];

      always_ff @(posedge clk) begin
              tile_name <= pattern_name_table[base_add];

              pixel_row <= pattern_gen_table[gen_add];

              pattern_colors <= color_table[tile_name];
      end

endmodule

module sprite_generator (input logic [10:0] hcount, input logic [9:0] vcount, input logic clk, input logic [7:0] sprite_change, input logic [7:0] sprite_name, input logic [7:0] new_x_column1, input logic [7:0] new_x_column2, input logic [7:0] new_y_row1, input logic [7:0] new_y_row2, input logic [7:0] new_name, input logic [7:0] new_tag, output logic [23:0] sprite_color, output logic [1:0] is_sprite);

	logic [15:0] sprite_att_table [32:0];
	logic [31:0] pattern_table [2047:0];
	logic [95:0] color_table [6:0];

        //logic [] pos_y;
        logic [12:0] pattern_add;
        logic [15:0] name;
        logic [9:0] vertical;
        logic [10:0] horizontal;
        logic [31:0] pixel_row;
        logic [5:0] pixel_col;
	logic [5:0] before_col;
	logic [5:0] after_col;
        logic [95:0] colors;
        int i;
        int j;


        initial
           begin
                sprite_att_table[0] = 400;  // vertical position to start
                sprite_att_table[1] = 550;  // horizontal position to start
                sprite_att_table[2] = 0;      // Name [Address to find ship]
                sprite_att_table[3] = 1;      // tag

		sprite_att_table[4] = 300; // Player bullet
		sprite_att_table[5] = 550;
		sprite_att_table[6] = 1;
		sprite_att_table[7] = 1;

		sprite_att_table[8] = 200; // Bird 1
		sprite_att_table[9] = 200;
		sprite_att_table[10] = 2;
		sprite_att_table[11] = 1;

		sprite_att_table[12] = 200; // Bird 2
		sprite_att_table[13] = 250;
		sprite_att_table[14] = 2;
		sprite_att_table[15] = 1;

		sprite_att_table[16] = 200; // Bird 3
		sprite_att_table[17] = 150;
		sprite_att_table[18] = 2;
		sprite_att_table[19] = 1;

		sprite_att_table[20] = 225; // bird bullet 1
		sprite_att_table[21] = 200;
		sprite_att_table[22] = 5;
		sprite_att_table[23] = 1;

		sprite_att_table[24] = 200; // bird bullet 2
		sprite_att_table[25] = 650;
		sprite_att_table[26] = 5;
		sprite_att_table[27] = 1;
		
		sprite_att_table[28] = 225; // bird bullet 3
		sprite_att_table[29] = 650;
		sprite_att_table[30] = 5;
		sprite_att_table[31] = 1;

		sprite_att_table[32] = 9; // blank name

		pattern_table[0] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[1] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[2] = {8'h00, 8'h01, 8'h80, 8'h00};
		pattern_table[3] = {8'h00, 8'h01, 8'h80, 8'h00};
		pattern_table[4] = {8'h00, 8'h01, 8'h80, 8'h00};
		pattern_table[5] = {8'h00, 8'h43, 8'hC2, 8'h00};
		pattern_table[6] = {8'h00, 8'h43, 8'hC2, 8'h00};
		pattern_table[7] = {8'h00, 8'h43, 8'hC2, 8'h00};
		pattern_table[8] = {8'h00, 8'h63, 8'hC6, 8'h00};
		pattern_table[9] = {8'h00, 8'h73, 8'hCE, 8'h00};
		pattern_table[10] = {8'h00, 8'h7B, 8'hDE, 8'h00};
		pattern_table[11] = {8'h00, 8'h7F, 8'hFE, 8'h00};
		pattern_table[12] = {8'h00, 8'h5F, 8'hFA, 8'h00};
		pattern_table[13] = {8'h00, 8'h4F, 8'hF2, 8'h00};
		pattern_table[14] = {8'h00, 8'h07, 8'hE0, 8'h00};
		pattern_table[15] = {8'h00, 8'h03, 8'hC0, 8'h00};
		pattern_table[16] = {8'h00, 8'h03, 8'hC0, 8'h00};
		pattern_table[17] = {8'h00, 8'h07, 8'hE0, 8'h00};
		pattern_table[18] = {8'h00, 8'h8F, 8'hF1, 8'h00};
		pattern_table[19] = {8'h00, 8'h9F, 8'hF9, 8'h00};
		pattern_table[20] = {8'h00, 8'hBF, 8'hFD, 8'h00};
		pattern_table[21] = {8'h00, 8'hFF, 8'hFF, 8'h00};
		pattern_table[22] = {8'h00, 8'hFB, 8'hDF, 8'h00};
		pattern_table[23] = {8'h00, 8'hF3, 8'hCF, 8'h00};
		pattern_table[24] = {8'h00, 8'hE3, 8'hC7, 8'h00};
		pattern_table[25] = {8'h00, 8'hC3, 8'hC3, 8'h00};
		pattern_table[26] = {8'h00, 8'h83, 8'hC1, 8'h00};
		pattern_table[27] = {8'h00, 8'h9F, 8'hF9, 8'h00};
		pattern_table[28] = {8'h00, 8'h99, 8'h99, 8'h00};
		pattern_table[29] = {8'h00, 8'h99, 8'h99, 8'h00};
		pattern_table[30] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[31] = {8'h00, 8'h00, 8'h00, 8'h00};

		for (i = 32; i < 64; i = i+1) pattern_table[i] = {8'h00, 8'h00, 8'h00, 8'h00};
		for (i = 45; i < 51; i = i+1) pattern_table[i] = {8'h00, 8'h01, 8'h00, 8'h00};

		// Bird
		for (i = 64; i < 69; i = i+1) pattern_table[i] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[69] = {8'h00, 8'h06, 8'h60, 8'h00};
		pattern_table[70] = {8'h00, 8'h86, 8'h61, 8'h00};
		pattern_table[71] = {8'h01, 8'hC1, 8'h83, 8'h80};
		pattern_table[72] = {8'h03, 8'hE1, 8'h87, 8'hC0};
		pattern_table[73] = {8'h07, 8'hF1, 8'h8F, 8'hE0};
		pattern_table[74] = {8'h0F, 8'hF9, 8'h9F, 8'hF0};
		pattern_table[75] = {8'h0F, 8'hFF, 8'hFF, 8'hF0};
		pattern_table[76] = {8'h0F, 8'hFF, 8'hFF, 8'hF0};
		pattern_table[77] = {8'h0F, 8'h8F, 8'hF1, 8'hF0};
		pattern_table[78] = {8'h0F, 8'h8F, 8'hF1, 8'hF0};
		pattern_table[79] = {8'h0F, 8'h8F, 8'hF1, 8'hF0};
		pattern_table[80] = {8'h0F, 8'h8F, 8'hF1, 8'hF0};
		pattern_table[81] = {8'h07, 8'h8F, 8'hF1, 8'hE0};
		pattern_table[82] = {8'h03, 8'hCF, 8'hF3, 8'hC0};
		pattern_table[83] = {8'h01, 8'hE7, 8'hE7, 8'h80};
		pattern_table[84] = {8'h00, 8'hE3, 8'hC7, 8'h00};
		pattern_table[85] = {8'h00, 8'h61, 8'h86, 8'h00};
		pattern_table[86] = {8'h00, 8'h21, 8'h84, 8'h00};
		pattern_table[87] = {8'h00, 8'h21, 8'h84, 8'h00};
		for (i = 88; i < 96; i = i+1) pattern_table[i] = {8'h00, 8'h00, 8'h00, 8'h00};

		// flying bird moving left
		for (i = 96; i < 104; i = i+1) pattern_table[i] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[104] = {8'h00, 8'h1F, 8'hFF, 8'hC0};
		pattern_table[105] = {8'h00, 8'h1C, 8'h1F, 8'h80};
		pattern_table[106] = {8'h00, 8'h78, 8'h1F, 8'h00};
		pattern_table[107] = {8'h00, 8'h00, 8'h1E, 8'h00};
		pattern_table[108] = {8'h00, 8'h07, 8'hFC, 8'h00};
		pattern_table[109] = {8'h00, 8'h0F, 8'hF8, 8'h60};
		pattern_table[110] = {8'h00, 8'h1F, 8'hF8, 8'h60};
		pattern_table[111] = {8'h00, 8'hFF, 8'hFF, 8'h80};
		pattern_table[112] = {8'h00, 8'hFF, 8'hFF, 8'h80};
		pattern_table[113] = {8'h00, 8'h1F, 8'hF8, 8'h60};
		pattern_table[114] = {8'h00, 8'h0F, 8'hF8, 8'h60};
		pattern_table[115] = {8'h00, 8'h07, 8'hFC, 8'h00};
		pattern_table[116] = {8'h00, 8'h00, 8'h1E, 8'h00};
		pattern_table[117] = {8'h00, 8'h78, 8'h1F, 8'h00};
		pattern_table[118] = {8'h00, 8'h1C, 8'h1F, 8'h80};
		pattern_table[119] = {8'h00, 8'h1F, 8'hFF, 8'hC0};
		for (i = 120; i < 128; i = i+1) pattern_table[i] = {8'h00, 8'h00, 8'h00, 8'h00};

		// flying bird moving right
		for (i = 128; i < 136; i = i+1) pattern_table[i] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[136] = {8'h03, 8'hFF, 8'hFC, 8'h00};
		pattern_table[137] = {8'h01, 8'hF8, 8'h3C, 8'h00};
		pattern_table[138] = {8'h00, 8'hF8, 8'h1E, 8'h00};
		pattern_table[139] = {8'h00, 8'h78, 8'h00, 8'h00};
		pattern_table[140] = {8'h00, 8'h3F, 8'h07, 8'h00};
		pattern_table[141] = {8'h06, 8'h1F, 8'hF0, 8'h60};
		pattern_table[142] = {8'h06, 8'h1F, 8'hF8, 8'h60};
		pattern_table[143] = {8'h01, 8'hFF, 8'hFF, 8'h80};	
		pattern_table[144] = {8'h01, 8'hFF, 8'hFF, 8'h80};
		pattern_table[145] = {8'h06, 8'h1F, 8'hF8, 8'h60};
		pattern_table[146] = {8'h06, 8'h1F, 8'hF0, 8'h60};
		pattern_table[147] = {8'h00, 8'h3F, 8'h07, 8'h00};
		pattern_table[148] = {8'h00, 8'h78, 8'h00, 8'h00};
		pattern_table[149] = {8'h00, 8'hF8, 8'h1E, 8'h00};
		pattern_table[150] = {8'h01, 8'hF8, 8'h3C, 8'h00};
		pattern_table[151] = {8'h03, 8'hFF, 8'hFC, 8'h00};	
		for (i = 152; i < 160; i = i+1) pattern_table[i] = {8'h00, 8'h00, 8'h00, 8'h00};


		// Bullets for the bird
		for (i = 160; i < 192; i = i+1) pattern_table[i] = {8'h00, 8'h00, 8'h00, 8'h00};
		for (i = 174; i < 178; i = i+1) pattern_table[i] = {8'h00, 8'h01, 8'h80, 8'h00};

		for (i = 192; i < 224; i = i+1) pattern_table[i] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[192] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[193] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[194] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[195] = {8'h00, 8'h08, 8'h00, 8'h00};
		pattern_table[196] = {8'h00, 8'h08, 8'h00, 8'h00};
		pattern_table[197] = {8'h18, 8'h60, 8'h00, 8'h00};
		pattern_table[198] = {8'h3C, 8'hF0, 8'h82, 8'h08};
		pattern_table[199] = {8'h3C, 8'hF8, 8'h98, 8'h1C};

		pattern_table[200] = {8'h19, 8'hFC, 8'h3C, 8'h1C};
		pattern_table[201] = {8'h01, 8'hFE, 8'h7E, 8'h08};
		pattern_table[202] = {8'h03, 8'hFF, 8'hFF, 8'h80};
		pattern_table[203] = {8'h0F, 8'hFF, 8'hFF, 8'hC0};
		pattern_table[204] = {8'h1F, 8'hFF, 8'hFF, 8'hE0};
		pattern_table[205] = {8'h1F, 8'hFF, 8'hFF, 8'hE0};
		pattern_table[206] = {8'h0C, 8'hFF, 8'hFF, 8'hE0};
		pattern_table[207] = {8'h22, 8'hFF, 8'hFF, 8'hC0};

		pattern_table[208] = {8'h01, 8'hFF, 8'hFF, 8'h90};
		pattern_table[209] = {8'h00, 8'hFF, 8'hFF, 8'hC0};
		pattern_table[210] = {8'h20, 8'hFF, 8'hFF, 8'hE0};
		pattern_table[211] = {8'h71, 8'hFF, 8'hFF, 8'hF0};
		pattern_table[212] = {8'h23, 8'hFF, 8'hFF, 8'hF0};
		pattern_table[213] = {8'h0F, 8'hFF, 8'hFF, 8'hF0};
		pattern_table[214] = {8'h1F, 8'hFF, 8'hFF, 8'hF0};
		pattern_table[215] = {8'h0F, 8'hDF, 8'hFB, 8'hE0};

		pattern_table[216] = {8'h47, 8'h8F, 8'hF1, 8'hC0};
		pattern_table[217] = {8'h03, 8'h07, 8'hE0, 8'h00};
		pattern_table[218] = {8'h00, 8'h03, 8'hC0, 8'h00};
		pattern_table[219] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[220] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[221] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[222] = {8'h00, 8'h00, 8'h00, 8'h00};
		pattern_table[223] = {8'h00, 8'h00, 8'h00, 8'h00};


		color_table[0] = {24'hffff00, 24'hff0000, 24'hffffff, 24'h000000};
       		color_table[1] = {24'hffffff, 24'h000000, 24'h000000, 24'h000000};
		color_table[2] = {24'hff00ff, 24'h00ff00, 24'h000000, 24'h000000};
		color_table[3] = {24'hff00ff, 24'h00ff00, 24'h000000, 24'h000000};
		color_table[4] = {24'hff00ff, 24'h00ff00, 24'h000000, 24'h000000};
		color_table[5] = {24'hffffff, 24'h000000, 24'h000000, 24'h000000};
		color_table[6] = {24'hff00ff, 24'hffff00, 24'h000000, 24'h000000};
           end

        assign pattern_add = {name[7:0], vertical[4:0]};
	assign pixel_col = horizontal[5:1]; // OG 5:1
	assign before_col = pixel_col - 5'b00001;
	assign after_col = pixel_col + 5'b00001;
	assign is_sprite = name == sprite_att_table[32] ? 2'b00 : 2'b01;

        always_comb begin
                horizontal = hcount;
                vertical = vcount;
                name = sprite_att_table[32];
		for (j = 0; j < 32; j = j+4) // Make this more dynamic
			if (vcount >= sprite_att_table[j] && vcount < sprite_att_table[j] + 32 && hcount >= sprite_att_table[j+1] && hcount < sprite_att_table[j+1] + 64 && sprite_att_table[j+3] == 1) begin
				horizontal = hcount - sprite_att_table[j+1];
				vertical = vcount - sprite_att_table[j];
				name = sprite_att_table[j+2];
			end 
        end


        always_ff @(posedge clk) begin
		if (sprite_change[0] == 1'b1) begin
                  sprite_att_table[(sprite_name << 2)] = {new_y_row1, new_y_row2};
                  sprite_att_table[(sprite_name << 2) + 1'b1] = {new_x_column1, new_x_column2};
		  sprite_att_table[(sprite_name << 2) + 2'b10] = {8'b0, new_name};
		  sprite_att_table[(sprite_name << 2) + 2'b11] = {8'b0, new_tag};
                end

                pixel_row <= pattern_table[pattern_add];

                colors <= color_table[name];
	end


	always_comb begin
		case ({pixel_row[before_col], pixel_row[pixel_col], pixel_row[after_col]})
			3'b010: sprite_color = colors[95:72];
			3'b011: sprite_color = colors[95:72];
			3'b110: sprite_color = colors[95:72];
			3'b111: sprite_color = colors[71:48];
		default: sprite_color = colors[23:0];
	endcase
	end



endmodule
