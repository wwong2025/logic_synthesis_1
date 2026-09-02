#!/bin/bash
if [ -z "$1" ]; then
    echo "Usage: ./run_check.sh <file.v>"
    exit 1
fi

V_FILE="$1"
BLIF_FILE="${V_FILE%.*}.blif"

# 1. Run your custom C++ compiler
./basic_compiler_1 "$V_FILE"

# 2. Run Yosys inline verification passing the dynamic filenames
yosys -p "
    # --- Step A: Read and Synthesize Golden Reference ---
    read_verilog -sv $V_FILE
    hierarchy -auto-top
    proc; opt; fsm; opt; memory; opt; techmap; opt; splitnets; opt
    rename -top gold_${BASE_NAME}
    design -save golden
    design -reset

    # --- Step B: Read Custom BLIF Output ---
    read_blif $BLIF_FILE
    rename -top gate_${BASE_NAME}

    # --- Step C: Copy Golden Reference into Active Design ---
    design -copy-from golden -as gold_${BASE_NAME} gold_${BASE_NAME}

    # --- Step D: Formal Equivalence Check ---
    equiv_make gold_${BASE_NAME} gate_${BASE_NAME} equiv_miter
    hierarchy -top equiv_miter
    opt_clean -purge
    opt -purge
    equiv_induct
    equiv_status -assert
"
