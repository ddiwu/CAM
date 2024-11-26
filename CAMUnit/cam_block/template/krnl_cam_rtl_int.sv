`define IDLE 0
`define UPDATE_ALL 1
`define SEARCH 2
`define UPDATE_ONE 3
`default_nettype none
`timescale 1 ns / 1 ps 
(* DONT_TOUCH = "FALSE" *)

module krnl_cam_rtl_int #( 
  parameter integer  C_S_AXI_CONTROL_DATA_WIDTH = 32,
  parameter integer  C_S_AXI_CONTROL_ADDR_WIDTH = 6,
  parameter integer  C_M_AXI_GMEM_ID_WIDTH = 1,
  parameter integer  C_M_AXI_GMEM_ADDR_WIDTH = 64,
  parameter integer  C_M_AXI_GMEM_DATA_WIDTH = 512,
  parameter integer  CAM_SIZE = CUSTORMIZED_CAM_SIZE,
  parameter integer  C_DATA_WIDTH = 512
)
(
  // System signals
  input  wire  ap_clk,
  input  wire  ap_rst_n,
  // AXI4 master interface 
  input  wire [C_DATA_WIDTH-1:0] p0_TDATA,
  input  wire        p0_TVALID,
  output wire        p0_TREADY,
  output wire [C_DATA_WIDTH-1:0] p1_TDATA,
  output wire        p1_TVALID,
  input  wire        p1_TREADY

  // AXI4-Lite slave interface
  // input  wire                                    s_axi_control_AWVALID,
  // output wire                                    s_axi_control_AWREADY,
  // input  wire [C_S_AXI_CONTROL_ADDR_WIDTH-1:0]   s_axi_control_AWADDR,
  // input  wire                                    s_axi_control_WVALID,
  // output wire                                    s_axi_control_WREADY,
  // input  wire [C_S_AXI_CONTROL_DATA_WIDTH-1:0]   s_axi_control_WDATA,
  // input  wire [C_S_AXI_CONTROL_DATA_WIDTH/8-1:0] s_axi_control_WSTRB,
  // input  wire                                    s_axi_control_ARVALID,
  // output wire                                    s_axi_control_ARREADY,
  // input  wire [C_S_AXI_CONTROL_ADDR_WIDTH-1:0]   s_axi_control_ARADDR,
  // output wire                                    s_axi_control_RVALID,
  // input  wire                                    s_axi_control_RREADY,
  // output wire [C_S_AXI_CONTROL_DATA_WIDTH-1:0]   s_axi_control_RDATA,
  // output wire [1:0]                              s_axi_control_RRESP,
  // output wire                                    s_axi_control_BVALID,
  // input  wire                                    s_axi_control_BREADY,
  // output wire [1:0]                              s_axi_control_BRESP,
  // output wire                                    interrupt 
);
///////////////////////////////////////////////////////////////////////////////
// Local Parameters (constants)
///////////////////////////////////////////////////////////////////////////////
localparam integer LP_NUM_READ_CHANNELS  = 1;
localparam integer LP_LENGTH_WIDTH       = 32;
localparam integer LP_DW_BYTES           = C_M_AXI_GMEM_DATA_WIDTH/8;
localparam integer LP_AXI_BURST_LEN      = 4096/LP_DW_BYTES < 256 ? 4096/LP_DW_BYTES : 256;
localparam integer LP_LOG_BURST_LEN      = $clog2(LP_AXI_BURST_LEN);
localparam integer LP_RD_MAX_OUTSTANDING = 3;
localparam integer LP_RD_FIFO_DEPTH      = LP_AXI_BURST_LEN*(LP_RD_MAX_OUTSTANDING + 1);
localparam integer LP_WR_FIFO_DEPTH      = LP_AXI_BURST_LEN;
localparam integer LP_FIFO_DEPTH         = 32;
localparam integer OP_CODE_WIDTH         = 3;

///////////////////////////////////////////////////////////////////////////////
// Variables
///////////////////////////////////////////////////////////////////////////////
logic areset = 1'b0;  
logic ap_start;
logic ap_start_pulse;
logic ap_start_r;
logic ap_ready;
logic ap_done;
logic ap_idle = 1'b1;
logic [C_M_AXI_GMEM_ADDR_WIDTH-1:0] a;
logic [C_M_AXI_GMEM_ADDR_WIDTH-1:0] c;
logic [LP_LENGTH_WIDTH-1:0]         length_r;

logic read_done;
logic [LP_NUM_READ_CHANNELS-1:0] rd_tvalid;
logic [LP_NUM_READ_CHANNELS-1:0] rd_tready_n; 
logic [LP_NUM_READ_CHANNELS-1:0] [C_M_AXI_GMEM_DATA_WIDTH-1:0] rd_tdata;
logic [LP_NUM_READ_CHANNELS-1:0] ctrl_rd_fifo_prog_full;
logic [LP_NUM_READ_CHANNELS-1:0] rd_fifo_tvalid_n;
logic [LP_NUM_READ_CHANNELS-1:0] rd_fifo_tready; 
logic [LP_NUM_READ_CHANNELS-1:0] [C_M_AXI_GMEM_DATA_WIDTH-1:0] rd_fifo_tdata;

logic                               cam_tvalid;
logic                               adder_tready_n; 
logic [C_M_AXI_GMEM_DATA_WIDTH-1:0] adder_tdata;
logic                               wr_fifo_tvalid_n;
logic                               wr_fifo_tready; 
logic [C_M_AXI_GMEM_DATA_WIDTH-1:0] wr_fifo_tdata;
logic wr_fifo_prog_full;

logic [31:0] ctrl_1_done;
logic ctrl_1_done_in;
logic final_transfer;
logic p0_tready_reg = 1'b0;
logic [OP_CODE_WIDTH-1:0] state;
logic [OP_CODE_WIDTH-1:0] state_pulse;
logic update_all_end;
logic [31:0] compare_num;
logic [31:0] count;

///////////////////////////////////////////////////////////////////////////////
// RTL Logic 
///////////////////////////////////////////////////////////////////////////////
// Tie-off unused AXI protocol features

// Register and invert reset signal for better timing.
always @(posedge ap_clk) begin 
  areset <= ~ap_rst_n; 
end

always_ff @(posedge ap_clk) begin
  if (areset) begin
    count <= 0;
  end
  else if (state_pulse == `SEARCH) begin
    count <= compare_num - 1;
  end
  else if (p0_TVALID && p0_TREADY && count != 0) begin
    count <= count - 1;
  end
end

assign compare_num = p0_TDATA[61:32];

krnl_cam_rtl_FSM #(
  .C_DATA_WIDTH ( C_DATA_WIDTH ),
  .OP_CODE_WIDTH( OP_CODE_WIDTH )
)
inst_FSM (
  .clk       ( ap_clk            ) ,
  .rst       ( areset            ) ,
  .data_in   ( p0_TDATA          ) ,
  .data_in_valid (p0_TVALID & p0_TREADY) ,
  .search_end ( count == 0 && p0_TVALID && p0_TREADY) , 
  .state_pulse(state_pulse       ) ,
  .update_all_end( update_all_end) ,
  .state     ( state             ) 
);

// Combinatorial Adder
krnl_cam_rtl_adder #( 
  .C_DATA_WIDTH   ( C_DATA_WIDTH           ) ,
  .C_NUM_CHANNELS ( LP_NUM_READ_CHANNELS    ) ,
  .CAM_SIZE       ( CAM_SIZE                ),
  .OP_CODE_WIDTH  ( OP_CODE_WIDTH           )
)
inst_adder ( 
  .aclk     ( ap_clk            ) ,
  .areset   ( areset            ) ,
  .state    ( state             ) ,
  .s_tvalid ( p0_TVALID & p0_TREADY ) ,
  // .s_tready ( rd_fifo_tready    ) ,
  .s_tdata  ( p0_TDATA          ) ,
  .update_all_end( update_all_end ) ,
  .m_tvalid ( cam_tvalid      ) ,
  // .m_tready ( ~wr_fifo_prog_full) ,
  .m_tdata  ( adder_tdata       )
  // .ctrl_edge( ctrl_1_done_in    ) , //upedge of ctrl_1_done
  // .ctrl_1_done_in( ctrl_1_done  ) //axilite to make ctrl_1_done_in low
);

// xpm_fifo_sync: Synchronous FIFO
// Xilinx Parameterized Macro, Version 2016.4
xpm_fifo_sync # (
  .FIFO_MEMORY_TYPE          ("auto"),           //string; "auto", "block", "distributed", or "ultra";
  .ECC_MODE                  ("no_ecc"),         //string; "no_ecc" or "en_ecc";
  .FIFO_WRITE_DEPTH          (LP_FIFO_DEPTH),   //positive integer
  .WRITE_DATA_WIDTH          (C_DATA_WIDTH),               //positive integer
  .WR_DATA_COUNT_WIDTH       ($clog2(LP_FIFO_DEPTH+1)),               //positive integer, Not used
  .PROG_FULL_THRESH          (LP_FIFO_DEPTH-5),               //positive integer, 3 pipeline stages, 
  .FULL_RESET_VALUE          (1),                //positive integer; 0 or 1
  .READ_MODE                 ("fwft"),            //string; "std" or "fwft";
  .FIFO_READ_LATENCY         (1),                //positive integer;
  .READ_DATA_WIDTH           (C_DATA_WIDTH),               //positive integer
  .RD_DATA_COUNT_WIDTH       ($clog2(LP_FIFO_DEPTH+1)),               //positive integer, not used
  .PROG_EMPTY_THRESH         (10),               //positive integer, not used 
  .DOUT_RESET_VALUE          ("0"),              //string, don't care
  .WAKEUP_TIME               (0)                 //positive integer; 0 or 2;

) inst_wr_xpm_fifo_sync (
  .sleep         ( 1'b0             ) ,
  .rst           ( areset           ) ,
  .wr_clk        ( ap_clk           ) ,
  .wr_en         ( cam_tvalid     ) ,
  .din           ( adder_tdata      ) ,
  .full          ( adder_tready_n   ) ,
  .prog_full     ( wr_fifo_prog_full) ,
  .wr_data_count (                  ) ,
  .overflow      (                  ) ,
  .wr_rst_busy   (                  ) ,
  .rd_en         ( wr_fifo_tready   ) ,
  .dout          ( wr_fifo_tdata    ) ,
  .empty         ( wr_fifo_tvalid_n ) ,
  .prog_empty    (                  ) ,
  .rd_data_count (                  ) ,
  .underflow     (                  ) ,
  .rd_rst_busy   (                  ) ,
  .injectsbiterr ( 1'b0             ) ,
  .injectdbiterr ( 1'b0             ) ,
  .sbiterr       (                  ) ,
  .dbiterr       (                  ) 

);



always @(posedge ap_clk) begin 
  p0_tready_reg <= ~wr_fifo_prog_full /*& ~ap_idle*/;
end

assign p0_TREADY = p0_tready_reg;

assign p1_TVALID = ~wr_fifo_tvalid_n;
assign p1_TDATA  = wr_fifo_tdata; 
assign wr_fifo_tready = p1_TREADY;

endmodule : krnl_cam_rtl_int

`default_nettype wire
