// RPN Interpreter functions and constants

export const namedConstants: Record<string, string> = {
  "NEG": "-1", "ZERO": "0", "ONE": "1", "TWO": "2", "THREE": "3",
  "FOUR": "4", "FIVE": "5", "SIX": "6", "SEVEN": "7", "EIGHT": "8",
  "NINE": "9", "POL": "½", "PI": "π", "EULER": "e", "GOLDENRATIO": "φ"
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

// Numerical evaluation constants
export const numConstants: Record<string, number> = {
  "NEG": -1, "ZERO": 0, "ONE": 1, "TWO": 2, "THREE": 3, "FOUR": 4, "FIVE": 5,
  "SIX": 6, "SEVEN": 7, "EIGHT": 8, "NINE": 9, "POL": 0.5,
  "PI": Math.PI, "EULER": Math.E, "GOLDENRATIO": (1 + Math.sqrt(5)) / 2
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

export function rpnToInfix(rpn: string[]): string {
  const stack: string[] = [];
  rpn.forEach(token => {
    if (namedConstants[token]) {
      stack.push(namedConstants[token]);
    } else if (namedFunctions[token]) {
      const arg = stack.pop() || '';
      stack.push(`${namedFunctions[token]}(${arg})`);
    } else if (namedOperators[token]) {
      const left = stack.pop() || '';
      const right = stack.pop() || '';
      stack.push(`(${left} ${namedOperators[token]} ${right})`);
    }
  });
  return stack.pop() || '';
}

export function evaluateRPN(rpn: string[]): number {
  const stack: number[] = [];
  rpn.forEach(token => {
    if (numConstants[token] !== undefined) {
      stack.push(numConstants[token]);
    } else if (numFunctions[token]) {
      const arg = stack.pop() || 0;
      stack.push(numFunctions[token](arg));
    } else if (numOperators[token]) {
      const a = stack.pop() || 0;
      const b = stack.pop() || 0;
      stack.push(numOperators[token](a, b));
    }
  });
  return stack.pop() || NaN;
}

export function extractPrecision(inputString: string): number {
  const parts = inputString.split(/e/i);
  const mainPart = parts[0];
  const exponent = parts.length > 1 ? parseInt(parts[1]) : 0;
  const decimalIndex = mainPart.indexOf('.');
  if (decimalIndex === -1) return 0;
  const fractionalPart = mainPart.slice(decimalIndex + 1);
  return 0.5 * Math.pow(10, -fractionalPart.length + exponent);
}

// Mathematica conversion
export function rpnToMathematica(rpn: string[]): string {
  const mmaConstants: Record<string, string> = {
    "NEG": "(-1)", "ZERO": "0", "ONE": "1", "TWO": "2", "THREE": "3",
    "FOUR": "4", "FIVE": "5", "SIX": "6", "SEVEN": "7", "EIGHT": "8",
    "NINE": "9", "PI": "Pi", "EULER": "E", "GOLDENRATIO": "GoldenRatio"
  };
  const mmaFunctions: Record<string, string> = {
    "EXP": "Exp", "LOG": "Log", "SIN": "Sin", "ARCSIN": "ArcSin",
    "COS": "Cos", "ARCCOS": "ArcCos", "TAN": "Tan", "ARCTAN": "ArcTan",
    "SINH": "Sinh", "ARCSINH": "ArcSinh", "COSH": "Cosh", "ARCCOSH": "ArcCosh",
    "TANH": "Tanh", "ARCTANH": "ArcTanh", "SQRT": "Sqrt", "GAMMA": "Gamma"
  };
  const mmaUnnamed: Record<string, (x: string) => string> = {
    "SQR": x => `(${x})^2`,
    "INV": x => `1/(${x})`
  };
  const mmaOperators: Record<string, string> = {
    "PLUS": "+", "SUBTRACT": "-", "TIMES": "*", "DIVIDE": "/", "POWER": "^"
  };

  const stack: string[] = [];
  rpn.forEach(token => {
    if (mmaConstants[token]) {
      stack.push(mmaConstants[token]);
    } else if (mmaFunctions[token]) {
      const arg = stack.pop() || '';
      stack.push(`${mmaFunctions[token]}[${arg}]`);
    } else if (mmaUnnamed[token]) {
      const arg = stack.pop() || '';
      stack.push(mmaUnnamed[token](arg));
    } else if (mmaOperators[token]) {
      const left = stack.pop() || '';
      const right = stack.pop() || '';
      stack.push(`((${left}) ${mmaOperators[token]} (${right}))`);
    }
  });
  return stack.pop() || '';
}

export function createWolframLink(formula: string): string {
  return `https://www.wolframalpha.com/input?i=${encodeURIComponent(formula)}`;
}
