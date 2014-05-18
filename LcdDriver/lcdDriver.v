//lcdDriver

module lcdDriver (
input wire iClock,
input wire iReset,
input wire [7:0] iDataIn,
input wire iRS,
input wire iRNW,
input wire iEn,
input wire iCnt,

output wire [7:0] oDataOut,
output wire oRS,
output wire oRNW,
output wire oEn,
output wire oCnt
)

