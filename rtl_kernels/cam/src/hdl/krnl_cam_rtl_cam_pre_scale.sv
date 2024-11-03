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
// (* DONT_TOUCH = "FALSE" *)
// (* use_dsp = "logic" *)
// module dspxor #(
//   parameter integer DW = 48
// )
// (
//   input  logic          aclk,
//   input  logic          areset,
//   input  logic          m_tready,
//   input  logic [DW-1:0] a,
//   input  logic [DW-1:0] b,
//   output logic [DW-1:0] c_out
// );
// logic [DW-1:0] a_in;
// logic [DW-1:0] b_in;

// always_ff @(posedge aclk) begin
//   if (areset) begin
//     c_out <= 0;
//   end
//   else if (m_tready) begin
//     a_in <= a;
//     b_in <= b;
//     c_out <= a_in ^ b_in;
//   end
// end
// endmodule

// `default_nettype none
(* DONT_TOUCH = "FALSE" *)
module krnl_cam_rtl_adder #(
  parameter integer C_DATA_WIDTH   = 512, // Data width of both input and output data
  parameter integer C_NUM_CHANNELS = 2,   // Number of input channels.  Only a value of 2 implemented.
  parameter integer CAM_SIZE = 1024,
  parameter integer INDEX_WIDTH = $clog2(CAM_SIZE),
  parameter integer NO_INDEX = INDEX_WIDTH + 1,
  parameter integer OUTPUT_ZERO = 512 - INDEX_WIDTH - 1,
  parameter integer DIVITION = 8
)
(
  input logic                                        aclk,
  input logic                                        areset,

  input logic  [C_NUM_CHANNELS-1:0]                   s_tvalid,
  input logic  [C_NUM_CHANNELS-1:0][C_DATA_WIDTH-1:0] s_tdata,
  output logic [C_NUM_CHANNELS-1:0]                   s_tready,

  output logic                                       m_tvalid,
  output logic [C_DATA_WIDTH-1:0]                    m_tdata,
  input  logic                                       m_tready,

  input  logic                                       read_done,

  // AXI-Lite Slave Interface
  input  logic [31:0]                                ctrl_1_done_in,
  output logic                                       ctrl_edge  //update done signal

);

timeunit 1ps; 
timeprecision 1ps; 

/////////////////////////////////////////////////////////////////////////////
// Variables
/////////////////////////////////////////////////////////////////////////////
logic [47:0] s_data1;
logic m_tvalid1, m_tvalid2, m_tvalid3;
logic [47:0] acc [CAM_SIZE];
// logic [29:0] A_in;
// logic [17:0] B_in;
// logic [7:0] num_index_1 [CAM_SIZE], num_index_2 [CAM_SIZE/2], num_index_3 [CAM_SIZE/4], num_index_4 [CAM_SIZE/8], num_index_5 [CAM_SIZE/16], num_index_6 [CAM_SIZE/32], num_index_7 [CAM_SIZE/64], num_index_8 [CAM_SIZE/128], num_index_9;
// logic [5:0] one_num_1 [CAM_SIZE], one_num_2 [CAM_SIZE/2], one_num_3 [CAM_SIZE/4], one_num_4 [CAM_SIZE/8], one_num_5 [CAM_SIZE/16], one_num_6 [CAM_SIZE/32], one_num_7 [CAM_SIZE/64], one_num_8 [CAM_SIZE/128], one_num_9;
logic [INDEX_WIDTH:0] num_index [DIVITION], num_index_final;
logic [INDEX_WIDTH-1:0] write_index [256]; 
logic [47:0] data_in [CAM_SIZE];
logic [47:0] data_in1 [CAM_SIZE];
logic [7:0] compare_index, compare_index1, compare_index2;

logic ctrl1 [256], ctrl_1_done [256];

logic s_tvalid1 [16], s_tvalid2 [256];
logic [C_DATA_WIDTH-1:0] s_tdata1 [16], s_tdata2 [256];
/////////////////////////////////////////////////////////////////////////////
// Logic
/////////////////////////////////////////////////////////////////////////////
//fanout 
always_ff @(posedge aclk) begin
  if (areset) begin
    for (int i = 0; i < 16; i++) begin
      s_tvalid1[i] <= 0;
      s_tdata1[i] <= 0;
    end
    for (int i = 0; i < 256; i++) begin
      s_tvalid2[i] <= 0;
      s_tdata2[i] <= 0;
    end
  end
  else begin
    for (int i = 0; i < 16; i++) begin
      s_tvalid1[i] <= s_tvalid[0];
      s_tdata1[i] <= s_tdata[0];
    end
    for (int i = 0; i < 256; i++) begin
      s_tvalid2[i] <= s_tvalid1[i/16];
      s_tdata2[i] <= s_tdata1[i/16];
    end
  end
end
always_ff @(posedge aclk) begin
  if (areset) begin
    for (int i = 0; i < 256; i++) begin
      ctrl1[i] <= 0;
    end
  end
  else begin
    for (int i = 0; i < 256; i++) begin
      ctrl1[i] <= ctrl_1_done[i];
    end
  end
end
assign ctrl_edge = !ctrl1[0] && ctrl_1_done[0];

generate begin
  genvar i;
  for (i = 0; i < CAM_SIZE/4; i++) begin
    always_ff @(posedge aclk) begin
      if (areset) begin
        // for (int i = 0; i < CAM_SIZE; i++) begin
        //   data_in[i] <= 0;
        // end
        write_index[i] <= 0;
        ctrl_1_done[i] <= 0;
      end
      else if (!ctrl_1_done[i] && s_tvalid2[i]) begin
        // for (int i = 0; i < 8; i++) begin
        //   data_in[write_index + i][47:0] <= s_tdata[0][i*64+:48];
        // end
        if (write_index[i] == (CAM_SIZE-8)) begin 
          write_index[i] <= 0;
          ctrl_1_done[i] <= 1;
        end
        else begin
          write_index[i] <= write_index[i] + 8;
        end
      end
      else if (ctrl1[i] && !ctrl_1_done_in[0]) begin //if we set ctrl_1_done_in low when starting
        ctrl_1_done[i] <= 0;
      end
    end
  end
end
endgenerate

generate begin
  genvar i;
  for (i = 0; i < CAM_SIZE; i++) begin
  DSP48E2 #(
    // Feature Control Attributes: Data Path Selection
    .AMULTSEL("A"),                    // Selects A input to multiplier (A, AD)
    .A_INPUT("DIRECT"),                // Selects A input source, "DIRECT" (A port) or "CASCADE" (ACIN port)
    .BMULTSEL("B"),                    // Selects B input to multiplier (AD, B)
    .B_INPUT("DIRECT"),                // Selects B input source, "DIRECT" (B port) or "CASCADE" (BCIN port)
    .PREADDINSEL("A"),                 // Selects input to pre-adder (A, B)
    .RND(48'h000000000000),            // Rounding Constant
    .USE_MULT("NONE"),                 // Disable the multiplier, as multiplication is not needed
    .USE_SIMD("ONE48"),                // SIMD selection (FOUR12, ONE48, TWO24)
    .USE_WIDEXOR("FALSE"),              // Enable the Wide XOR function for 48-bit operation
    .XORSIMD("XOR24_48_96"),           // Enable full 48-bit XOR function
    // Pattern Detector Attributes: Pattern Detection Configuration
    .AUTORESET_PATDET("NO_RESET"),     // No reset for pattern detection
    .AUTORESET_PRIORITY("RESET"),      // Priority of AUTORESET vs. CEP (CEP, RESET)
    .MASK(48'h3fffffffffff),           // 48-bit mask value for pattern detect (1=ignore)
    .PATTERN(48'h000000000000),        // 48-bit pattern match for pattern detect
    .SEL_MASK("MASK"),                 // Select MASK value for pattern detection
    .SEL_PATTERN("PATTERN"),           // Select pattern value for pattern detection
    .USE_PATTERN_DETECT("NO_PATDET"),  // Disable pattern detection
    // Programmable Inversion Attributes: Specifies built-in programmable inversion on specific pins
    .IS_ALUMODE_INVERTED(4'b0000),     // No inversion for ALUMODE
    .IS_CARRYIN_INVERTED(1'b0),        // No inversion for CARRYIN
    .IS_CLK_INVERTED(1'b0),            // No inversion for CLK
    .IS_INMODE_INVERTED(5'b00000),     // No inversion for INMODE
    .IS_OPMODE_INVERTED(9'b000000000), // No inversion for OPMODE
    .IS_RSTALLCARRYIN_INVERTED(1'b0),  // No inversion for RSTALLCARRYIN
    .IS_RSTALUMODE_INVERTED(1'b0),     // No inversion for RSTALUMODE
    .IS_RSTA_INVERTED(1'b0),           // No inversion for RSTA
    .IS_RSTB_INVERTED(1'b0),           // No inversion for RSTB
    .IS_RSTCTRL_INVERTED(1'b0),        // No inversion for RSTCTRL
    .IS_RSTC_INVERTED(1'b0),           // No inversion for RSTC
    .IS_RSTD_INVERTED(1'b0),           // No inversion for RSTD
    .IS_RSTINMODE_INVERTED(5'b00000),  // No inversion for RSTINMODE
    .IS_RSTM_INVERTED(1'b0),           // No inversion for RSTM
    .IS_RSTP_INVERTED(1'b0),           // No inversion for RSTP
    // Register Control Attributes: Pipeline Register Configuration
    .ACASCREG(1),                      // Number of pipeline stages between A/ACIN and ACOUT (0-2)
    .ADREG(1),                         // Pipeline stages for pre-adder (0-1)
    .ALUMODEREG(0),                    // Pipeline stages for ALUMODE (0-1)
    .AREG(1),                          // Pipeline stages for A (0-2)
    .BCASCREG(1),                      // Number of pipeline stages between B/BCIN and BCOUT (0-2)
    .BREG(1),                          // Pipeline stages for B (0-2)
    .CARRYINREG(0),                    // Disable carry-in register
    .CARRYINSELREG(0),                 // Disable carry-in select register
    .CREG(1),                          // Pipeline stages for C (0-1)
    .DREG(1),                          // Disable D input register
    .INMODEREG(0),                     // Disable INMODE register
    .MREG(0),                          // Disable multiplier register
    .OPMODEREG(0),                     // Enable OPMODE register for better control
    .PREG(1)                           // Disable P register for direct output
  )
  DSP48E2_inst (
    // Control outputs: Control Inputs/Status Bits
    .OVERFLOW(),             // Overflow status output (not used)
    .UNDERFLOW(),            // Underflow status output (not used)
    .PATTERNBDETECT(),       // Pattern B detect output (not used)
    .PATTERNDETECT(),        // Pattern detect output (not used)
    // Data outputs: Data Ports
    .P(acc[i][47:0]),                   // 48-bit output: Result of A:B XOR C
    // Data inputs: Data Ports
    .A(s_tdata2[i/4][47:18]),                   // 30-bit input: A data
    .B(s_tdata2[i/4][17:0]),                   // 18-bit input: B data
    .C(s_tdata2[i/4][(i%8)*64+:48]),                   // 48-bit input: C data
    // .C(data_in[i][47:0]),                   // 48-bit input: C data
    // Control inputs: Control Inputs/Status Bits
    .ALUMODE(4'b0100),       // Set ALUMODE to perform XOR operation
    .OPMODE(9'b000110011),   // Set OPMODE to enable A:B XOR C operation
    .CLK(aclk),               // Clock signal
    .CEA1(1'b0),             // Clock enable for A input register
    // .CEA2(m_tready),             // Clock enable for A input register
    .CEA2(1'b1),
    .CEB1(1'b0),             // Clock enable for B input register
    // .CEB2(m_tready),             // Clock enable for B input register
    .CEB2(1'b1),
    .CEC(!ctrl_1_done[i/4] && s_tvalid2[i/4] && write_index[i/4] <= i && write_index[i/4]+8 > i),
    // .CEC(1'b1),             // Clock enable for C input register
    .CEP(m_tready),             // pipeline stall
    .CEALUMODE(1'b1),         // Clock enable for ALUMODE register
    .CECTRL(1'b1),           // Clock enable for control register 
    .CED(1'b0),             // Clock enable for D input register (not used)
    .CEAD(1'b0),            // Clock enable for AD input register (not used)
    .RSTA(1'b0),             // Reset for A input register
    .RSTB(1'b0),             // Reset for B input register
    .RSTC(1'b0),             // Reset for C input register
    .RSTD(1'b0),             // Reset for D input register (not used)
    .RSTALLCARRYIN(1'b0),    // Reset for carry-in register (not used)
    .RSTALUMODE(1'b0),       // Reset for ALUMODE register
    .RSTCTRL(1'b0),          // Reset for control register
    .RSTP(1'b0)              // Reset for output register
  );
end
end
endgenerate

// always_comb begin
//   num_index = 9'h1ff;  // no match
//   for (int i = 0; i < CAM_SIZE; i++) begin
//     if (acc[i] == 0) begin
//       num_index = i;
//       break;
//     end
//   end
// end
// always_ff @(posedge aclk) begin
//   if (areset) begin
//     num_index_final <= 9'h1ff;
//   end
//   else begin
//     num_index_final <= num_index;
//   end
// end

always_ff @(posedge aclk) begin
  if (areset) begin
    for (int i = 0; i < 8; i++) begin
      num_index[i] <= {NO_INDEX{1'b1}};
    end
    num_index_final <= {NO_INDEX{1'b1}};
  end
  else begin
    for (int j = 0; j < DIVITION; j++) begin
      num_index[j] <= {NO_INDEX{1'b1}};
      for (int i = CAM_SIZE*j/DIVITION; i < CAM_SIZE*(j+1)/DIVITION; i++) begin
        if (acc[i] == 0) begin
          num_index[j] <= i;
          break;
        end
      end
    end
    // num_index[0] <= {NO_INDEX{1'b1}};
    // for (int i = 0; i < CAM_SIZE/DIVITION; i++) begin
    //   if (acc[i] == 0) begin
    //     num_index[0] <= i;
    //     break;
    //   end
    // end
    // num_index[1] <= {NO_INDEX{1'b1}};
    // for (int i = CAM_SIZE/DIVITION; i < CAM_SIZE*2/DIVITION; i++) begin
    //   if (acc[i] == 0) begin
    //     num_index[1] <= i;
    //     break;
    //   end
    // end
    // num_index[2] <= {NO_INDEX{1'b1}};
    // for (int i = CAM_SIZE*2/DIVITION; i < CAM_SIZE*3/DIVITION; i++) begin
    //   if (acc[i] == 0) begin
    //     num_index[2] <= i;
    //     break;
    //   end
    // end
    // num_index[3] <= {NO_INDEX{1'b1}};
    // for (int i = CAM_SIZE*3/DIVITION; i < CAM_SIZE*4/DIVITION; i++) begin
    //   if (acc[i] == 0) begin
    //     num_index[3] <= i;
    //     break;
    //   end
    // end
    // num_index[4] <= {NO_INDEX{1'b1}};
    // for (int i = CAM_SIZE*4/DIVITION; i < CAM_SIZE*5/DIVITION; i++) begin
    //   if (acc[i] == 0) begin
    //     num_index[4] <= i;
    //     break;
    //   end
    // end
    // num_index[5] <= {NO_INDEX{1'b1}};
    // for (int i = CAM_SIZE*5/DIVITION; i < CAM_SIZE*6/DIVITION; i++) begin
    //   if (acc[i] == 0) begin
    //     num_index[5] <= i;
    //     break;
    //   end
    // end
    // num_index[6] <= {NO_INDEX{1'b1}};
    // for (int i = CAM_SIZE*6/DIVITION; i < CAM_SIZE*7/DIVITION; i++) begin
    //   if (acc[i] == 0) begin
    //     num_index[6] <= i;
    //     break;
    //   end
    // end
    // num_index[7] <= {NO_INDEX{1'b1}};
    // for (int i = CAM_SIZE*7/DIVITION; i < CAM_SIZE*8/DIVITION; i++) begin
    //   if (acc[i] == 0) begin
    //     num_index[7] <= i;
    //     break;
    //   end
    // end
    num_index_final <= num_index[DIVITION-1];
    for (int i = 0; i < 8; i++) begin
      if (num_index[i] != {NO_INDEX{1'b1}}) begin
        num_index_final <= num_index[i];
        break;
      end
    end
  end
end

// assign m_tvalid = m_tready && ctrl_1_done;
always_ff @(posedge aclk) begin
  if (areset) begin
    m_tvalid1 <= 0;
    m_tvalid2 <= 0;
    m_tvalid <= 0;
  end
  else if (ctrl_1_done[0] && m_tready) begin
    m_tvalid1 <= s_tvalid2[0] /*| read_done*/;
    m_tvalid2 <= m_tvalid1;
    m_tvalid3 <= m_tvalid2;
    m_tvalid <= m_tvalid3;
  end
end

// always_ff @(posedge aclk) begin
//   if (areset) begin
//     compare_index <= 0;
//   end
//   else if (ctrl_1_done && s_tvalid2 && m_tready) begin
//     compare_index <= compare_index + 8'd8;
//   end
// end

always_comb begin
  if (!ctrl_1_done[0]) begin
    m_tdata = 0;
  end
  else begin
    // m_tdata = {16'b0,acc[compare_index+7],16'b0,acc[compare_index+6],16'b0,acc[compare_index+5],16'b0,acc[compare_index+4],16'b0,acc[compare_index+3],16'b0,acc[compare_index+2],16'b0,acc[compare_index+1],16'b0,acc[compare_index]};
    m_tdata = {{OUTPUT_ZERO{1'b0}}, num_index_final[INDEX_WIDTH:0]};
    // m_tdata = {464'b0, s_tdata[0][47:0]};
  end
end

always_comb begin
  if (!ctrl_1_done[0]) begin
    s_tready = &s_tvalid /*&& write_index != 8'd248*/ ? {C_NUM_CHANNELS{1'b1}} : {C_NUM_CHANNELS{1'b0}};
  end
  else begin
    // s_tready = (ctrl_edge || (compare_index == (CAM_SIZE - 8'd16) && &s_tvalid && m_tready)) ? {C_NUM_CHANNELS{1'b1}} : {C_NUM_CHANNELS{1'b0}};
    s_tready = &s_tvalid && m_tready ? {C_NUM_CHANNELS{1'b1}} : {C_NUM_CHANNELS{1'b0}};
  end
end


endmodule : krnl_cam_rtl_adder

// `default_nettype wire
