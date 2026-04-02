#!/bin/bash
# search_new.sh - Parallel constant recognition search
#
# Author: Andrzej Odrzywolek
# Date: January 6, 2025
# Code assist: Claude 4.5 Opus
#
# Usage: ./search_new.sh <target> [MaxK]
# Example: ./search_new.sh 2.4207177617493614932545182839801 7

TARGET="$1"
K="${2:-7}"

if [ -z "$TARGET" ]; then
    echo "Usage: $0 <target> [MaxK]"
    echo "Example: $0 3.14159265358979 6"
    exit 1
fi

NCPUS=$(getconf _NPROCESSORS_ONLN)
PROGRAM="../cpu-c/vsearch_batch"
DATE_STR=$(date +%F)
START_TIME=$(date +%s)

echo "Search started at $(date '+%Y-%m-%d %H:%M') using $NCPUS CPUs."
echo "Target: $TARGET, MaxK: $K"

if [ ! -x "$PROGRAM" ]; then
    echo "Error: $PROGRAM not found or not executable."
    echo "Compile with: cd C && make vsearch_batch"
    exit 1
fi

declare -a PIDS
declare -a OUTFILES

for i in $(seq 0 $((NCPUS - 1))); do
    OUTFILE="search_log_${i}_${DATE_STR}.json"
    $PROGRAM "$TARGET" $i $NCPUS $K > "$OUTFILE" &
    PIDS+=($!)
    OUTFILES+=("$OUTFILE")
done

echo "Launched ${#PIDS[@]} workers: ${PIDS[*]}"
echo "Waiting for results..."

SUCCESS_PID=""
SUCCESS_FILE=""
SUCCESS_IDX=""

while true; do
    for idx in "${!PIDS[@]}"; do
        pid="${PIDS[$idx]}"
        [ -z "$pid" ] && continue
        if ! kill -0 "$pid" 2>/dev/null; then
            wait "$pid"
            EXIT_CODE=$?
            if [ $EXIT_CODE -eq 0 ]; then
                SUCCESS_PID="$pid"
                SUCCESS_FILE="${OUTFILES[$idx]}"
                SUCCESS_IDX="$idx"
                echo "Worker $idx (PID $pid) found solution!"
                break 2
            else
                echo "Worker $idx (PID $pid) finished without match."
                PIDS[$idx]=""
            fi
        fi
    done
    REMAINING=0
    for pid in "${PIDS[@]}"; do
        [ -n "$pid" ] && REMAINING=$((REMAINING + 1))
    done
    if [ $REMAINING -eq 0 ]; then
        echo "All workers finished without finding exact match."
        break
    fi
    sleep 1
done

if [ -n "$SUCCESS_PID" ]; then
    echo "Terminating remaining workers..."
    for idx in "${!PIDS[@]}"; do
        pid="${PIDS[$idx]}"
        [ -z "$pid" ] && continue
        [ "$idx" = "$SUCCESS_IDX" ] && continue
        kill "$pid" 2>/dev/null
    done
    wait 2>/dev/null
fi

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))

COMBINED="search_combined_log_${DATE_STR}.json"
echo "[" > "$COMBINED"
FIRST=1
for f in "${OUTFILES[@]}"; do
    if [ -f "$f" ] && [ -s "$f" ]; then
        [ $FIRST -eq 1 ] && FIRST=0 || echo "," >> "$COMBINED"
        cat "$f" >> "$COMBINED"
    fi
done
echo "]" >> "$COMBINED"

echo ""
echo "Results combined into: $COMBINED"
echo "Total elapsed time: $ELAPSED seconds."

if [ -n "$SUCCESS_PID" ]; then
    echo "SUCCESS! Solution found in: $SUCCESS_FILE"
    echo ""
    echo "Best result:"
    grep -o '"RPN":"[^"]*"' "$SUCCESS_FILE" | tail -1
    grep -o '"REL_ERR":[^,]*' "$SUCCESS_FILE" | tail -1
    exit 0
else
    echo "FAILURE: No exact match found for K <= $K"
    echo "Check $COMBINED for best approximations."
    exit 1
fi
