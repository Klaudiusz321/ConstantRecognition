const { FORTHToMathematica } = require('./FORTH_to_Mma.js');


function extractAndConvertEquations(riesOutput) {
    const lines = riesOutput.split('\n');
    const equations = [];
    let startParsing = false;

    for (const line of lines) {
        if (line.includes('Your target value:')) {
            startParsing = true;
            continue;
        }
        if (line.includes('(Stopping now because')) {
            break;
        }
        if (startParsing && line.trim() !== '') {
            const match = line.trim().match(/^\s*(.+?)\s*=\s*(.+?)\s*(?:(?:for x = T ([-+]) (\S+)|(?:\('exact' match\))))\s*{(\d+)}$/);
            if (match) {
                const lhs = match[1].trim();
                const rhs = match[2].trim();
                const delta = match[3] && match[4] ? parseFloat((match[3] === '-' ? '-' : '') + match[4]) : 0.0;
                const complexity = parseInt(match[5]);

                const mmaEquation = `${FORTHToMathematica(lhs)} == ${FORTHToMathematica(rhs)}`;

                equations.push({
                    forth: {
                        lhs: lhs,
                        rhs: rhs
                    },
                    mathematica: mmaEquation,
                    DELTA: delta,
                    complexity: complexity
                });
            }
        }
    }

    return equations;
}

if (typeof process !== 'undefined') {
    if (process.argv.length > 2) {
        const riesOutput = process.argv.slice(2).join('\n');
        const equations = extractAndConvertEquations(riesOutput);
        
        console.log(JSON.stringify(equations, null, 2));
    } else {
        console.log("Please provide RIES output as arguments.");
    }
}