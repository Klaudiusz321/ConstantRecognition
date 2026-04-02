// Constant Recognition - WebGPU Compute Shader
// Author: Klaudiusz Sroka, December 2025
// Evaluates RPN expressions in parallel on GPU

// ============================================================================
// CONSTANTS
// ============================================================================

const PI: f32 = 3.14159265358979323846;
const E: f32 = 2.71828182845904523536;
const PHI: f32 = 1.61803398874989484820;  // Golden ratio
const NEG_ONE: f32 = -1.0;

const N_CONST: u32 = 13u;
const N_UNARY: u32 = 18u;
const N_BINARY: u32 = 5u;
const STACK_SIZE: u32 = 16u;
const MAX_K: u32 = 12u;

// ============================================================================
// UNIFORMS & BUFFERS
// ============================================================================

struct Params {
    search_target: f32,
    threshold: f32,
    K: u32,
    total_combinations: u32,
    // Radix packed as 3 vec4<u32> for proper alignment (12 values total)
    form_radix_0: vec4<u32>,  // radix[0..3]
    form_radix_1: vec4<u32>,  // radix[4..7]
    form_radix_2: vec4<u32>,  // radix[8..11]
    batch_start: u32,
    _pad1: u32,
    _pad2: u32,
    _pad3: u32,
}

struct Counter {
    count: atomic<u32>
}

struct Result {
    error: f32,
    idx: u32,
    valid: u32,
    _pad: u32,
}

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var<storage, read> ternary_form: array<u32>;  // 0=const, 1=unary, 2=binary
@group(0) @binding(2) var<storage, read_write> results: array<Result>;
@group(0) @binding(3) var<storage, read_write> global_counter: Counter;

// ============================================================================
// MATHEMATICAL CONSTANTS LOOKUP
// ============================================================================

fn get_constant(idx: u32) -> f32 {
    switch idx {
        case 0u: { return PI; }
        case 1u: { return E; }
        case 2u: { return NEG_ONE; }
        case 3u: { return PHI; }
        case 4u: { return 1.0; }
        case 5u: { return 2.0; }
        case 6u: { return 3.0; }
        case 7u: { return 4.0; }
        case 8u: { return 5.0; }
        case 9u: { return 6.0; }
        case 10u: { return 7.0; }
        case 11u: { return 8.0; }
        case 12u: { return 9.0; }
        default: { return 0.0; }
    }
}

// ============================================================================
// UNARY FUNCTIONS
// ============================================================================

fn apply_unary(op: u32, x: f32) -> f32 {
    switch op {
        case 0u: { return log(x); }           // ln
        case 1u: { return exp(x); }           // exp
        case 2u: { return 1.0 / x; }          // reciprocal
        // case 3u: { return tgamma(x); }     // gamma - not in WGSL, skip
        case 3u: { return x; }                // placeholder for gamma
        case 4u: { return sqrt(x); }          // sqrt
        case 5u: { return x * x; }            // square
        case 6u: { return sin(x); }           // sin
        case 7u: { return asin(x); }          // asin
        case 8u: { return cos(x); }           // cos
        case 9u: { return acos(x); }          // acos
        case 10u: { return tan(x); }          // tan
        case 11u: { return atan(x); }         // atan
        case 12u: { return sinh(x); }         // sinh
        case 13u: { return asinh(x); }        // asinh
        case 14u: { return cosh(x); }         // cosh
        case 15u: { return acosh(x); }        // acosh
        case 16u: { return tanh(x); }         // tanh
        case 17u: { return atanh(x); }        // atanh
        default: { return x; }
    }
}

// ============================================================================
// BINARY FUNCTIONS
// ============================================================================

fn apply_binary(op: u32, a: f32, b: f32) -> f32 {
    switch op {
        case 0u: { return a + b; }            // add
        case 1u: { return a * b; }            // multiply
        case 2u: { return a - b; }            // subtract
        case 3u: { return a / b; }            // divide
        case 4u: { return pow(a, b); }        // power
        default: { return 0.0; }
    }
}

// ============================================================================
// INDEX TO SLOT CONVERSION
// Converts global index to specific instruction slots for each position
// ============================================================================

// Helper function to get radix value from packed vec4s
fn get_radix(i: u32) -> u32 {
    if (i < 4u) {
        return params.form_radix_0[i];
    } else if (i < 8u) {
        return params.form_radix_1[i - 4u];
    } else {
        return params.form_radix_2[i - 8u];
    }
}

fn idx_to_slots(idx: u32, K: u32, slots: ptr<function, array<u32, 12>>) {
    var remaining = idx;
    for (var i = 0u; i < K; i++) {
        let radix = get_radix(i);
        (*slots)[i] = remaining % radix;
        remaining = remaining / radix;
    }
}

// ============================================================================
// RPN STACK EVALUATOR
// ============================================================================

fn evaluate_rpn(slots: ptr<function, array<u32, 12>>, K: u32) -> f32 {
    var stack: array<f32, 16>;
    var sp: u32 = 0u;
    
    for (var i = 0u; i < K; i++) {
        let t = ternary_form[i];
        let slot = (*slots)[i];
        
        if (t == 0u) {
            // Constant
            stack[sp] = get_constant(slot);
            sp = sp + 1u;
        } else if (t == 1u) {
            // Unary function
            if (sp >= 1u) {
                stack[sp - 1u] = apply_unary(slot, stack[sp - 1u]);
            }
        } else {
            // Binary function
            if (sp >= 2u) {
                sp = sp - 1u;
                stack[sp - 1u] = apply_binary(slot, stack[sp - 1u], stack[sp]);
            }
        }
    }
    
    if (sp == 1u) {
        return stack[0];
    }
    // Return a very large error value instead of NaN (WGSL doesn't allow NaN literals)
    return 1e38;
}

// ============================================================================
// MAIN COMPUTE KERNEL
// Each thread evaluates ONE RPN expression
// ============================================================================

@compute @workgroup_size(256)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let idx = global_id.x;
    
    // Bounds check
    if (idx >= params.total_combinations) {
        return;
    }
    
    // Real index including batch offset
    let real_idx = idx + params.batch_start;
    
    // Convert REAL index to instruction slots (not local idx)
    var slots: array<u32, 12>;
    idx_to_slots(real_idx, params.K, &slots);
    
    // Evaluate RPN expression
    let computed = evaluate_rpn(&slots, params.K);
    
    // Check for NaN/Inf
    if (isNan(computed) || isInf(computed)) {
        return;
    }
    
    // Calculate error
    let diff = abs(computed - params.search_target);
    
    // FP32 threshold filter - only record if error is below threshold
    if (diff < params.threshold) {
        // Atomically get write index
        let write_index = atomicAdd(&global_counter.count, 1u);
        
        // Only write if we haven't exceeded buffer capacity
        if (write_index < 1024u) {
            results[write_index].error = diff;
            results[write_index].idx = real_idx;
            results[write_index].valid = 1u;
        }
    }
}

// ============================================================================
// HELPER: Check if value is NaN
// ============================================================================

fn isNan(v: f32) -> bool {
    return !(v == v);
}

fn isInf(v: f32) -> bool {
    return abs(v) > 3.4e38;
}
