/**
 * RPN Evaluator for short-form RPN codes
 * Provides FP64 (double precision) evaluation on CPU
 */

// Number of opcodes per type
export const N_CONST = 13;
export const N_UNARY = 18;
export const N_BINARY = 5;

// Character mappings for short-form RPN
export const CONST_CHARS = '0123opqrstuvw';
export const UNARY_CHARS = '4589abcdefghijklmn';
export const BINARY_CHARS = '67xyz';

// FP64 constants for CPU verification (matching shader's get_constant)
// Index mapping:
// 0=π, 1=e, 2=-1, 3=φ (golden ratio), 4-12=1-9
const FP64_CONSTANTS: number[] = [
  Math.PI,                      // 0 = π
  Math.E,                       // 1 = e
  -1,                           // 2 = -1
  (1 + Math.sqrt(5)) / 2,       // 3 = φ (golden ratio)
  1, 2, 3, 4, 5, 6, 7, 8, 9     // 4-12 = 1-9
];

// FP64 unary functions for CPU verification (matching shader's apply_unary)
// Index mapping:
// 0=ln, 1=exp, 2=1/x, 3=gamma, 4=sqrt, 5=sqr,
// 6=sin, 7=asin, 8=cos, 9=acos, 10=tan, 11=atan,
// 12=sinh, 13=asinh, 14=cosh, 15=acosh, 16=tanh, 17=atanh
const FP64_UNARY: ((x: number) => number)[] = [
  Math.log,           // 0 = ln
  Math.exp,           // 1 = exp
  x => 1 / x,         // 2 = reciprocal
  x => x,             // 3 = gamma placeholder (not implemented properly)
  Math.sqrt,          // 4 = sqrt
  x => x * x,         // 5 = square
  Math.sin,           // 6 = sin
  Math.asin,          // 7 = asin
  Math.cos,           // 8 = cos
  Math.acos,          // 9 = acos
  Math.tan,           // 10 = tan
  Math.atan,          // 11 = atan
  Math.sinh,          // 12 = sinh
  Math.asinh,         // 13 = asinh
  Math.cosh,          // 14 = cosh
  Math.acosh,         // 15 = acosh
  Math.tanh,          // 16 = tanh
  Math.atanh          // 17 = atanh
];

// FP64 binary functions for CPU verification (matching shader's apply_binary)
// Index mapping: 0=add, 1=multiply, 2=subtract, 3=divide, 4=power
const FP64_BINARY: ((a: number, b: number) => number)[] = [
  (a, b) => a + b,    // 0 = add
  (a, b) => a * b,    // 1 = multiply
  (a, b) => a - b,    // 2 = subtract
  (a, b) => a / b,    // 3 = divide
  Math.pow            // 4 = power
];

/**
 * Evaluate short-form RPN code in FP64 (double precision)
 * Used for CPU verification of GPU candidates
 * 
 * @param rpn - Short-form RPN string (e.g., "3" for φ, "35" for exp(φ))
 * @returns Computed value, or NaN if invalid
 */
export function evaluateShortRPN(rpn: string): number {
  const stack: number[] = [];
  
  for (const char of rpn) {
    const constIdx = CONST_CHARS.indexOf(char);
    const unaryIdx = UNARY_CHARS.indexOf(char);
    const binaryIdx = BINARY_CHARS.indexOf(char);
    
    if (constIdx >= 0) {
      stack.push(FP64_CONSTANTS[constIdx]);
    } else if (unaryIdx >= 0) {
      if (stack.length < 1) return NaN;
      const x = stack.pop()!;
      stack.push(FP64_UNARY[unaryIdx](x));
    } else if (binaryIdx >= 0) {
      if (stack.length < 2) return NaN;
      const b = stack.pop()!;
      const a = stack.pop()!;
      stack.push(FP64_BINARY[binaryIdx](a, b));
    }
  }
  
  return stack.length === 1 ? stack[0] : NaN;
}

/**
 * Convert combination index to short-form RPN string
 * 
 * @param idx - Combination index
 * @param form - Ternary form array (0=const, 1=unary, 2=binary)
 * @param radix - Radix for each position
 * @param K - Formula length
 * @returns Short-form RPN string
 */
export function indexToRPN(idx: number, form: number[], radix: number[], K: number): string {
  let result = '';
  let remaining = idx;

  for (let i = 0; i < K; i++) {
    const slot = remaining % radix[i];
    remaining = Math.floor(remaining / radix[i]);

    switch (form[i]) {
      case 0: result += CONST_CHARS[slot]; break;
      case 1: result += UNARY_CHARS[slot]; break;
      case 2: result += BINARY_CHARS[slot]; break;
    }
  }

  return result;
}
