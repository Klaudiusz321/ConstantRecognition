#!/bin/bash

# USAGE: ./search.sh targetNumber Komplexity
# EXAMPLE: ./search 2.4207177617493614932545182839801 5

# Assign the input to a variable
x="$1"
# Set K (max. Kolmogorov complexity, RPN code length), to the second argument or default to 7 if not provided
K="${2:-7}"


# Beware of non-prime number NCPUS !
#NCPUS=$(nproc --all)
# getconf work on both Linux and Mac
NCPUS=$(getconf _NPROCESSORS_ONLN)
#NCPUS=7



# Determine if the input is complex based on the presence of 'i' or 'I'.
if [[ "$x" == *"i"* || "$x" == *"I"* ]]; then
  PROGRAM="./C/ComplexConstantRecognition"
else
  PROGRAM="./C/RealConstantRecognition"
fi

START_TIME=$(date +%s)
echo "Search started at $(date '+%Y-%m-%d %H:%M') using $NCPUS CPUs."
echo "Using $PROGRAM"

echo "0" > found.txt


#for i in $(seq 1 $NCPUS)
for i in $(seq 0 $((NCPUS - 1)))
do
   $PROGRAM "$x" $i $NCPUS $K &
done

wait

END_TIME=$(date +%s)
ELAPSED_TIME=$((END_TIME - START_TIME))

# Reading the content of found.txt
FOUND=$(cat found.txt)

if [ "$FOUND" -eq 1 ]; then
    RESULT_MESSAGE="Search succeeded."
else
    RESULT_MESSAGE="Search did not succeed for K<=$K. Check log files."
fi



# Define the output file
output_file="search_combined_log_$(date +%F).txt"
> "$output_file"

# Counter to keep track of the first file
first_file=1

# Loop through each log file and append its contents to the output file
for log_file in search_log_*.txt; do
    echo "Appending $log_file to $output_file"

    # Check if it's the first file
    if [ $first_file -eq 1 ]; then
        cat "$log_file" >> "$output_file"
        first_file=0
    else
        # Skip the first line (header) for subsequent files
        tail -n +2 "$log_file" >> "$output_file"
    fi
done

sort -n "$output_file" -o "$output_file"

echo "Log files combined into sorted $output_file"



echo "Search finished at $(date '+%Y-%m-%d %H:%M')."
echo "Total elapsed time: $ELAPSED_TIME seconds."
echo "$RESULT_MESSAGE"

# Check if 'js' command is available
if command -v js &> /dev/null; then
    echo "Parsing RPN codes to Mathematica..."
    ./RPN_to_Mma_batch.sh "$output_file"
else
    echo "'js' command (JavaScript) could not be found. Unable to parse RPN to Mathematica. Proceeding without parsing."
fi

exit 0
