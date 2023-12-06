/*
 *  SVO - Simple Video Out FPGA Core
 *
 *  Copyright (C) 2014  Clifford Wolf <clifford@clifford.at>
 *  
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

`timescale 1ns / 1ps
`include "svo_defines.vh"

module svo_canvas #( `CANVAS_DEFAULT_PARAMS, `SVO_DEFAULT_PARAMS ) (
    input user_clk,
    input [2:0] user_data_3, // TODO parameterize this
    input pixel_clk,
    input resetn,
    input out_axis_tready,
    output reg out_axis_tvalid,
    output reg [SVO_BITS_PER_PIXEL-1:0] out_axis_tdata,
    output reg [0:0] out_axis_tuser
);
    `SVO_DECLS
    // Note to change the text size you also have to change the char_mem.
    localparam PX_PER_PT = 8;
    localparam BITMAP_WIDTH_PTS = 8;
    localparam BITMAP_WIDTH_PX = BITMAP_WIDTH_PTS * PX_PER_PT;
    localparam BITMAP_SIZE_PTS = BITMAP_WIDTH_PTS * BITMAP_WIDTH_PTS;
    localparam DISPLAY_LENGTH_GRIDS = 16; // Must be a power of 2 for char_mem indexing.
    localparam DISPLAY_HEIGHT_GRIDS = 8;  // Must be a power of 2 for char_mem indexing.
    localparam DISPLAY_LENGTH_PX = DISPLAY_LENGTH_GRIDS * BITMAP_WIDTH_PX;
    localparam DISPLAY_HEIGHT_PX = DISPLAY_HEIGHT_GRIDS * BITMAP_WIDTH_PX;
    localparam DISPLAY_LEFT_BOUND = 64;
    localparam DISPLAY_RIGHT_BOUND = DISPLAY_LEFT_BOUND + DISPLAY_LENGTH_PX;
    localparam DISPLAY_TOP_BOUND = 0;
    localparam DISPLAY_BOT_BOUND = DISPLAY_TOP_BOUND + DISPLAY_HEIGHT_PX;
    localparam FILLED_PIXEL = { 8'd255, 8'd255, 8'd255 }; // B, G, R
    localparam BLANK_PIXEL  = { 8'd000, 8'd000, 8'd000 };

    reg next_out_axis_tvalid;
    reg [SVO_BITS_PER_PIXEL-1:0] next_out_axis_tdata;
    reg [0:0] next_out_axis_tuser;
    // https://github.com/dhepper/font8x8
    wire [$clog2(NUM_SYMBOLS)-1:0] char_code;
    wire [$clog2(DISPLAY_LENGTH_GRIDS)+$clog2(DISPLAY_HEIGHT_GRIDS)-1:0] read_addr;
    reg [$clog2(DISPLAY_HEIGHT_GRIDS)-1:0] read_offset;
    reg [$clog2(DISPLAY_HEIGHT_GRIDS)-1:0] next_read_offset;
    reg enable_read_offset;
    reg next_enable_read_offset;
    // Position on the screen that the user is writing to.
    // Once the cursor reaches the bottom-right corner, scrolling occurs.
    reg [$clog2(DISPLAY_LENGTH_GRIDS)-1:0] cursor_x;
    reg [$clog2(DISPLAY_HEIGHT_GRIDS)-1:0] cursor_y;
    reg [$clog2(DISPLAY_LENGTH_GRIDS)-1:0] next_cursor_x;
    reg [$clog2(DISPLAY_HEIGHT_GRIDS)-1:0] next_cursor_y;
    // Current screen position of the pixel that is being rendered.
    reg [`SVO_XYBITS-1:0] screen_x; // [0, 1279]
    reg [`SVO_XYBITS-1:0] screen_y; // [0, 719]
    reg [`SVO_XYBITS-1:0] next_screen_x;
    reg [`SVO_XYBITS-1:0] next_screen_y;
    // Zero-indexed counter of display area.
    reg [$clog2(DISPLAY_LENGTH_PX)-1:0] display_x;
    reg [$clog2(DISPLAY_HEIGHT_PX)-1:0] display_y;
    reg [$clog2(DISPLAY_LENGTH_PX)-1:0] next_display_x;
    reg [$clog2(DISPLAY_HEIGHT_PX)-1:0] next_display_y;
    // Current bitmap position of the pixel that is being rendered.
    // Each character is 8 units by 8 units, where each unit is 8px.
    // This means that each character is effectively scaled up to a size of 64px by 64 px.
    reg [$clog2(BITMAP_WIDTH_PTS)-1:0] bitmap_x; // [0, 7]
    reg [$clog2(BITMAP_WIDTH_PTS)-1:0] bitmap_y; // [0, 7]
    wire [$clog2(BITMAP_WIDTH_PTS)-1:0] next_bitmap_x;
    wire [$clog2(BITMAP_WIDTH_PTS)-1:0] next_bitmap_y;
    // Current grid position of the pixel that is being rendered.
    reg [$clog2(DISPLAY_LENGTH_GRIDS)-1:0]  grid_x;
    reg [$clog2(DISPLAY_HEIGHT_GRIDS)-1:0]  grid_y;
    wire [$clog2(DISPLAY_LENGTH_GRIDS)-1:0] next_grid_x;
    wire [$clog2(DISPLAY_HEIGHT_GRIDS)-1:0] next_grid_y;
    // Determines if a char_mem position is valid.
    reg [DISPLAY_LENGTH_GRIDS*DISPLAY_HEIGHT_GRIDS-1:0] char_mem_valid;
    reg [DISPLAY_LENGTH_GRIDS*DISPLAY_HEIGHT_GRIDS-1:0] next_char_mem_valid;
    reg [$clog2(NUM_SYMBOLS)-1:0] user_data_6;
    reg [$clog2(NUM_SYMBOLS)-1:0] next_user_data_6;
    reg load_upper;
    reg write_clk;
    reg write_clk_en;
    // Note that software will need to control the display such as clearing, menus, etc.
    Gowin_SDPB char_mem(
        .dout(char_code), //output [5:0] dout
        .clka(write_clk), //input clka
        .cea(1), // input cea
        .reseta(!resetn), //input reseta
        .clkb(pixel_clk), //input clkb
        .ceb(1), //input ceb
        .resetb(!resetn /*!char_mem_valid[read_addr]*/), //input resetb
        .oce(1), //input oce
        .ada({ cursor_y, cursor_x }), //input [6:0] ada
        .din(user_data_6), //input [5:0] din
        .adb(read_addr) //input [6:0] adb
    );
    wire [BITMAP_SIZE_PTS-1:0] curr_font_bitmap;
    Gowin_pROM font_bitmaps(
        .dout(curr_font_bitmap), //output [63:0] dout
        .clk(pixel_clk), //input clk
        .oce(1), //input oce
        .ce(1), //input ce
        .reset(!resetn), //input reset
        .ad(char_code) //input [5:0] ad
    );
    // Combinational logic.
    // assign read_addr = { next_grid_y, next_grid_x };   // Scrolling disabled.
    assign read_addr = { next_grid_y + read_offset, next_grid_x }; // Scrolling enabled.
    // Increment bitmap coordinates once every PX_PER_PT screen coordinates.
    assign next_bitmap_x = display_x >> $clog2(PX_PER_PT);
    assign next_bitmap_y = display_y >> $clog2(PX_PER_PT);
    // Increment grid coordinates once every BITMAP_WIDTH_PX screen coordinates.
    assign next_grid_x = next_display_x >> $clog2(BITMAP_WIDTH_PX);
    assign next_grid_y = next_display_y >> $clog2(BITMAP_WIDTH_PX);

    // Every time the user writes a character, the cursor is incremented.
    always @(*) begin
        next_cursor_x = cursor_x;
        next_cursor_y = cursor_y;
        if (cursor_x == DISPLAY_LENGTH_GRIDS-1) begin
            next_cursor_x = 0;
            if (cursor_y == DISPLAY_HEIGHT_GRIDS-1) begin
                next_cursor_y = 0;
            end else begin
                next_cursor_y = cursor_y + 1;
            end
        end else begin
            next_cursor_x = cursor_x + 1;
        end
    end

    always @(*) begin
        next_enable_read_offset = enable_read_offset;
        if (cursor_x == DISPLAY_LENGTH_GRIDS-1 && cursor_y == DISPLAY_HEIGHT_GRIDS-1) begin
            next_enable_read_offset = 1;
        end
    end

    always @(*) begin
        // Once enable_read_offset is on, we need to increment the read offset every row.
        next_read_offset = read_offset;
        if (enable_read_offset && cursor_x == 0) begin
            next_read_offset = read_offset + 1;
        end
    end

    always @(*) begin
        next_screen_x = screen_x;
        next_screen_y = screen_y;
        if (!out_axis_tvalid || out_axis_tready) begin
            if (screen_x == SVO_HOR_PIXELS-1) begin
                next_screen_x  = 0;
                if (screen_y == SVO_VER_PIXELS-1) begin
                    next_screen_y = 0;
                end else begin
                    next_screen_y = screen_y + 1;
                end
            end else begin
                next_screen_x = screen_x + 1;
            end
        end
    end

    always @(*) begin
        next_display_x = display_x;
        next_display_y = display_y;
        if (screen_x == DISPLAY_RIGHT_BOUND-1) begin
            next_display_x  = 0;
            if (screen_y == DISPLAY_BOT_BOUND-1) begin
                next_display_y = 0;
            end else if (screen_y >= DISPLAY_TOP_BOUND && screen_y < DISPLAY_BOT_BOUND) begin
                next_display_y = display_y + 1;
            end
        end else if (screen_x >= DISPLAY_LEFT_BOUND && screen_x < DISPLAY_RIGHT_BOUND) begin
            next_display_x = display_x + 1;
        end
    end

    always @(*) begin
        next_out_axis_tdata = out_axis_tdata;
        next_out_axis_tuser = out_axis_tuser;
        next_out_axis_tvalid = out_axis_tvalid;
        if (!out_axis_tvalid || out_axis_tready) begin
            if (char_mem_valid[read_addr] &&
                screen_x >= DISPLAY_LEFT_BOUND && screen_x < DISPLAY_RIGHT_BOUND &&
                screen_y >= DISPLAY_TOP_BOUND && screen_y < DISPLAY_BOT_BOUND) begin
                // Read from character memory.
                // next_out_axis_tdata = font_bitmaps[char_code_from_memory][{ bitmap_y, bitmap_x }] ? FILLED_PIXEL : BLANK_PIXEL;
                next_out_axis_tdata = curr_font_bitmap[{ bitmap_y, bitmap_x }] ? FILLED_PIXEL : BLANK_PIXEL;
            end else begin
                next_out_axis_tdata = BLANK_PIXEL;
            end
            next_out_axis_tvalid = 1;
            next_out_axis_tuser[0] = (screen_x == 0) && (screen_y == 0);
        end
    end

    integer valid_x_cntr;
    integer valid_y_cntr;
    integer i;
    always @(*) begin
        /*for (valid_y_cntr = 0; valid_y_cntr < DISPLAY_HEIGHT_GRIDS; valid_y_cntr = valid_y_cntr+1) begin
            for (valid_x_cntr = 0; valid_x_cntr < DISPLAY_LENGTH_GRIDS; valid_x_cntr = valid_x_cntr+1) begin
                if (valid_y_cntr[$clog2(DISPLAY_HEIGHT_GRIDS)-1] == cursor_y && valid_x_cntr[$clog2(DISPLAY_LENGTH_GRIDS)-1] == cursor_x) begin
                    next_char_mem_valid[{
                        valid_y_cntr[$clog2(DISPLAY_HEIGHT_GRIDS)-1:0],
                        valid_x_cntr[$clog2(DISPLAY_LENGTH_GRIDS)-1:0]
                    }] = 1;
                end else begin
                    next_char_mem_valid[{
                        valid_y_cntr[$clog2(DISPLAY_HEIGHT_GRIDS)-1:0],
                        valid_x_cntr[$clog2(DISPLAY_LENGTH_GRIDS)-1:0]
                    }] = 0;
                end
            end
        end*/
        next_char_mem_valid = char_mem_valid;
        next_char_mem_valid[{ cursor_y, cursor_x }] = 1;
        if (cursor_x == 0) begin
            for (i = 1; i < DISPLAY_LENGTH_GRIDS; i = i+1) begin
                next_char_mem_valid[{ next_cursor_y, cursor_x + i[$clog2(DISPLAY_LENGTH_GRIDS)-1:0] }] = 0;
            end
        end
    end

    always @(*) begin
        next_user_data_6 = user_data_6;
        if (load_upper) begin
            next_user_data_6[5:3] = user_data_3;
        end else begin
            next_user_data_6[2:0] = user_data_3;
        end
    end

    // Flip flops.
    always @(negedge resetn or posedge user_clk) begin
        if (!resetn) begin
            write_clk <= 0;
        end else begin
            write_clk <= !write_clk;
        end
    end

    always @(negedge resetn or negedge user_clk) begin
        if (!resetn) begin
            load_upper <= 0;
            user_data_6 <= 0;
        end else begin
            load_upper <= !load_upper;
            user_data_6 <= next_user_data_6;
        end
    end

    always @(negedge resetn or posedge load_upper) begin
        if (!resetn) begin
            cursor_x <= 0;
            cursor_y <= 0;
            write_clk_en = 0;
        end else begin
            if (write_clk_en) begin
                cursor_x <= next_cursor_x;
                cursor_y <= next_cursor_y;
            end
            write_clk_en = 1;
        end
    end

    always @(negedge resetn or posedge write_clk) begin
        if (!resetn) begin
            enable_read_offset <= 0;
            read_offset <= 0;
            char_mem_valid <= 0;
        end else begin
            enable_read_offset <= next_enable_read_offset;
            read_offset <= next_read_offset;
            char_mem_valid <= next_char_mem_valid;
        end
    end

    always @(posedge pixel_clk) begin
        if (!resetn) begin
            screen_x <= 0;
            screen_y <= 0;
            display_x <= 0;
            display_y <= 0;
            bitmap_x <= 0;
            bitmap_y <= 0;
            grid_x   <= 0;
            grid_y   <= 0;
            out_axis_tvalid <= 0;
            out_axis_tdata <= 0;
            out_axis_tuser <= 0;
    end else begin
            screen_x <= next_screen_x;
            screen_y <= next_screen_y;
            display_x <= next_display_x;
            display_y <= next_display_y;
            bitmap_x <= next_bitmap_x;
            bitmap_y <= next_bitmap_y;
            grid_x   <= next_grid_x;
            grid_y   <= next_grid_y;
            out_axis_tvalid <= next_out_axis_tvalid;
            out_axis_tdata  <= next_out_axis_tdata;
            out_axis_tuser  <= next_out_axis_tuser;
        end
    end
endmodule
