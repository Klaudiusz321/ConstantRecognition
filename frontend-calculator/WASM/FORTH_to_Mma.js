function FORTHToMathematica(rpnExpression) {
    const stack = [];
    const tokens = rpnExpression.split(' ');
    
    const unaryOperators = {
        'neg': 'Minus',
        'recip': 'Power[#, -1]&',
        'dup*': 'Power[#, 2]&',
        'sqrt': 'Sqrt',
        'ln': 'Log',
        'exp': 'Exp',
        'sin': 'Sin',
        'cos': 'Cos',
        'tan': 'Tan',
        'sinpi': 'Sin[Pi #]&',
        'cospi': 'Cos[Pi #]&',
        'tanpi': 'Tan[Pi #]&',
        'W': 'ProductLog'
    };

    const binaryOperators = {
        '+': 'Plus',
        '-': 'Subtract',
        '*': 'Times',
        '/': 'Divide',
        '**': 'Power',
        'root': 'Power[#2, 1/#1]&',
        'logN': 'Log[#2, #1]&',
        'atan2': 'ArcTan'
    };

    const constants = {
        'pi': 'Pi',
        'e': 'E',
        'phi': 'GoldenRatio'
    };

    for (let token of tokens) {
        if (token in unaryOperators) {
            const a = stack.pop();
            if (unaryOperators[token].includes('#')) {
                stack.push(`(${unaryOperators[token]})[${a}]`);
            } else {
                stack.push(`${unaryOperators[token]}[${a}]`);
            }
        } else if (token in binaryOperators) {
            const b = stack.pop();
            const a = stack.pop();
            if (binaryOperators[token].includes('#')) {
                stack.push(`(${binaryOperators[token]})[${a}, ${b}]`);
            } else {
                stack.push(`${binaryOperators[token]}[${a}, ${b}]`);
            }
        } else if (token in constants) {
            stack.push(constants[token]);
        } else {
            // Assume it's a number or variable
            stack.push(token);
        }
    }

    return stack.pop();
}

module.exports = { FORTHToMathematica };


if (typeof process !== 'undefined') {


  if (process.argv.length > 2) {
      // Skip the first two elements and pass the rest as the RPN array
      const rpnArray = process.argv.slice(2).join(' ');
      //console.log(rpnArray);
      //console.log(FORTHToMathematica(rpnArray));
  } else {
      console.log("Please provide an RPN expression as arguments.");
  }

}