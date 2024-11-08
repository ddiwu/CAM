`timescale 1ps/1ps
`define IDLE 0
`define UPDATE_ALL 1
`define SEARCH 2
`define UPDATE_ONE 3

// 1 is symble for transfering state

module krnl_cam_rtl_FSM  #(
  parameter integer C_DATA_WIDTH  = 512,
  parameter integer OP_CODE_WIDTH = 3
)
( 
  input  logic                    clk,
  input  logic                    rst,
  input  logic [C_DATA_WIDTH-1:0] data_in,
  input  logic                    state_end,
  input  logic                    update_all_end,
  output logic [OP_CODE_WIDTH-1:0] state_pulse,
  output logic [31:0]              compare_num,
  output logic [OP_CODE_WIDTH-1:0] state
);

  always_ff @(posedge clk) begin
    if (rst) begin
      state_pulse <= 0;
    end 
    else if (state_pulse != 0) begin
      state_pulse <= 0;
    end
    else begin
      // if (state == `IDLE && data_in[1]) begin
      //   state_pulse <= `UPDATE_ALL;
      // end
      // else 
      if (state == `IDLE && data_in[2]) begin
        state_pulse <= `SEARCH;
      end
      else if (state == `IDLE && data_in[3]) begin
        state_pulse <= `UPDATE_ONE;
      end
    end
  end

  always_ff @(posedge clk) begin
    if (rst) begin
      state <= `IDLE;
      compare_num <= 0;
    end
    else begin
      case (state)
        `IDLE: begin
          if (data_in[1])
            state <= `UPDATE_ALL;
          else if (data_in[2]) begin
            state <= `SEARCH;
            compare_num <= data_in[63:32];
          end
          else if (data_in[3])
            state <= `UPDATE_ONE;
          else
            state <= `IDLE;
        end
        `UPDATE_ALL: begin
          if (update_all_end)
            state <= `IDLE;
        end
        `SEARCH: begin
          if (state_end)
            state <= `IDLE;
        end
        `UPDATE_ONE: begin
          if (state_end)
            state <= `IDLE;
        end
        default: begin
          state <= `IDLE;
        end
      endcase
    end
  end


endmodule