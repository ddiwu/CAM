`timescale 1ps/1ps
`define IDLE 32'hffffff00
`define UPDATE_ALL 32'hffffff01
`define SEARCH 32'hffffff03
`define UPDATE_ONE 32'hffffff02

// 1 is symble for transfering state

module krnl_cam_rtl_FSM  #(
  parameter integer C_DATA_WIDTH  = 512,
  parameter integer OP_CODE_WIDTH = 32
)
( 
  input  logic                    clk,
  input  logic                    rst,
  input  logic [C_DATA_WIDTH-1:0] data_in,
  input  logic                    search_end,
  input  logic                    update_all_end,
  input  logic                    data_in_valid,
  output logic [OP_CODE_WIDTH-1:0] state_pulse,
  output logic [OP_CODE_WIDTH-1:0] state
);

  // always_ff @(posedge clk) begin
  //   if (rst) begin
  //     state_pulse <= 0;
  //   end 
  //   else if (state_pulse != 0) begin
  //     state_pulse <= 0;
  //   end
  //   else begin
  //     if (state == `IDLE && data_in[2] && data_in_valid) begin
  //       state_pulse <= `SEARCH;
  //     end
  //     else if (state == `IDLE && data_in[3] && data_in_valid) begin
  //       state_pulse <= `UPDATE_ONE;
  //     end
  //   end
  // end
  always_comb begin 
    if (state == `IDLE && data_in[511:480]==`SEARCH && data_in_valid) begin
      state_pulse = `SEARCH;
    end
    else if (state == `IDLE && data_in[511:480]==`UPDATE_ONE && data_in_valid) begin
      state_pulse = `UPDATE_ONE;
    end
    else begin
      state_pulse = 0;
    end
  end

  always_ff @(posedge clk) begin
    if (rst) begin
      state <= `IDLE;
    end
    else begin
      case (state)
        `IDLE: begin
          if (data_in[511:480]==`UPDATE_ALL && data_in_valid)
            state <= `UPDATE_ALL;
          else if (data_in[511:480] ==`SEARCH && data_in_valid) begin
            state <= `SEARCH;
          end
          else if (data_in[511:480]==`UPDATE_ONE && data_in_valid)
            state <= `UPDATE_ONE;
          else
            state <= `IDLE;
        end
        `UPDATE_ALL: begin
          if (update_all_end)
            state <= `IDLE;
        end
        `SEARCH: begin
          if (search_end )
            state <= `IDLE;
        end
        `UPDATE_ONE: begin
          if (search_end)
            state <= `IDLE;
        end
        default: begin
          state <= `IDLE;
        end
      endcase
    end
  end


endmodule