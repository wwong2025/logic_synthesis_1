# logic_synthesis_1

Logic synthesis exercise for basic translation from verilog to basic technology independent netlist before technology mapping (developed with AI tools no paid subscription).

After translation, run_check.sh script to automatically check for logic equivalence of generated .blif file with yosys generated netlist.

Usage: 
Step 1: ./basic_compiler_1 xxxxx.v
Step 2: source [yosys_path]/environment
Step 3: ./run_check.sh xxxxx.v
