#!/bin/bash
# Script convert RPN code and add Mathematica readable formulas at the EOL 
# USAGE EXAMPLE: ./RPN_to_Mma_batch.sh search_combined_log_2024-03-14.txt

input_file="$1"
# Extract date from input filename or use current date
date_in_filename=$(echo "$input_file" | grep -oE '[0-9]{4}-[0-9]{2}-[0-9]{2}' || date +%F)

output_file="search_combined_log_mma_$date_in_filename.txt"

exec 3> "$output_file"

while IFS= read -r line
do
    # Extract text enclosed by {} and remove commas
    line2=$(echo "$line" | grep -oP '(?<=\{)[^\}]*(?=\})' | tr -d ',')
    result=$(js WASM/RPN_to_Mma_interpreter.mjs $line2)
    
    # Write original line and result to output file with a tab separator
    echo -e "$line\t$result" >&3
done < "$input_file"

# Close output file
exec 3>&-
