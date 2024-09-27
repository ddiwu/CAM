/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

////////////////////////////////////////////////////////////////////////////////
// Description: Basic Adder using DSP48E2, no overflow. Unsigned. Combinatorial.
////////////////////////////////////////////////////////////////////////////////

`default_nettype none

(* use_dsp = "logic" *)
module krnl_cam_rtl_adder #(
  parameter integer C_DATA_WIDTH   = 32, // Data width of both input and output data
  parameter integer C_NUM_CHANNELS = 2   // Number of input channels.  Only a value of 2 implemented.
)
(
  input wire                                         aclk,
  input wire                                         areset,

  input wire  [C_NUM_CHANNELS-1:0]                   s_tvalid,
  input wire  [C_NUM_CHANNELS-1:0][C_DATA_WIDTH-1:0] s_tdata,
  output wire [C_NUM_CHANNELS-1:0]                   s_tready,

  output logic                                       m_tvalid,
  output wire [C_DATA_WIDTH-1:0]                     m_tdata,
  input  wire                                        m_tready

  // AXI-Lite Slave Interface
  //output logic                                       ctrl_1_done  //update done signal

);

timeunit 1ps; 
timeprecision 1ps; 

/////////////////////////////////////////////////////////////////////////////
// Variables
/////////////////////////////////////////////////////////////////////////////
logic [C_DATA_WIDTH-1:0] acc;
logic [15:0] dummy;
logic [5:0] num;

logic [5:0][2:0][47:0] data_in;
/////////////////////////////////////////////////////////////////////////////
// Logic
/////////////////////////////////////////////////////////////////////////////
// always_ff @(posedge aclk) begin
//   if (areset) begin
//     data_in <= 0;
//     num <= 0;
//     ctrl_1_done <= 0;
//   end
//   else if (ctrl_1_done) begin
//     data_in <= data_in;
//     num <= num;
//   end
//   else if (num == 6'b111111 && &s_tvalid) begin
//     for (int i = 0; i < 8; i++) begin
//       data_in[num][i] <= s_tdata[0][i*64+:48];
//     end
//     num <= 0;
//     ctrl_1_done <= 1;
//   end
//   else if (&s_tvalid) begin
//     for (int i = 0; i < 8; i++) begin
//       data_in[num][i] <= s_tdata[0][i*64+:48];
//     end
//     num <= num + 1;
//   end
// end

// after clock cycles, update ctrl_1_done
// always_ff @(posedge aclk) begin
//   if (areset) begin
//     ctrl_1_done <= 0;
//     num <= 0;
//   end
//   else if (num == 6'b111111) begin
//     ctrl_1_done <= 1;
//     num <= 0;
//   end
//   else if (&s_tvalid) begin 
//     ctrl_1_done <= 0;
//     num <= num + 1;
//   end
// end


// always_ff @(posedge aclk) begin
//   if (areset) begin
//     dummy <= 0;
//     acc <= 0;
//     m_tvalid <= 0;
//   end
//   else if (&s_tvalid) begin
//     {dummy, acc} <= {16'b0, s_tdata[0]} ^ {16'b0, s_tdata[1]};
//     m_tvalid <= 1;
//   end
//   else begin
//     dummy <= 0;
//     acc <= 0;
//     m_tvalid <= 0;
//   end
// end

always_comb begin 
    acc = s_tdata[0] ^ s_tdata[1];
end

assign m_tvalid = &s_tvalid;
assign m_tdata = acc;

// Only assert s_tready when transfer has been accepted.  tready asserted on all channels simultaneously
assign s_tready = m_tready & m_tvalid ? {C_NUM_CHANNELS{1'b1}} : {C_NUM_CHANNELS{1'b0}};

endmodule : krnl_cam_rtl_adder

`default_nettype wire
