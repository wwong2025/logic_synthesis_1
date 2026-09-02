#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

// Represents combinational logic gates mapped to BLIF truth tables
struct Gate {
    std::string op;             // AND, OR, XOR, NAND, NOR, XNOR, NOT, BUF
    std::string output_net;
    std::vector<std::string> input_nets;
};

// Represents sequential state elements mapped to BLIF .latch
struct Latch {
    std::string input_d;
    std::string output_q;
    std::string clock;
    std::string edge_type;      // "re" (rising/posedge) or "fe" (falling/negedge)
    int init_val;               // 0, 1, or 2 (don't care / uninitialized)
};

struct Netlist {
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    std::vector<std::string> wires;
    std::vector<Gate> gates;
    std::vector<Latch> latches;
};

class BlifSynthesizer {
private:
    Netlist netlist;

    std::string clean(std::string str) {
        str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
        str.erase(std::remove(str.begin(), str.end(), ';'), str.end());
        str.erase(std::remove(str.begin(), str.end(), '('), str.end());
        str.erase(std::remove(str.begin(), str.end(), ')'), str.end());
        return str;
    }

    void registerWire(const std::string& net) {
        if (std::find(netlist.outputs.begin(), netlist.outputs.end(), net) == netlist.outputs.end() &&
            std::find(netlist.inputs.begin(), netlist.inputs.end(), net) == netlist.inputs.end() &&
            std::find(netlist.wires.begin(), netlist.wires.end(), net) == netlist.wires.end()) {
            netlist.wires.push_back(net);
        }
    }

public:
    void parseDeclaration(const std::string& line) {
        std::stringstream ss(line);
        std::string type, token;
        ss >> type;
        while (ss >> token) {
            token = clean(token);
            if (token.empty()) continue;
            if (type == "input")  netlist.inputs.push_back(token);
            if (type == "output") netlist.outputs.push_back(token);
            if (type == "wire")   netlist.wires.push_back(token);
        }
    }

    // Handles single-line and multi-input assigns: assign Y = A & B & C;
    void parseAssignment(const std::string& line) {
        std::stringstream ss(line);
        std::string assign_kw, lhs, eq_token;
        ss >> assign_kw >> lhs >> eq_token;
        lhs = clean(lhs);

        std::vector<std::string> tokens;
        std::string tok;
        while (ss >> tok) {
            tok = clean(tok);
            if (!tok.empty()) tokens.push_back(tok);
        }

        if (tokens.empty()) return;

        Gate gate;
        gate.output_net = lhs;

        // Unary NOT: assign Z = ~C;
        if (tokens.size() == 1 && tokens[0][0] == '~') {
            gate.op = "NOT";
            gate.input_nets.push_back(tokens[0].substr(1));
        }
        // Direct buffer / pass-through: assign Z = C;
        else if (tokens.size() == 1) {
            gate.op = "BUF";
            gate.input_nets.push_back(tokens[0]);
        }
        // Multi-input gates: A & B & C ... or A | B | C ...
        else if (tokens.size() >= 3) {
            std::string raw_op = tokens[1]; // Operators sit at odd indices like tokens[1], tokens[3]...

            if (raw_op == "&")        gate.op = "AND";
            else if (raw_op == "|")   gate.op = "OR";
            else if (raw_op == "^")   gate.op = "XOR";
            else if (raw_op == "~&")  gate.op = "NAND";
            else if (raw_op == "~|")  gate.op = "NOR";
            else if (raw_op == "~^" || raw_op == "^~") gate.op = "XNOR";

            for (size_t i = 0; i < tokens.size(); i += 2) {
                gate.input_nets.push_back(tokens[i]);
            }
        }

        registerWire(lhs);
        netlist.gates.push_back(gate);
    }

    // Handles sequential clocked registers safely
    void parseAlways(const std::string& line, std::ifstream& infile) {
        std::stringstream ss(line);
        std::string always_kw, at_symbol, edge, clk;
        ss >> always_kw >> at_symbol >> edge >> clk;
        edge = clean(edge);
        clk = clean(clk);

        std::string edge_type = (edge == "negedge") ? "fe" : "re";

        std::string body_line;
        while (std::getline(infile, body_line)) {
            size_t first = body_line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) continue;
            std::string stmt = body_line.substr(first);

            // Break if block ends or hits module termination safely
            if (stmt.rfind("end", 0) == 0 || stmt.rfind("endmodule", 0) == 0) break;

            // Non-blocking assignment: q <= d;
            if (stmt.find("<=") != std::string::npos) {
                std::stringstream bss(stmt);
                std::string q_net, arrow, d_net;
                bss >> q_net >> arrow >> d_net;
                q_net = clean(q_net);
                d_net = clean(d_net);

                Latch latch;
                latch.output_q = q_net;
                latch.input_d = d_net;
                latch.clock = clk;
                latch.edge_type = edge_type;
                latch.init_val = 2; // Uninitialized / don't care

                registerWire(q_net);
                netlist.latches.push_back(latch);
            }
        }
    }

    void exportToBlif(const std::string& model_name, const std::string& output_filename) {
        std::ofstream outfile(output_filename);
        if (!outfile.is_open()) {
            std::cerr << "Error: Could not open output file: " << output_filename << std::endl;
            return;
        }

        outfile << "# Generated by Extended C++ BLIF Synthesizer\n";
        outfile << ".model " << model_name << "\n";

        // Primary Inputs
        outfile << ".inputs";
        for (const auto& in : netlist.inputs) outfile << " " << in;
        outfile << "\n";

        // Primary Outputs
        outfile << ".outputs";
        for (const auto& out : netlist.outputs) outfile << " " << out;
        outfile << "\n\n";

        // 1. Synthesize Clocked Sequential Latches
        for (const auto& l : netlist.latches) {
            outfile << ".latch " << l.input_d << " " << l.output_q << " "
                    << l.edge_type << " " << l.clock << " " << l.init_val << "\n";
        }
        if (!netlist.latches.empty()) outfile << "\n";

        // 2. Synthesize Combinational Logic Networks
        for (const auto& gate : netlist.gates) {
            size_t n = gate.input_nets.size();
            outfile << ".names";
            for (const auto& in : gate.input_nets) outfile << " " << in;
            outfile << " " << gate.output_net << "\n";

            if (gate.op == "BUF") {
                outfile << "1 1\n";
            } else if (gate.op == "NOT") {
                outfile << "0 1\n";
            } else if (gate.op == "AND") {
                outfile << std::string(n, '1') << " 1\n";
            } else if (gate.op == "NAND") {
                for (size_t i = 0; i < n; ++i) {
                    std::string row(n, '-');
                    row[i] = '0';
                    outfile << row << " 1\n";
                }
            } else if (gate.op == "OR") {
                for (size_t i = 0; i < n; ++i) {
                    std::string row(n, '-');
                    row[i] = '1';
                    outfile << row << " 1\n";
                }
            } else if (gate.op == "NOR") {
                outfile << std::string(n, '0') << " 1\n";
            } else if (gate.op == "XOR" || gate.op == "XNOR") {
                size_t total_combinations = (size_t)1 << n;
                for (size_t i = 0; i < total_combinations; ++i) {
                    int ones_count = 0;
                    std::string row = "";
                    for (size_t bit = n; bit > 0; --bit) {
                        if ((i >> (bit - 1)) & 1) {
                            row += "1";
                            ones_count++;
                        } else {
                            row += "0";
                        }
                    }
                    bool parity = (ones_count % 2 != 0);
                    if (gate.op == "XNOR") parity = !parity;
                    if (parity) {
                        outfile << row << " 1\n";
                    }
                }
            }
            outfile << "\n";
        }

        outfile << ".end\n";
        outfile.close();
        std::cout << "Successfully synthesized BLIF to: " << output_filename << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source.v>" << std::endl;
        return 1;
    }

    std::string input_filepath = argv[1];
    std::ifstream infile(input_filepath);
    if (!infile.is_open()) {
        std::cerr << "Error: Cannot open file " << input_filepath << std::endl;
        return 1;
    }

    BlifSynthesizer synth;
    std::string line;

    while (std::getline(infile, line)) {
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        std::string trimmed = line.substr(first);

        // Ignore empty lines, module wrappers, or end signals in global scope
        if (trimmed.rfind("module", 0) == 0 || trimmed.rfind("endmodule", 0) == 0) {
            continue;
        }

        if (trimmed.rfind("input", 0) == 0 || trimmed.rfind("output", 0) == 0 || trimmed.rfind("wire", 0) == 0) {
            synth.parseDeclaration(trimmed);
        } else if (trimmed.rfind("assign", 0) == 0) {
            synth.parseAssignment(trimmed);
        } else if (trimmed.rfind("always", 0) == 0 || trimmed.rfind("always_ff", 0) == 0) {
            synth.parseAlways(trimmed, infile);
        }
    }
    infile.close();

    size_t last_dot = input_filepath.find_last_of(".");
    std::string base_name = (last_dot == std::string::npos) ? input_filepath : input_filepath.substr(0, last_dot);
    std::string output_filepath = base_name + ".blif";
    size_t last_slash = base_name.find_last_of("\\");
    std::string model_name = (last_slash == std::string::npos) ? base_name : base_name.substr(last_slash + 1);
    
    synth.exportToBlif(model_name, output_filepath);
    
    return 0;
}