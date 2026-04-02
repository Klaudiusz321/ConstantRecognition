import { Precision } from './types';

// RPN Interpreter and conversion functions

// Named constants for display
export const namedConstants: Record<string, string> = {
  "NEG": "-1", "ZERO": "0", "ONE": "1", "TWO": "2", "THREE": "3",
  "FOUR": "4", "FIVE": "5", "SIX": "6", "SEVEN": "7", "EIGHT": "8",
  "NINE": "9", "POL": "½", "PI": "π", "EULER": "e", "GOLDENRATIO": "φ",
  "EULER_GAMMA": "γ"  // Euler-Mascheroni constant (not in WASM yet)
};

export const namedFunctions: Record<string, string> = {
  "EXP": "exp", "LOG": "ln", "INV": "inv", "MINUS": "minus",
  "SIN": "sin", "ARCSIN": "arcsin", "COS": "cos", "ARCCOS": "arccos",
  "TAN": "tan", "ARCTAN": "arctan", "SINH": "sinh", "ARCSINH": "arsinh",
  "COSH": "cosh", "ARCCOSH": "arcosh", "TANH": "tanh", "ARCTANH": "artanh",
  "SQRT": "sqrt", "SQR": "sqr", "GAMMA": "Γ"
};

export const namedOperators: Record<string, string> = {
  "PLUS": "+", "SUBTRACT": "-", "TIMES": "*", "DIVIDE": "/", "POWER": "^"
};

// Numerical constants for evaluation
export const numConstants: Record<string, number> = {
  "NEG": -1, "ZERO": 0, "ONE": 1, "TWO": 2, "THREE": 3, "FOUR": 4, "FIVE": 5,
  "SIX": 6, "SEVEN": 7, "EIGHT": 8, "NINE": 9, "POL": 0.5,
  "PI": Math.PI, "EULER": Math.E, "GOLDENRATIO": (1 + Math.sqrt(5)) / 2
};

export const numFunctions: Record<string, (x: number) => number> = {
  "EXP": Math.exp, "LOG": Math.log, "INV": x => 1/x, "MINUS": x => -x,
  "SIN": Math.sin, "ARCSIN": Math.asin, "COS": Math.cos, "ARCCOS": Math.acos,
  "TAN": Math.tan, "ARCTAN": Math.atan, "SINH": Math.sinh, "ARCSINH": Math.asinh,
  "COSH": Math.cosh, "ARCCOSH": Math.acosh, "TANH": Math.tanh, "ARCTANH": Math.atanh,
  "SQRT": Math.sqrt, "SQR": x => x*x, "GAMMA": x => gamma(x)
};

export const numOperators: Record<string, (a: number, b: number) => number> = {
  "PLUS": (a, b) => a + b, "SUBTRACT": (a, b) => a - b,
  "TIMES": (a, b) => a * b, "DIVIDE": (a, b) => a / b,
  "POWER": (a, b) => Math.pow(a, b)
};

// Gamma function approximation (Lanczos)
export function gamma(z: number): number {
  if (z < 0.5) return Math.PI / (Math.sin(Math.PI * z) * gamma(1 - z));
  z -= 1;
  const g = 7;
  const c = [0.99999999999980993, 676.5203681218851, -1259.1392167224028,
    771.32342877765313, -176.61502916214059, 12.507343278686905,
    -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7];
  let x = c[0];
  for (let i = 1; i < g + 2; i++) x += c[i] / (z + i);
  const t = z + g + 0.5;
  return Math.sqrt(2 * Math.PI) * Math.pow(t, z + 0.5) * Math.exp(-t) * x;
}

// Short-form RPN character mappings (from WebGPU)
// CONST_CHARS = '0123opqrstuvw' (13 chars)
const SHORT_CONST_MAP: Record<string, string> = {
  '0': 'PI', '1': 'EULER', '2': 'NEG', '3': 'GOLDENRATIO',
  'o': 'ONE', 'p': 'TWO', 'q': 'THREE', 'r': 'FOUR', 's': 'FIVE',
  't': 'SIX', 'u': 'SEVEN', 'v': 'EIGHT', 'w': 'NINE'
};

// UNARY_CHARS = '4589abcdefghijklmn' (18 chars)
const SHORT_UNARY_MAP: Record<string, string> = {
  '4': 'LOG', '5': 'EXP', '8': 'INV', '9': 'GAMMA',
  'a': 'SQRT', 'b': 'SQR', 'c': 'SIN', 'd': 'ARCSIN',
  'e': 'COS', 'f': 'ARCCOS', 'g': 'TAN', 'h': 'ARCTAN',
  'i': 'SINH', 'j': 'ARCSINH', 'k': 'COSH', 'l': 'ARCCOSH',
  'm': 'TANH', 'n': 'ARCTANH'
};

// BINARY_CHARS = '67xyz' (5 chars)
const SHORT_BINARY_MAP: Record<string, string> = {
  '6': 'PLUS', '7': 'TIMES', 'x': 'SUBTRACT', 'y': 'DIVIDE', 'z': 'POWER'
};

// All valid short-form characters
const ALL_SHORT_CHARS = new Set([
  ...Object.keys(SHORT_CONST_MAP),
  ...Object.keys(SHORT_UNARY_MAP),
  ...Object.keys(SHORT_BINARY_MAP)
]);

// Check if string is short-form RPN (from WebGPU)
function isShortFormRPN(rpn: string): boolean {
  // Short-form RPN has no delimiters and all chars are from the GPU charset
  if (rpn.includes(',') || rpn.includes(' ')) return false;
  // Must have at least one char and all chars must be valid short-form
  return rpn.length > 0 && [...rpn].every(c => ALL_SHORT_CHARS.has(c));
}

// Convert short-form RPN to long-form tokens
function expandShortRPN(rpn: string): string[] {
  const tokens: string[] = [];
  for (const char of rpn) {
    if (SHORT_CONST_MAP[char]) {
      tokens.push(SHORT_CONST_MAP[char]);
    } else if (SHORT_UNARY_MAP[char]) {
      tokens.push(SHORT_UNARY_MAP[char]);
    } else if (SHORT_BINARY_MAP[char]) {
      tokens.push(SHORT_BINARY_MAP[char]);
    }
  }
  return tokens;
}

// Convert RPN string to array
export function parseRPN(rpn: string): string[] {
  if (!rpn || rpn.length === 0) return [];
  
  // Check if this is short-form RPN from WebGPU (single chars, no delimiters)
  if (isShortFormRPN(rpn)) {
    return expandShortRPN(rpn);
  }
  
  // WASM returns format like "PI, EULER, PLUS" - comma and space separated
  if (rpn.includes(',')) {
    return rpn.split(',')
      .map(t => t.trim())
      .filter(t => t.length > 0);
  }
  // If contains spaces but no commas, split by space
  if (rpn.includes(' ')) {
    return rpn.split(' ').filter(t => t.length > 0);
  }
  // Single token (no delimiters) - return as single element array
  // This handles cases like "PI" which should be ["PI"], not ["P", "I"]
  return [rpn];
}

// Convert RPN to infix notation for display
export function rpnToInfix(rpn: string | string[]): string {
  // Detect if this is short-form (GPU) or long-form (WASM) RPN
  // Short-form uses standard RPN order, long-form uses swapped order
  const isShort = typeof rpn === 'string' && isShortFormRPN(rpn);
  const tokens = typeof rpn === 'string' ? parseRPN(rpn) : rpn;
  const stack: string[] = [];
  
  tokens.forEach(token => {
    if (namedConstants[token]) {
      stack.push(namedConstants[token]);
    } else if (namedFunctions[token]) {
      const arg = stack.pop() || '?';
      stack.push(`${namedFunctions[token]}(${arg})`);
    } else if (namedOperators[token]) {
      const right = stack.pop() || '?';  // top of stack
      const left = stack.pop() || '?';   // second from top
      if (isShort) {
        // Standard RPN: "a b op" means op(a, b)
        stack.push(`(${left} ${namedOperators[token]} ${right})`);
      } else {
        // WASM non-standard RPN: "a b op" means op(b, a)
        stack.push(`(${right} ${namedOperators[token]} ${left})`);
      }
    } else if (token) {
      // Unknown token - push as-is
      stack.push(token);
    }
  });
  
  return stack.pop() || rpn.toString();
}

// Evaluate RPN expression numerically
export function evaluateRPN(rpn: string | string[]): number {
  // Detect if this is short-form (GPU) or long-form (WASM) RPN
  const isShort = typeof rpn === 'string' && isShortFormRPN(rpn);
  const tokens = typeof rpn === 'string' ? parseRPN(rpn) : rpn;
  const stack: number[] = [];
  tokens.forEach(token => {
    if (numConstants[token] !== undefined) {
      stack.push(numConstants[token]);
    } else if (numFunctions[token]) {
      const arg = stack.pop() || 0;
      stack.push(numFunctions[token](arg));
    } else if (numOperators[token]) {
      const right = stack.pop() || 0;  // top
      const left = stack.pop() || 0;   // second
      if (isShort) {
        // Standard RPN: "a b op" means op(a, b)
        stack.push(numOperators[token](left, right));
      } else {
        // WASM non-standard RPN: "a b op" means op(b, a)
        stack.push(numOperators[token](right, left));
      }
    }
  });
  return stack.pop() || NaN;
}

// Extract precision info from input string
export function extractPrecision(inputString: string): Precision {
  const z = inputString;
  const parts = inputString.split(/e/i);
  const mainPart = parts[0];
  const exponent = parts.length > 1 ? parseInt(parts[1]) : 0;
  const decimalIndex = mainPart.indexOf('.');
  
  let deltaZ: number;
  if (decimalIndex === -1) {
    deltaZ = 0.5;
  } else {
    const fractionalPart = mainPart.slice(decimalIndex + 1);
    deltaZ = 0.5 * Math.pow(10, -fractionalPart.length + exponent);
  }
  
  const zNum = parseFloat(inputString);
  const relDeltaZ = zNum !== 0 ? deltaZ / Math.abs(zNum) : 0;
  
  return {
    z,
    deltaZ: deltaZ.toExponential(2),
    relDeltaZ: relDeltaZ.toExponential(2)
  };
}

// Convert RPN to Mathematica syntax
export function rpnToMathematica(rpn: string | string[]): string {
  // Detect if this is short-form (GPU) or long-form (WASM) RPN
  const isShort = typeof rpn === 'string' && isShortFormRPN(rpn);
  const tokens = typeof rpn === 'string' ? parseRPN(rpn) : rpn;
  const mmaConstants: Record<string, string> = {
    "NEG": "(-1)", "ZERO": "0", "ONE": "1", "TWO": "2", "THREE": "3",
    "FOUR": "4", "FIVE": "5", "SIX": "6", "SEVEN": "7", "EIGHT": "8",
    "NINE": "9", "PI": "Pi", "EULER": "E", "GOLDENRATIO": "GoldenRatio",
    "EULER_GAMMA": "EulerGamma", "POL": "(1/2)"
  };
  const mmaFunctions: Record<string, string> = {
    "EXP": "Exp", "LOG": "Log", "SIN": "Sin", "ARCSIN": "ArcSin",
    "COS": "Cos", "ARCCOS": "ArcCos", "TAN": "Tan", "ARCTAN": "ArcTan",
    "SINH": "Sinh", "ARCSINH": "ArcSinh", "COSH": "Cosh", "ARCCOSH": "ArcCosh",
    "TANH": "Tanh", "ARCTANH": "ArcTanh", "SQRT": "Sqrt", "GAMMA": "Gamma",
    "MINUS": "Minus"
  };
  const mmaUnnamed: Record<string, (x: string) => string> = {
    "SQR": x => `(${x})^2`,
    "INV": x => `1/(${x})`,
    "MINUS": x => `(-${x})`
  };
  const mmaOperators: Record<string, string> = {
    "PLUS": "+", "SUBTRACT": "-", "TIMES": "*", "DIVIDE": "/", "POWER": "^"
  };

  const stack: string[] = [];
  tokens.forEach(token => {
    if (mmaConstants[token]) {
      stack.push(mmaConstants[token]);
    } else if (mmaUnnamed[token]) {
      const arg = stack.pop() || '?';
      stack.push(mmaUnnamed[token](arg));
    } else if (mmaFunctions[token]) {
      const arg = stack.pop() || '?';
      stack.push(`${mmaFunctions[token]}[${arg}]`);
    } else if (mmaOperators[token]) {
      const right = stack.pop() || '?';  // top
      const left = stack.pop() || '?';   // second
      if (isShort) {
        // Standard RPN: "a b op" means op(a, b)
        stack.push(`(${left} ${mmaOperators[token]} ${right})`);
      } else {
        // WASM non-standard RPN: "a b op" means op(b, a)
        stack.push(`(${right} ${mmaOperators[token]} ${left})`);
      }
    } else if (token) {
      // Unknown token - push as-is
      stack.push(token);
    }
  });
  return stack.pop() || rpn.toString();
}

// Create Wolfram Alpha link
export function createWolframLink(formula: string): string {
  return `https://www.wolframalpha.com/input?i=${encodeURIComponent(formula)}`;
}

// Convert RPN to LaTeX syntax for beautiful rendering
export function rpnToLatex(rpn: string | string[]): string {
  // Detect if this is short-form (GPU) or long-form (WASM) RPN
  const isShort = typeof rpn === 'string' && isShortFormRPN(rpn);
  const tokens = typeof rpn === 'string' ? parseRPN(rpn) : rpn;
  
  type LatexNode = { latex: string; precedence: number };
  const wrapWithParens = (node: LatexNode, minPrecedence: number) => {
    if (node.precedence < minPrecedence) {
      return `\\left(${node.latex}\\right)`;
    }
    return node.latex;
  };

  const latexConstants: Record<string, string> = {
    "NEG": "(-1)", "ZERO": "0", "ONE": "1", "TWO": "2", "THREE": "3",
    "FOUR": "4", "FIVE": "5", "SIX": "6", "SEVEN": "7", "EIGHT": "8",
    "NINE": "9", "PI": "\\pi", "EULER": "e", "GOLDENRATIO": "\\varphi",
    "EULER_GAMMA": "\\gamma"
  };
  
  const latexFunctions: Record<string, (x: string) => string> = {
    "EXP": x => `e^{${x}}`,
    "LOG": x => `\\ln(${x})`,
    "SIN": x => `\\sin(${x})`,
    "ARCSIN": x => `\\arcsin(${x})`,
    "COS": x => `\\cos(${x})`,
    "ARCCOS": x => `\\arccos(${x})`,
    "TAN": x => `\\tan(${x})`,
    "ARCTAN": x => `\\arctan(${x})`,
    "SINH": x => `\\sinh(${x})`,
    "ARCSINH": x => `\\text{arsinh}(${x})`,
    "COSH": x => `\\cosh(${x})`,
    "ARCCOSH": x => `\\text{arcosh}(${x})`,
    "TANH": x => `\\tanh(${x})`,
    "ARCTANH": x => `\\text{artanh}(${x})`,
    "SQRT": x => `\\sqrt{${x}}`,
    "SQR": x => `(${x})^2`,
    "GAMMA": x => `\\Gamma(${x})`,
    "INV": x => `\\frac{1}{${x}}`,
    "MINUS": x => `(-${x})`
  };
  
  const latexOperators = new Set(["PLUS", "SUBTRACT", "TIMES", "DIVIDE", "POWER"]);

  const stack: LatexNode[] = [];
  tokens.forEach(token => {
    if (latexConstants[token]) {
      stack.push({ latex: latexConstants[token], precedence: 5 });
    } else if (latexFunctions[token]) {
      const arg = stack.pop() || { latex: '?', precedence: 5 };
      stack.push({ latex: latexFunctions[token](arg.latex), precedence: 4 });
    } else if (latexOperators.has(token)) {
      const right = stack.pop() || { latex: '?', precedence: 5 };  // top
      const left = stack.pop() || { latex: '?', precedence: 5 };   // second
      const lhs = isShort ? left : right;
      const rhs = isShort ? right : left;

      if (token === 'PLUS') {
        const leftLatex = wrapWithParens(lhs, 1);
        const rightLatex = wrapWithParens(rhs, 1);
        stack.push({ latex: `${leftLatex} + ${rightLatex}`, precedence: 1 });
        return;
      }
      if (token === 'SUBTRACT') {
        const leftLatex = wrapWithParens(lhs, 1);
        const rightLatex = wrapWithParens(rhs, 2);
        stack.push({ latex: `${leftLatex} - ${rightLatex}`, precedence: 1 });
        return;
      }
      if (token === 'TIMES') {
        const leftLatex = wrapWithParens(lhs, 2);
        const rightLatex = wrapWithParens(rhs, 2);
        stack.push({ latex: `${leftLatex} \\cdot ${rightLatex}`, precedence: 2 });
        return;
      }
      if (token === 'DIVIDE') {
        const leftLatex = wrapWithParens(lhs, 2);
        const rightLatex = wrapWithParens(rhs, 2);
        stack.push({ latex: `\\frac{${leftLatex}}{${rightLatex}}`, precedence: 2 });
        return;
      }
      if (token === 'POWER') {
        const leftLatex = wrapWithParens(lhs, 3);
        const rightLatex = wrapWithParens(rhs, 4);
        stack.push({ latex: `{${leftLatex}}^{${rightLatex}}`, precedence: 3 });
        return;
      }
    } else if (token) {
      stack.push({ latex: token, precedence: 5 });
    }
  });
  return stack.pop()?.latex || rpn.toString();
}
