export const namedConstants = {
 "NEG"   : "-1",
 "ZERO"  : "0",
 "ONE"   : "1",
 "TWO"   : "2",
 "THREE" : "3",
 "FOUR"  : "4",
 "FIVE"  : "5",
 "SIX"   : "6",
 "SEVEN" : "7",
 "EIGHT" : "8",
 "NINE"  : "9",
 "POL"   : "½",
 "PI"    : "π",
 "EULER" : "e",
 "GOLDENRATIO": "φ"
}


export const namedFunctions = {
"EXP"    : "exp", 
"LOG"    : "ln", 
"INV"    : "inv",
"MINUS"  : "minus",
"SIN"    : "sin", 
"ARCSIN" : "arcsin",
"COS"    : "cos", 
"ARCCOS" : "arccos",
"TAN"    : "tan", 
"ARCTAN" : "arctan",
"SINH"   : "sinh",
"ARCSINH": "arsinh",
"COSH"   : "cosh",
"ARCCOSH": "arcosh",
"TANH"   : "tanh",
"ARCTANH": "artanh",
"SQRT"   : "sqrt",
"SQR"    : "sqr",
"GAMMA"  : "Γ"  // Using the actual Greek letter Γ
}

export const namedOperators = {
    "PLUS": "+",
    "SUBTRACT": "-",
    "TIMES": "*",
    "DIVIDE": "/",
    "POWER":  "^"
};

export function rpnToInfix(rpn) {
    let stack = [];

    rpn.forEach(token => {
        if (namedConstants.hasOwnProperty(token)) {
            stack.push(namedConstants[token].toString());
        } else if (namedFunctions.hasOwnProperty(token)) {
            let argument = stack.pop();
            stack.push(namedFunctions[token] + "(" + argument + ")");
        } else if (namedOperators.hasOwnProperty(token)) {
            let left = stack.pop();
            let right = stack.pop();
            stack.push("(" + left + " " + namedOperators[token] + " " + right + ")");
        } else {
            // If it's neither, handle it as an error or special case
            throw new Error("Unrecognized token: " + token);
        }
    });

    return stack.pop();
}

export function removeRedundantParentheses(expression) {
    // Regular expression to match redundant parentheses around simple expressions
    // This is a simple example and might not cover all cases
    const redundantParenthesesRegex = /\(\(([^\(\)]+)\)\)/g;
    
    let newExpression = expression;
    let prevExpression;
    
    do {
        prevExpression = newExpression;
        // Replace redundant parentheses with single ones
        newExpression = newExpression.replace(redundantParenthesesRegex, '($1)');
    } while (newExpression !== prevExpression); // Repeat if changes were made
    
    return newExpression;
}


//export function removeOutermostParentheses(expression) {
//    // Regular expression to match an expression enclosed in outermost parentheses
//    const outermostParenthesesRegex = /^\((.*)\)$/;
//
//    let match = outermostParenthesesRegex.exec(expression);
//    if (match) {
//        return match[1]; // Return the inner expression
//    }
//    return expression; // Return the original expression unchanged if no outermost parentheses
//}



export function removeOutermostParentheses(expression) {
    if (expression.startsWith('(') && expression.endsWith(')')) {
        // Check if the parentheses are actually matching
        let count = 0;
        for (let i = 0; i < expression.length; i++) {
            if (expression[i] === '(') count++;
            if (expression[i] === ')') count--;
            if (count === 0 && i < expression.length - 1) {
                // If we reach 0 before the end, these are not the outermost parentheses
                return expression;
            }
        }
        // If we get here, the outermost parentheses match and can be removed
        return expression.slice(1, -1);
    }
    return expression;
}

export function replaceGammaWithFactorial(formula) {
    return formula.replace(/Γ\((\d+)\)/g, (match, number) => {
        const n = parseInt(number, 10);
        if (n > 1) {
            return `${n-1}!`;  // Removed extra parentheses
        }
        return match;  // Return unchanged for Γ(1) or invalid inputs
    });
}

// Example usage:
//let rpnExpression = "GOLDENRATIO, PI, PLUS, EXP".split(", ")
//let rpnExpression = ["GOLDENRATIO", "PI", "PLUS", "EXP"];
//let infixExpression = rpnToInfix(rpnExpression); // Assuming this is your function converting RPN to infix
//let simplifiedExpression = removeRedundantParentheses(infixExpression);
//
//console.log(simplifiedExpression); // Should output a version with fewer redundant parentheses


