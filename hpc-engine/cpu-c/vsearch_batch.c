/* vsearch_batch.c - CLI wrapper for shell-based parallel search
 *
 * Author: Andrzej Odrzywolek, andrzej.odrzywolek@uj.edu.pl
 * Date: January 6, 2025
 * Code assist: Claude 4.5 Opus
 *
 * Usage:
 *   ./vsearch_batch <target> <cpu_id> <ncpus> <MaxK>
 *
 * Exit codes:
 *   0 = SUCCESS (exact match found)
 *   1 = FAILURE (no exact match)
 *
 * Compilation:
 *   gcc -O2 -Wall vsearch_batch.c vsearch_RPN_core.c utils.c -lm -o vsearch_batch
 *   clang -O2 -Wall vsearch_batch.c vsearch_RPN_core.c utils.c -lm -o vsearch_batch
 *   icx -O2 -Wall vsearch_batch.c vsearch_RPN_core.c utils.c -o vsearch_batch
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vsearch_RPN_core.h"
#include "CALC4.h"

int main(int argc, char** argv) {
    double target = 0.0;
    double delta = 0.0;
    int cpu_id = 0;
    int ncpus = 1;
    int MaxK = 6;
    int MinK = 1;

    if (argc < 5) {
        fprintf(stderr, "Usage: %s <target> <cpu_id> <ncpus> <MaxK> [MinK] [delta]\n", argv[0]);
        fprintf(stderr, "Example: %s 3.14159265358979 0 4 6\n", argv[0]);
        return 1;
    }

    sscanf(argv[1], "%lf", &target);
    sscanf(argv[2], "%d", &cpu_id);
    sscanf(argv[3], "%d", &ncpus);
    sscanf(argv[4], "%d", &MaxK);
    if (argc > 5) sscanf(argv[5], "%d", &MinK);
    if (argc > 6) sscanf(argv[6], "%lf", &delta);

    char* result = search_constant(
        target, delta,
        MinK, MaxK,
        cpu_id, ncpus,
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_REL,
        COMPARE_STRICT);

    printf("%s\n", result);

    int success = (strstr(result, "\"result\":\"SUCCESS\"") != NULL);
    free(result);

    return success ? 0 : 1;
}
