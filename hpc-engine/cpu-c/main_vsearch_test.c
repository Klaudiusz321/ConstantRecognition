/* main_vsearch_test.c - Standalone test for unified search
 *
 * Author: Andrzej Odrzywolek
 * Date: January 2, 2025
 *
 * Compilation:
 *   gcc -O2 -Wall main_vsearch_test.c vsearch_RPN_core.c utils.c -lm -o vsearch_test
 *   clang -O2 -Wall main_vsearch_test.c vsearch_RPN_core.c utils.c -lm -o vsearch_test
 *   
 *
 * Windows (Visual Studio, using x64 Native Tools Command Prompt for VS 2022):
 *   cl /O2 /W3 main_vsearch_test.c vsearch_RPN_core.c utils.c /Fe:vsearch_test.exe
 *
 *
 *  Windows (Activate and run Intel C compiler):
 *  Recommended (avoid UTF-16 BOM issues): install Power Shell 7
 *  winget install --id Microsoft.PowerShell --source winget
 *  cmd.exe "/K" '"C:\Program Files (x86)\Intel\oneAPI\setvars.bat" && pwsh'
 *  icx -O2 -Wall main_vsearch_test.c vsearch_RPN_core.c utils.c -o vsearch_test.exe
 *
 * Run:
 *   ./vsearch_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vsearch_RPN_core.h"
#include "CALC4.h"
#include "CALC_EXAMPLE.h"

/* ============================================================================
 * TEST UTILITIES
 * ============================================================================ */


static void print_result_summary(const char* json) {
   
    printf("%s\n", json);
    
}

/* ============================================================================
 * CONSTANT RECOGNITION TESTS
 * ============================================================================ */

static void test_constant_pi_squared(void) {

    
    char* result = search_constant(
        9.8696044010893586,  /* Pi^2 */
        0.0,                 /* delta */
        1, 4,                /* MinK, MaxK */
        0, 1,                /* cpu_id, ncpus */
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_REL,
        COMPARE_STRICT);
    
    print_result_summary(result);
    free(result);
}

static void test_constant_euler_exp(void) {

    
    /* Actually e^(1/12) ≈ 1.0869565217 */
    char* result = search_constant(
        1.0869040495212288886382796970131,
        0.0,
        1, 6,
        0, 1,
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_REL,
        COMPARE_STRICT);
    
    print_result_summary(result);
    free(result);
}

static void test_constant_golden_ratio(void) {

    
    char* result = search_constant(
        1.6180339887498948482045868343656,
        0.0,
        1, 3,
        0, 1,
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_REL,
        COMPARE_STRICT);
    
    print_result_summary(result);
    free(result);
}

static void test_constant_with_delta(void) {

    
    char* result = search_constant(
        3.1416,
        0.0001,              /* Non-zero delta */
        1, 4,
        0, 1,
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_REL,
        COMPARE_STRICT);
    
    print_result_summary(result);
    free(result);
}

static void test_constant_fine_structure(void) {


    
    char* result = search_constant(
        0.0072973525693,     /* alpha = 1/137.035999... */
        0.0,
        1, 6,
        0, 1,
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_REL,
        COMPARE_STRICT);
    
    print_result_summary(result);
    free(result);
}

/* ============================================================================
 * FUNCTION RECOGNITION TESTS
 * ============================================================================ */

static void test_function_exp(void) {

    
    DataPoint data[] = {
        {0.0, 1.0, 0.0},
        {0.5, 1.6487212707, 0.0},
        {1.0, 2.7182818285, 0.0},
        {1.5, 4.4816890703, 0.0},
        {2.0, 7.3890560989, 0.0}
    };
    int n_data = sizeof(data) / sizeof(data[0]);
    
    char* result = search_function(
        data, n_data,
        1, 4,
        0, 1,
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_MSE,
        COMPARE_STRICT);
    
    print_result_summary(result);
    free(result);
}

static void test_function_square(void) {

    
    DataPoint data[] = {
        {0.0, 0.0, 0.0},
        {1.0, 1.0, 0.0},
        {2.0, 4.0, 0.0},
        {3.0, 9.0, 0.0},
        {4.0, 16.0, 0.0}
    };
    int n_data = sizeof(data) / sizeof(data[0]);
    
    char* result = search_function(
        data, n_data,
        1, 4,
        0, 1,
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_MSE,
        COMPARE_STRICT);
    
    print_result_summary(result);
    free(result);
}

static void test_function_sin(void) {

    
    DataPoint data[] = {
        {0.0, 0.0, 0.0},
        {0.5, 0.4794255386, 0.0},
        {1.0, 0.8414709848, 0.0},
        {1.5, 0.9974949866, 0.0},
        {2.0, 0.9092974268, 0.0}
    };
    int n_data = sizeof(data) / sizeof(data[0]);
    
    char* result = search_function(
        data, n_data,
        1, 4,
        0, 1,
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_MSE,
        COMPARE_STRICT);
    
    print_result_summary(result);
    free(result);
}

static void test_function_x_power_inv_x(void) {


    
    /* Data for x^(1/x) */
    DataPoint data[] = {
         {0.3, 0.01807468965221857709083180176528},
         {0.6, 0.4268271965388075367207385546687}, 
         {0.9, 0.8895253798041733910316591178629}, 
         {1.2, 1.1640841385349604647157786579565}, 
         {1.5, 1.3103706971044483035708306402210}, 
         {1.8, 1.3861749885561887374982547287834}, 
         {2.1, 1.4237631864219935453169731588325}, 
         {2.4, 1.4401951720709922133071648775890}, 
         {2.7, 1.4446557054368542162945935366942}
    };
    int n_data = sizeof(data) / sizeof(data[0]);
    
    char* result = search_function(
        data, n_data,
        1, 5,
        0, 1,
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_MSE,
        COMPARE_STRICT);
    
    print_result_summary(result);
    free(result);
}

static void test_function_linear_pi(void) {


    
    DataPoint data[] = {
        {1.0, 3.14159265, 0.0},
        {2.0, 6.28318530, 0.0},
        {3.0, 9.42477796, 0.0},
        {4.0, 12.56637061, 0.0},
        {5.0, 15.70796327, 0.0}
    };
    int n_data = sizeof(data) / sizeof(data[0]);
    
    char* result = search_function(
        data, n_data,
        1, 5,
        0, 1,
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_MSE,
        COMPARE_STRICT);
    
    print_result_summary(result);
    free(result);
}

/* ============================================================================
 * ERROR METRIC TESTS
 * ============================================================================ */

static void test_metrics_comparison(void) {

    
    double target = 0.58056496476996227169614652940448;
    ErrorMetric metrics[] = {ERROR_ABS, ERROR_REL, ERROR_ULP, ERROR_HAMMING};
    const char* names[] = {"ABS", "REL", "ULP", "HAMMING"};
    
    printf("[\n");
    for (int m = 0; m < 4; m++) {
       
        char* result = search_constant(
            target, 0.0,
            1, 6,
            0, 1,
            CALC4_CONSTS, CALC4_N_CONST,
            CALC4_FUNCS,  CALC4_N_UNARY,
            CALC4_OPS,    CALC4_N_BINARY,
            metrics[m],
            COMPARE_STRICT);
        
        printf("%s",result);
        if(m<3) printf("\n,\n"); 
        free(result);
    }
    printf("]");
}

/* ============================================================================
 * COMPARE MODE TEST
 * ============================================================================ */

static void test_compare_modes(void) {


    char* result1 = search_constant(
        0.58056496476996227169614652940448, 0.0,
        1, 6,
        0, 1,
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_REL,
        COMPARE_STRICT);

    printf("[\n");

    print_result_summary(result1);
    free(result1);
    printf("\n,\n"); 

    char* result2 = search_constant(
        0.58056496476996227169614652940448, 0.0,
        1, 6,
        0, 1,
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_REL,
        COMPARE_EQUAL);
    print_result_summary(result2);
    free(result2);
    printf("]");
}

/* ============================================================================
 * MINIMAL CALCULATOR TEST
 * ============================================================================ */

static void test_minimal_calculator(void) {

    
    char* result = search_constant(
        7.3890560989306495,  /* E^2 */
        0.0,
        1, 4,
        0, 1,
        EXAMPLE_CONSTS, EXAMPLE_N_CONST,
        EXAMPLE_FUNCS,  EXAMPLE_N_UNARY,
        EXAMPLE_OPS,    EXAMPLE_N_BINARY,
        ERROR_REL,
        COMPARE_STRICT);
    
    print_result_summary(result);
    free(result);
}

/* ============================================================================
 * BATCH SEARCH TESTS
 * ============================================================================ */

static void test_batch_small_integers(void) {
    /* Find formulas for integers 1-10 */
    DataPoint data[] = {
        {1, 1.0, 0}, {2, 2.0, 0}, {3, 3.0, 0}, {4, 4.0, 0}, {5, 5.0, 0},
        {6, 6.0, 0}, {7, 7.0, 0}, {8, 8.0, 0}, {9, 9.0, 0}, {10, 10.0, 0}
    };
    int n_data = sizeof(data) / sizeof(data[0]);
    char* result = search_batch(data, n_data, -1, 1, 4, 0, 1,
        CALC4_CONSTS, CALC4_N_CONST, CALC4_FUNCS, CALC4_N_UNARY, CALC4_OPS, CALC4_N_BINARY,
        ERROR_REL, COMPARE_STRICT);
    print_result_summary(result);
    free(result);
}

static void test_batch_math_constants(void) {
    /* Find formulas for pi, e, phi, pi^2, e^2 */
    DataPoint data[] = {
        {1, 3.14159265358979, 0},   /* pi */
        {2, 2.71828182845905, 0},   /* e */
        {3, 1.61803398874989, 0},   /* phi */
        {4, 9.86960440108936, 0},   /* pi^2 */
        {5, 7.38905609893065, 0}    /* e^2 */
    };
    int n_data = sizeof(data) / sizeof(data[0]);
    char* result = search_batch(data, n_data, -1, 1, 4, 0, 1,
        CALC4_CONSTS, CALC4_N_CONST, CALC4_FUNCS, CALC4_N_UNARY, CALC4_OPS, CALC4_N_BINARY,
        ERROR_REL, COMPARE_STRICT);
    print_result_summary(result);
    free(result);
}

static void test_batch_multiple_formulas_for_137(void) {
    /* Find 4 different formulas for 137 */
    DataPoint data[] = {
        {0, 137.0, 0}, {1, 137.0, 0}, {2, 137.0, 0}, {3, 137.0, 0}
    };
    int n_data = sizeof(data) / sizeof(data[0]);
    char* result = search_batch(data, n_data, -1, 1, 6, 0, 1,
        CALC4_CONSTS, CALC4_N_CONST, CALC4_FUNCS, CALC4_N_UNARY, CALC4_OPS, CALC4_N_BINARY,
        ERROR_REL, COMPARE_STRICT);
    print_result_summary(result);
    free(result);
}

static void test_batch_stop_early(void) {
    /* Test num_to_find=3: stop after finding 3 targets */
    DataPoint data[] = {
        {1, 1.0, 0}, {2, 2.0, 0}, {3, 3.0, 0}, {4, 4.0, 0}, {5, 5.0, 0},
        {6, 6.0, 0}, {7, 7.0, 0}, {8, 8.0, 0}, {9, 9.0, 0}, {10, 10.0, 0}
    };
    int n_data = sizeof(data) / sizeof(data[0]);
    char* result = search_batch(data, n_data, 3, 1, 4, 0, 1,
        CALC4_CONSTS, CALC4_N_CONST, CALC4_FUNCS, CALC4_N_UNARY, CALC4_OPS, CALC4_N_BINARY,
        ERROR_REL, COMPARE_STRICT);
    print_result_summary(result);
    free(result);
}

static void test_batch_integer_complexity_100(void) {
    /* Integer complexity for 1-100 */
    DataPoint data[100];
    for (int i = 0; i < 100; i++) { data[i].x = i+1; data[i].y = i+1; data[i].dy = 0; }
    char* result = search_batch(data, 100, -1, 1, 6, 0, 1,
        CALC4_CONSTS, CALC4_N_CONST, CALC4_FUNCS, CALC4_N_UNARY, CALC4_OPS, CALC4_N_BINARY,
        ERROR_REL, COMPARE_STRICT);
    print_result_summary(result);
    free(result);
}



/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(int argc, char** argv) {

    
    

    /* All tests */
    //test_constant_pi_squared();
    //test_constant_euler_exp();
    //test_constant_golden_ratio();
    //test_constant_with_delta();
    //test_function_exp();
    //test_function_square();
    //test_function_sin();
    //test_function_x_power_inv_x();
    //test_function_linear_pi();
    //test_metrics_comparison();
    //test_compare_modes();
    //test_minimal_calculator();

    /* Batch tests */
    //test_batch_small_integers();
    test_batch_math_constants();
    //test_batch_multiple_formulas_for_137();
    //test_batch_stop_early();
    //test_batch_integer_complexity_100();

    
    return 0;
}
