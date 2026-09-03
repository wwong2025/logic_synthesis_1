module test (
    input A,
    input B,
    input C,
    output Y,
    output Z
);

wire w1;

assign w1 = A & B ;
assign Y = w1 | C ;
assign Z = ~C ;

endmodule
