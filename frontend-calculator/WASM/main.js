import * as Interpreter from './RPN_interpreter.js';
import * as Evaluator from './RPN_evaluator.js';
import * as Mma from './RPN_to_Mma_interpreter.mjs';

let Module;

async function initializeModule() {
    Module = await window.moduleReadyPromise;
    console.log("Module initialized in main.js");
}

async function calculate() {
    try {
        if (!Module) {
            await initializeModule();
        }
        
        const z = document.getElementById('numberInput').value;
        const MaxCodeLength = parseInt(document.getElementById('searchDepthValue').textContent);

        // Capture start time
        const startTime = new Date();

        // Your calculation logic here
        const result = JSON.parse(Module.ccall('search_RPN', 'string', 
                                   ['number', 'number', 'number', 'number','number', 'number'], 
                                   [z, 0, 1, MaxCodeLength, 0, 1]));
        const rpnCode = result.RPN;

        // Capture end time
        const endTime = new Date();

        // Calculate the difference in milliseconds
        const timeTaken = endTime - startTime;

        console.log(rpnCode);

        // Update the UI with the result
        document.getElementById('resultInfix').value = Interpreter.removeRedundantParentheses(Interpreter.rpnToInfix(rpnCode.split(', ')));
        document.getElementById('resultRPN').textContent = rpnCode;
        document.getElementById('resultMathematica').textContent = Mma.rpnToMma(rpnCode.split(", ")) || "";
        document.getElementById('timing').textContent = `${timeTaken} ms`;
        document.getElementById('resultNumeric').value = Evaluator.evaluateRPN(rpnCode.split(", "));
    } catch (error) {
        console.error("Error in calculate function:", error);
        alert("An error occurred while performing the calculation. Please try again.");
    }
}

function updateSearchDepthValue(value) {
    document.getElementById('searchDepthValue').textContent = value;
}

// Set up event listeners
function setupEventListeners() {
    const calculateButton = document.getElementById('calculateButton');
    if (calculateButton) {
        calculateButton.addEventListener('click', calculate);
    } else {
        console.error("Calculate button not found");
    }

    const slider = document.getElementById('searchDepth');
    if (slider) {
        slider.addEventListener('input', (event) => updateSearchDepthValue(event.target.value));
    } else {
        console.error("Search depth slider not found");
    }
}

// Initialize everything when the DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    setupEventListeners();
    initializeModule();
});