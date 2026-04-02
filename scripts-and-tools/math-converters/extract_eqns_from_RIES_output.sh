#!/bin/bash

# Check if a file is provided as an argument
if [ $# -eq 0 ]; then
    echo "Please provide a file containing RIES output as an argument."
    exit 1
fi

# Check if the file exists
if [ ! -f "$1" ]; then
    echo "File not found: $1"
    exit 1
fi

# Read the file content and pass it to the Node.js script
node WASM/extract_eqns_from_RIES.js "$(cat "$1")" 