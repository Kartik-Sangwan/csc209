module ripple_carry(SW, LEDR)

input [9:0] SW;
output [9:0] LEDR;

wire w1, w2, w3;

full_adder FA1 (
	.cin(SW[8]);
	.a(SW[0]);
	.b(SW[1]);
	.sum(LEDR[0]);
	.cout(w1);
);

full_adder FA2 (
	.cin(w1);
	.a(SW[1]);
	.b(SW[2]);
	.sum(LEDR[1]);
	.cout(w2);
);


full_adder FA3 (
	.cin(w2);
	.a(SW[3]);
	.b(SW[4]);
	.sum(LEDR[2]);
	.cout(w3);
);

full_adder FA4 (
	.cin(w3);
	.a(SW[5]);
	.b(SW[6]);
	.sum(LEDR[3]);
	.cout(LEDR[4]);
);

endmodule


module full_adder(sum, cout, cin, a, b)

output sum, cout;
input a, b;

assign sum = a^b^cin;
assign cout = (a&b)|(cin&(a^b));

endmodule