// First, let's define numerical values for our constants
export const namedConstants = {
    "NEG": -1,
    "ZERO": 0,
    "ONE": 1,
    "TWO": 2,
    "THREE": 3,
    "FOUR": 4,
    "FIVE": 5,
    "SIX": 6,
    "SEVEN": 7,
    "EIGHT": 8,
    "NINE": 9,
    "POL": 0.5,
    "PI": Math.PI,
    "EULER": Math.E,
    "GOLDENRATIO": (1 + Math.sqrt(5)) / 2
};

// Define our functions
export const namedFunctions = {
    "EXP": Math.exp,
    "LOG": Math.log,
    "INV": x => 1 / x,
    "MINUS": x => -x,
    "SIN": Math.sin,
    "ARCSIN": Math.asin,
    "COS": Math.cos,
    "ARCCOS": Math.acos,
    "TAN": Math.tan,
    "ARCTAN": Math.atan,
    "SINH": Math.sinh,
    "ARCSINH": Math.asinh,
    "COSH": Math.cosh,
    "ARCCOSH": Math.acosh,
    "TANH": Math.tanh,
    "ARCTANH": Math.atanh,
    "SQRT": Math.sqrt,
    "SQR": x => x * x,
    "GAMMA": math.gamma
};

// Define our operators
export const namedOperators = {
    "PLUS": (a, b) => a + b,
    "SUBTRACT": (a, b) => a - b,  // Note the order: a - b
    "TIMES": (a, b) => a * b,
    "DIVIDE": (a, b) => a / b,    // Note the order: a / b
    "POWER": (a, b) => Math.pow(a, b)  // Note the order: a^b
};

export function evaluateRPN(rpn) {
    let stack = [];

    rpn.forEach(token => {
        if (namedConstants.hasOwnProperty(token)) {
            stack.push(namedConstants[token]);
        } else if (namedFunctions.hasOwnProperty(token)) {
            let arg = stack.pop();
            stack.push(namedFunctions[token](arg));
        } else if (namedOperators.hasOwnProperty(token)) {
            let a = stack.pop();
            let b = stack.pop();
            stack.push(namedOperators[token](a, b));
        } else {
            // If it's neither, handle it as an error or special case
            throw new Error("Unrecognized token: " + token);
        }
    });

    return stack.pop();
}

// Example usage:
// let rpnExpression = ["GOLDENRATIO", "PI", "PLUS", "EXP"];
// let result = evaluateRPN(rpnExpression);
// console.log(result);  // Should output the numerical result