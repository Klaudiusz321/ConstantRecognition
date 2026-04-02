// Worker script: worker.js
importScripts('rpn_function.js');


// tell the main thread only when the runtime is ready
Module.onRuntimeInitialized = () => postMessage({type: 'ready'});


function doWork(initDelay, z, inputPrecision, MinCodeLength, MaxCodeLength, cpuId, ncpus) {
    return new Promise(resolve => {
        setTimeout(() => {
            const result = Module.ccall('search_RPN', 'string', 
                           ['number', 'number', 'number', 'number', 'number', 'number'], 
                           [z, inputPrecision, MinCodeLength, MaxCodeLength, cpuId, ncpus]);
            resolve(JSON.parse(result));
        }, initDelay);
    });
}

onmessage = async function(e) {
    const { initDelay, z, inputPrecision, MinCodeLength, MaxCodeLength, cpuId, ncpus } = e.data;
    console.log(`Worker ${cpuId} of ${ncpus} starting work for z=${z}, Delta_z = ${inputPrecision}, MinCodeLength = ${MinCodeLength}, MaxCodeLength=${MaxCodeLength}`);
    
    const resultJSON = await doWork(initDelay, z, inputPrecision, MinCodeLength, MaxCodeLength, cpuId, ncpus);
    
    console.log(`Worker ${cpuId} finished work with result:`, resultJSON);
    
    postMessage({
        cpuId,
        ...resultJSON
    });
};