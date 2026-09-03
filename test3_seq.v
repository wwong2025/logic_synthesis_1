module sequential_test (
    input test_clk,
    input rst,
    input A,
    input B,
    input C,
    output reg Q_out
);

    wire w1;

    // Combinational logic path
    assign w1 = A & B | C;

    // Sequential block path for testing parseAlways
    always @(posedge test_clk) begin
        if (rst) begin
            Q_out <= 1'b0;
        end
        else begin
            Q_out <= w1;
        end
    end

endmodule

