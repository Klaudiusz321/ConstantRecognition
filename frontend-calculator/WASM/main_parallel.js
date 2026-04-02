import * as Interpreter from './RPN_interpreter.js';
import * as Evaluator from './RPN_evaluator.js';
import * as Mma from './RPN_to_Mma_interpreter.mjs';


let Module;
let workers = [];
let activeWorkers = 0;
let inputRelativePrecision;
window.calculationComplete = false;
let userThreadCount; // Variable to store user-selected thread count

async function initializeModule() {
    Module = await window.moduleReadyPromise;
    console.log("Module initialized in main_parallel.js");
}

function updateSearchDepthValue(value) {
    document.getElementById('searchDepthValue').textContent = value;
}

function updateThreadCountValue(value) {
    document.getElementById('threadCountValue').textContent = value;
}

function extractPrecision(inputString) {
    // Parse the input string to a number
    let value = parseFloat(inputString);

    // Handle scientific notation
    let parts = inputString.split(/e/i);
    let mainPart = parts[0];
    let exponent = parts.length > 1 ? parseInt(parts[1]) : 0;

    // Find the decimal point
    let decimalIndex = mainPart.indexOf('.');

    let absolutePrecision;
    if (decimalIndex === -1) {
        // If there's no decimal point, it's an integer
        absolutePrecision = 0; // Integers have exact precision
    } else {
        // Count the number of digits after the decimal point, including trailing zeros
        let fractionalPart = mainPart.slice(decimalIndex + 1);
        let significantDigits = fractionalPart.length;

        // Calculate absolute precision
        absolutePrecision = 0.5 * Math.pow(10, -significantDigits + exponent);
    }

    return absolutePrecision;
}



async function calculate() {
    try {
        //window.calculationComplete = false; 
        if (!Module) {
            await initializeModule();
        }
        
        const inputElement = document.getElementById('numberInput');
        const inputValue = inputElement.value;
        const z = parseFloat(inputValue);
        const inputPrecision = extractPrecision(inputValue);
        //const inputRelativePrecision;
        const MinCodeLength = 1;
        const MaxCodeLength = parseInt(document.getElementById('searchDepthValue').textContent);
        
        // Use user-selected thread count or auto-detected value
        const isAutoThreads = document.getElementById('autoThreads').checked;
        const ncpus = isAutoThreads ? 
            (navigator.hardwareConcurrency || 7) : 
            parseInt(document.getElementById('threadCount').value);
            
        //const ncpus = 2; // For debug

        // Extract precision from input
        console.log("Input element:", inputElement);
        console.log("Input value:", inputValue);
        console.log("z=", z);
        //inputPrecision = extractPrecision(inputValue);
        inputRelativePrecision = inputPrecision/Math.abs(z);
        console.log("Δz=", inputPrecision);
        console.log("Δz/z=", inputRelativePrecision);
        console.log("Using " + ncpus + " threads");

        document.getElementById('z').textContent = z.toString();
        document.getElementById('delta_z').textContent = inputPrecision.toString();
        document.getElementById('rel_delta_z').textContent = inputRelativePrecision.toString();
        document.getElementById('abs_delta_z').textContent = inputPrecision.toString();
        


        // Clear previous results
        clearResultsTable();
        clearResults();

        // Capture start time
        const startTime = new Date();

        // Create and start workers
        workers = [];
        activeWorkers = ncpus;

        // --- create and start workers ---
        workers = [];
        activeWorkers = ncpus;
        
        for (let i = 0; i < ncpus; i++) {
            const worker = new Worker('worker.js');
        
            // wait until we get the ready ping ---------------------------------
            worker.addEventListener('message', function readyListener(e) {
                if (e.data && e.data.type === 'ready') {
                    worker.removeEventListener('message', readyListener);
        
                    // real results arrive here ----------------------------------
                    worker.addEventListener('message', e => {
                        const result = e.data;
        
                        if (result.results) {                      // intermediate rows
                            result.results.forEach(updateResultsTable);
                        }
        
                        if (result.result === 'SUCCESS') {         // final answer
                            displayResult(result, startTime);
                            terminateAllWorkers();
                            updateResultsTable(result);
                            window.calculationComplete = true;
                        } else {
                            updateResultsTable(result);
                            if (result.status === 'FINISHED') activeWorkers--;
                            if (activeWorkers === 0) {
                                displayResult(result, startTime);
                                window.calculationComplete = true;
                            }
                        }
                    });
        
                    // now it is safe to start work -------------------------------
                    worker.postMessage({
                        initDelay: 0,               // delay no longer necessary
                        z, inputPrecision, MinCodeLength, MaxCodeLength,
                        cpuId: i, ncpus
                    });
                }
            });
        
            worker.onerror = err => console.error('Worker error:', err);
        
            workers.push(worker);
        }


    } catch (error) {
        console.error("Error in calculate function:", error);
        alert("An error occurred while performing the calculation. Please try again.");
    }
}

function createWolframAlphaLink(formula) {
    const encodedFormula = encodeURIComponent(formula);
    return `https://www.wolframalpha.com/input?i=${encodedFormula}`;
}


function terminateAllWorkers() {
    workers.forEach(worker => worker.terminate());
    workers = [];
    activeWorkers = 0;
}

function clearResults() {
    document.getElementById('resultInfix').value = '';
    document.getElementById('resultRPN').textContent = '';
    document.getElementById('resultMathematica').textContent = '';
    document.getElementById('timing').textContent = '';
    document.getElementById('resultNumeric').value = '';
}

function displayResult(result, startTime) {
    const rpnCode = result.RPN;
    const endTime = new Date();
    const timeTaken = (endTime - startTime)/1000.0;
    let mmaResult;



    if (result.result === "SUCCESS") {
      //document.getElementById('resultInfix').value = Interpreter.replaceGammaWithFactorial(Interpreter.removeRedundantParentheses(Interpreter.rpnToInfix(rpnCode.split(', '))));
      
      const rpnArray = rpnCode.split(', ');
      const infixExpression = Interpreter.rpnToInfix(rpnArray);
      const cleanedExpression = Interpreter.removeRedundantParentheses(infixExpression);
      const cleanedExpression2 = Interpreter.removeOutermostParentheses(infixExpression);
      const finalExpression = Interpreter.replaceGammaWithFactorial(cleanedExpression2);
      
      document.getElementById('resultInfix').value = finalExpression;


      document.getElementById('resultRPN').textContent = rpnCode;
      //document.getElementById('resultMathematica').textContent = Mma.rpnToMma(rpnCode.split(", ")) || "";
      mmaResult =  Mma.rpnToMma(rpnCode.split(", ")) || "";

      document.getElementById('resultMathematica').innerHTML = `<a href="${createWolframAlphaLink(mmaResult)}" target="_blank">${mmaResult}</a>`;

      document.getElementById('timing').textContent = `${timeTaken} s`;
      document.getElementById('resultNumeric').value = Evaluator.evaluateRPN(rpnCode.split(", "));
    } else {
      document.getElementById('resultInfix').value = 'Not found';
      document.getElementById('resultRPN').textContent = 'Nothing unambiguous found so far. Use larger K and be patient...';
      document.getElementById('resultMathematica').textContent = '?';
      document.getElementById('timing').textContent = `${timeTaken} s`;
      document.getElementById('resultNumeric').value = 'Check table';
    };
}

function updateResultsTable(result) {

    //console.log("From updateResultsTable");
    //console.log(result);
    if (!window.dataTable) {
        console.error('DataTable not initialized');
        return;
    }
    
    const rpnCode = result.RPN.split(", ");
    const z = parseFloat(document.getElementById('numberInput').value);
    const K = parseInt(result.K);
    const n = 36;
    const mmaResult=Mma.rpnToMma(rpnCode) || "";
    const wolframLink = createWolframAlphaLink(mmaResult);    

    const compressionRatio = calculateCompressionRatio(result.REL_ERR, inputRelativePrecision, K, n, z);
    
    window.dataTable.row.add([
        result.cpuId,
        K,
        Evaluator.evaluateRPN(rpnCode) || "",
        `<a href="${wolframLink}" target="_blank">${mmaResult}</a>`, // This line is modified
        result.result,
        result.REL_ERR,
        compressionRatio.toFixed(7),
        result.HAMMING_DISTANCE,
        result.RPN
    ]).draw(false);

    applyFilters(); // Add this line
}

function clearResultsTable() {
    if (window.dataTable) {
        window.dataTable.clear().draw();
    }
}

function applyFilters() {
    let selectedStatuses = Array.from(document.querySelectorAll('#table-filters input:checked'))
        .map(checkbox => checkbox.value);
    
    $.fn.dataTable.ext.search.push(
        function(settings, data, dataIndex) {
            let status = data[4]; // Assuming status is in the 5th column
            return selectedStatuses.includes(status);
        }
    );
    
    window.dataTable.draw();
    
    // Clear the custom filter
    $.fn.dataTable.ext.search.pop();
}

function setupFilterListeners() {
    document.querySelectorAll('#table-filters input[type="checkbox"]').forEach(checkbox => {
        checkbox.addEventListener('change', applyFilters);
    });

    document.getElementById('clearFilters').addEventListener('click', () => {
        document.querySelectorAll('#table-filters input[type="checkbox"]').forEach(checkbox => {
            checkbox.checked = true;
        });
        applyFilters();
    });

    // Apply filters immediately when the page loads
    //applyFilters();
}


function countSignificantDigits(number) {
  if (number === 0) {
    return 1; // Special case for 0
  }

  const s = String(Math.abs(number));
  
  if (s.indexOf(".") == -1)
  {
    return s.length;
  }

  // Remove leading zeros after the decimal point
  let firstSignificantDigitIndex = 0;
  if (number > -1 && number < 1) {
    
    for (let i = 0; i < s.length; i++)
     {
      if (s[i]!="0" && s[i]!=".")
       {
        firstSignificantDigitIndex=i;
        break;
       }
     }
  }

  return s.length - firstSignificantDigitIndex - (s.indexOf(".")<firstSignificantDigitIndex);
}

function calculateCompressionRatio(relativeError, inputRelativePrecision, K, n, targetX) {


    if (targetX === 0) {
        return 0; // Handle cases where the target is 0
    }



    if (relativeError === 0) {
        // Perfect match (special case)
        //const digitsInTarget = Math.floor(Math.log10(Math.abs(targetX))) + 1;
        const digitsInTarget = countSignificantDigits(targetX);
        const informationInRPN = K * Math.log10(n);
        console.log('informationInRPN:', informationInRPN);
        console.log('digitsInTarget:', digitsInTarget);

        if (informationInRPN <= 0) {
            return 0; // Or handle it as an error
        } else {
            return digitsInTarget / informationInRPN;
        }
    } else {
        // General case (approximation)
        const digitsInTarget = -Math.log10(relativeError);
        const informationInRPN = K * Math.log10(n);

        if (relativeError >= 1 || informationInRPN <= 0) {
            return 0; // Or handle it as a very poor approximation
        } else {
            return digitsInTarget / informationInRPN;
        }
    }
}

async function testSingleValue(value) {
    try {
        if (!Module) {
            await initializeModule();
        }
        
        //document.getElementById('numberInput').value = value.toString();
        document.getElementById('numberInput').value = value;
        
        const startTime = new Date();
        
        window.calculationComplete = false;
        calculate(); 

        // Wait for the calculation to complete
        while (!window.calculationComplete) {
            await new Promise(resolve => setTimeout(resolve, 100));  // Wait 100ms before checking again
        }

        const endTime = new Date();
        
        const result = {
            input: value,
            abs_error: document.getElementById('delta_z').textContent, 
            infix: document.getElementById('resultInfix').value,
            rpn: document.getElementById('resultRPN').textContent,
            mathematica: document.getElementById('resultMathematica').textContent,
            numeric: document.getElementById('resultNumeric').value,
            timeTaken: (endTime - startTime) / 1000  // in seconds
        };
        
        console.log('Test result:', result);
        window.calculationComplete=false;
        return result;
    } catch (error) {
        console.error('Error testing value:', error);
    }
}

async function runBenchmark(filename) {

    const startTime = Date.now();
    const response = await fetch(filename);
    const tsvData = await response.text();
    
    const testCases = tsvData
    .split('\n')
    .filter(line => line.trim() !== '') // Remove empty lines
    //.slice(1) // Skip the header row
    .map(line => {
    //const testCases = tsvData.split('\n').slice(1).map(line => {
        console.log("line:",line);
        //const [input, name, wzor] = line.split('\t');
        const [input, name, wzor] = line.split('\t').map(field => field.trim().replace(/^"(.*)"$/, '$1'));

        console.log("[input, name, wzor]", input, name, wzor);
        return { input: input, expected: wzor.trim() };
    });

    const results = [];

    for (const testCase of testCases) {
        const result = await testSingleValue(testCase.input);
        result.expected = testCase.expected;
        results.push(result);
    }


    const endTime = Date.now();
    const totalTimeInSeconds = (endTime - startTime) / 1000;
    console.log(`Benchmark complete. Total cases: ${results.length}`);
    console.log(`Total benchmark time: ${totalTimeInSeconds}`);
    return results;
}

function saveResultsToJSON(results) {
    const jsonString = JSON.stringify(results, null, 2);
    const blob = new Blob([jsonString], { type: 'application/json' });
    const url = URL.createObjectURL(blob);

 // Create a timestamp
    const now = new Date();
    const timestamp = now.toISOString().replace(/:/g, '-').replace(/\..+/, '');

    
    const a = document.createElement('a');
    a.href = url;
    a.download = 'benchmark_results_' + timestamp + '.json';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

async function runAndSaveBenchmark(filename) {
    const benchmarkResults = await runBenchmark(filename);
    if (benchmarkResults) {
        saveResultsToJSON(benchmarkResults);
        console.log('Benchmark results saved to JSON file.');
    } else {
        console.log('Benchmark failed. No results to save.');
    }
}

// Make these functions globally accessible
window.runBenchmark = runBenchmark;
window.saveResultsToJSON = saveResultsToJSON;
window.runAndSaveBenchmark = runAndSaveBenchmark;

// Make it globally accessible
window.testSingleValue = testSingleValue;

function setupThreadSlider() {
    const threadSlider = document.getElementById('threadCount');
    const autoThreadCheckbox = document.getElementById('autoThreads');
    const detected = navigator.hardwareConcurrency || 7;
    
    // Set max value based on detected cores (with some room for higher values if needed)
    threadSlider.max = Math.max(32, detected * 2);
    
    // Set initial value to detected cores
    threadSlider.value = detected;
    updateThreadCountValue('Auto');
    
    // Event listeners for the thread slider and auto checkbox
    threadSlider.addEventListener('input', (event) => {
        const value = parseInt(event.target.value);
        updateThreadCountValue(value);
        
        // Uncheck Auto checkbox when user manually changes the slider
        if (autoThreadCheckbox.checked) {
            autoThreadCheckbox.checked = false;
        }
    });
    
    autoThreadCheckbox.addEventListener('change', (event) => {
        if (event.target.checked) {
            // If Auto is checked, set slider to detected value
            threadSlider.value = detected;
            updateThreadCountValue('Auto');
        } else {
            // If Auto is unchecked, show the current slider value
            updateThreadCountValue(parseInt(threadSlider.value));
        }
    });
}

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


    // Display number of CPUs
    const cpuSpan = document.getElementById('detectedCPUs');
    if (cpuSpan) {
        cpuSpan.textContent = navigator.hardwareConcurrency || 'Unknown';
    }

    setupFilterListeners(); 
    setupThreadSlider();
}

// Initialize everything when the DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    setupEventListeners();
    initializeModule();

    // New code for handling paste events to remove whitespaces
    const inputElement = document.getElementById('numberInput');
    
    inputElement.addEventListener('paste', (event) => {
        event.preventDefault();
        let pastedText = (event.clipboardData || window.clipboardData).getData('text');
        pastedText = pastedText.replace(/\s/g, '');
        inputElement.value = pastedText;
    });
});
