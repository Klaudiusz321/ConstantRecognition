export const namedConstantsMma = {
 "NEG"   : "(-1)", //PreDecrement::rvalue: 1 is not a variable with a value, so its value cannot be changed.
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
 "POL"   : "1/2",
 "PI"    : "Pi",
 "EULER" : "E",
 "GOLDENRATIO": "GoldenRatio",
 "EULERGAMMA": "EulerGamma",
 "TWOZEROTWOFOUR": "2024"
}


export const namedFunctionsMma = {
"EXP"    : "Exp", 
"LOG"    : "Log", 
"SIN"    : "Sin", 
"ARCSIN" : "ArcSin",
"COS"    : "Cos", 
"ARCCOS" : "ArcCos",
"TAN"    : "Tan", 
"ARCTAN" : "ArcTan",
"SINH"   : "Sinh",
"ARCSINH": "ArcSinh",
"COSH"   : "Cosh",
"ARCCOSH": "ArcCosh",
"TANH"   : "Tanh",
"ARCTANH": "ArcTanh",
"SQRT"   : "Sqrt",
"GAMMA"  : "Gamma"
}

// Define a mapping for unnamed functions to Mathematica equivalents or expressions
export const unnamedFunctionsMma = {
    //"SQR": (x) => "Power[" + x + ", 2]",  // Converts to Power[x,2] which is x^2
    "SQR":   (x) => "(" + x + ")^2",  // Converts directly to(x)^2
    //"INV": (x) => "Power[" + x + ", -1]"  // Converts to Power[x,-1] which is 1/x
    "INV":   (x) => "1/(" + x + ")",  // Converts directly to 1/x
    "MINUS": (x) => "(-" + x + ")", // Converts directly to -x
    "HALF":  (x) => "(" + x + "/2)",
    "DBL":   (x) => "(2*" + x + ")",
    "PRE":   (x) => "(" + x + "-1)",
    "SUC":   (x) => "(" + x + "+1)",
    "TET":   (x) => "(" + x + ")^(" + x + ")"
};

export const namedOperatorsMma = {
    "PLUS": "+",
    "SUBTRACT": "-",
    "TIMES": "*",
    "DIVIDE": "/",
    "POWER":  "^",
    "LOGARITHM": "~Log~",
    "EML": "~EmL~"
};

export function rpnToMma(rpn) {
    let stack = [];

    rpn.forEach(token => {
        if (namedConstantsMma.hasOwnProperty(token)) {
            stack.push(namedConstantsMma[token].toString());
        } else if (namedFunctionsMma.hasOwnProperty(token)) {
            let argument = stack.pop();
            stack.push(namedFunctionsMma[token] + "[" + argument + "]");        
        } else if (unnamedFunctionsMma.hasOwnProperty(token)) {
            let argument = stack.pop();
            stack.push(unnamedFunctionsMma[token](argument));
        } else if (namedOperatorsMma.hasOwnProperty(token)) {
            let left = stack.pop();
            let right = stack.pop();
            stack.push("((" + left + ") " + namedOperatorsMma[token] + " (" + right + "))");
        } else {
            // If it's neither, handle it as an error or special case
            throw new Error("Unrecognized token: " + token);
        }
    });

    return stack.pop();
}


if (typeof process !== 'undefined') {


  if (process.argv.length > 2) {
      // Skip the first two elements and pass the rest as the RPN array
      const rpnArray = process.argv.slice(2);
      console.log(rpnToMma(rpnArray));
  } else {
      console.log("Please provide an RPN expression as arguments.");
  }

}
