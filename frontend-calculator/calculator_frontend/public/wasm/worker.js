// Worker script: worker.js
//importScripts('rpn_function.js');
importScripts('vsearch.js');

let isReady = false;

// tell the main thread only when the runtime is ready
Module.onRuntimeInitialized = () => {
    isReady = true;
    postMessage({type: 'ready'});
};

function waitForReady() {
    return new Promise(resolve => {
        if (isReady) {
            resolve();
        } else {
            const check = setInterval(() => {
                if (isReady) {
                    clearInterval(check);
                    resolve();
                }
            }, 10);
        }
    });
}

function doWork(initDelay, z, inputPrecision, MinCodeLength, MaxCodeLength, cpuId, ncpus) {
    return new Promise(resolve => {
        setTimeout(() => {
            try {
                // Using standard FP64 version
                const result = Module.ccall('search_RPN', 'string', 
                               ['number', 'number', 'number', 'number', 'number', 'number'], 
                               [z, inputPrecision, MinCodeLength, MaxCodeLength, cpuId, ncpus]);
                resolve(JSON.parse(result));
            } catch (err) {
                console.error('WASM call error:', err);
                resolve({ results: [], error: err.message });
            }
        }, initDelay);
    });
}

onmessage = async function(e) {
    const { initDelay = 0, z, inputPrecision, MinCodeLength, MaxCodeLength, cpuId, ncpus } = e.data;
    
    // Wait for WASM to be ready
    await waitForReady();
    
    console.log(`Worker ${cpuId} of ${ncpus} starting work for z=${z}, Delta_z = ${inputPrecision}, MinCodeLength = ${MinCodeLength}, MaxCodeLength=${MaxCodeLength}`);
    
    const resultJSON = await doWork(initDelay, z, inputPrecision, MinCodeLength, MaxCodeLength, cpuId, ncpus);
    
    console.log(`Worker ${cpuId} finished work with result:`, resultJSON);
    
    postMessage({
        cpuId,
        ...resultJSON
    });
};