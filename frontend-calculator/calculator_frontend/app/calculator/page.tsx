'use client';

import { useState, useEffect, useRef, useMemo } from 'react';
import { SearchResult, Filters, Precision, ActiveWorker, defaultFilters, ErrorMode, ComputeMode } from './lib/types';
import { extractPrecision, evaluateRPN } from './lib/rpn';
import { useWebGPU } from './hooks/useWebGPU';
import { Sidebar, InputBar, ResultCard, ResultsTable, EmptyState } from './components';

// Ensures that all worker/WASM fetches include the configured base path (if any).
// - Trailing slashes are removed so "//" never appears in URLs.
// - A leading "/" is added when needed so a value like "~user/app" becomes "/~user/app".
// - In the browser we return an absolute URL using window.location.origin; on the server we
//   return a path that Next.js can understand during static export.
const withBasePath = (path: string) => {
  const base = process.env.NEXT_PUBLIC_BASE_PATH?.replace(/\/+$/g, '') ?? '';
  const normalizedBase = base ? (base.startsWith('/') ? base : `/${base}`) : '';
  const normalizedPath = path.startsWith('/') ? path : `/${path}`;
  if (typeof window === 'undefined') return `${normalizedBase}${normalizedPath}`;
  return new URL(`${normalizedBase}${normalizedPath}`, window.location.origin).toString();
};


export default function CalculatorPage() {
  const [inputValue, setInputValue] = useState('');
  const [results, setResults] = useState<SearchResult[]>([]);
  const [wasmLoaded, setWasmLoaded] = useState(false);
  const [isCalculating, setIsCalculating] = useState(false);
  const [searchDepth, setSearchDepth] = useState(7);
  const [threadCount, setThreadCount] = useState(4);
  const [autoThreads, setAutoThreads] = useState(true);
  const [detectedCPUs, setDetectedCPUs] = useState(4);
  const [filters, setFilters] = useState<Filters>(defaultFilters);
  const [precision, setPrecision] = useState<Precision>({});
  const [activeWorkers, setActiveWorkers] = useState<ActiveWorker[]>([]);
  const [sortColumn, setSortColumn] = useState<'K' | 'REL_ERR' | 'CR' | null>(null);
  const [sortDirection, setSortDirection] = useState<'asc' | 'desc'>('asc');
  const [searchFinished, setSearchFinished] = useState(false);
  const [elapsedTime, setElapsedTime] = useState(0);
  const [sidebarCollapsed, setSidebarCollapsed] = useState(false);
  const [errorMode, setErrorMode] = useState<ErrorMode>('automatic');
  const [manualError, setManualError] = useState('');
  const [computeMode, setComputeMode] = useState<ComputeMode>('auto');
  
  const workersRef = useRef<Worker[]>([]);
  const isAbortedRef = useRef(false);
  const timerRef = useRef<NodeJS.Timeout | null>(null);
  const startTimeRef = useRef<number>(0);
  const resolveAllRef = useRef<(() => void) | null>(null);
  const searchEndedRef = useRef(false);

  const {
    gpuAvailable,
    gpuInfo,
    search: searchGPU,
    abort: abortGPU
  } = useWebGPU();
  const gpuName = gpuInfo?.name;
  
  // Helper to calculate compression ratio
  const getCompressionRatio = (r: SearchResult): number => {
    if (r.compressionRatio !== undefined && r.compressionRatio !== null) {
      return r.compressionRatio;
    }
    if (typeof r.REL_ERR === 'number' && r.K > 0) {
      const numerator = r.REL_ERR === 0 ? 16.0 : -Math.log10(r.REL_ERR);
      return numerator / r.K / Math.log10(36);
    }
    return 0;
  };
  
  // Best result = MAXIMUM Compression Ratio (CR) - this is the correct identification criterion
  // CR rises initially as accuracy improves, then falls when overfitting starts
  // The maximum CR indicates the true match
  const bestResult = useMemo(() => {
    if (results.length === 0) return null;
    return [...results].sort((a, b) => getCompressionRatio(b) - getCompressionRatio(a))[0];
  }, [results]);

  // Check for WASM support and detect CPUs
  useEffect(() => {
    const checkWasm = async () => {
      try {
        //const response = await fetch('/wasm/rpn_function.wasm');
        const response = await fetch(withBasePath('/wasm/rpn_function.wasm'));
        setWasmLoaded(response.ok);
      } catch {
        setWasmLoaded(false);
      }
    };
    checkWasm();
    
    const cpus = navigator.hardwareConcurrency || 4;
    setDetectedCPUs(cpus);
    setThreadCount(cpus);
    
  }, []);


  const handleWorkerMessage = (cpuId: number, e: MessageEvent, onComplete?: () => void) => {
    const data = e.data;
    
    // Skip ready message
    if (data.type === 'ready') return;
    
    // Collect all results in one batch to avoid multiple re-renders
    const newResults: SearchResult[] = [];
    
    // Worker completed with results array
    if (data.results && Array.isArray(data.results)) {
      data.results.forEach((r: { K: number; RPN: string; result: string; REL_ERR: number; status?: string; cpuId?: number; COMPRESSION_RATIO?: number }) => {
        // Calculate numeric value from RPN
        let numericValue: string;
        try {
          numericValue = evaluateRPN(r.RPN).toString();
        } catch {
          numericValue = 'N/A';
        }
        
        const resolvedCpuId = r.cpuId ?? data.cpuId ?? cpuId;
        newResults.push({
          cpuId: resolvedCpuId,
          K: r.K,
          RPN: r.RPN,
          result: numericValue,
          REL_ERR: r.REL_ERR,
          status: r.result === 'INTERMEDIATE' ? 'SEARCHING' : (r.result || r.status || 'K_BEST'),
          compressionRatio: r.COMPRESSION_RATIO
        });
      });
      
      // Handle final result (SUCCESS/FAILURE/ABORTED) from top-level data
      if (data.result && data.RPN) {
        let numericValue: string;
        try {
          numericValue = evaluateRPN(data.RPN).toString();
        } catch {
          numericValue = 'N/A';
        }
        
        newResults.push({
          cpuId: data.cpuId ?? cpuId,
          K: data.K,
          RPN: data.RPN,
          result: numericValue,
          REL_ERR: data.REL_ERR,
          status: data.result, // SUCCESS, FAILURE, ABORTED
          compressionRatio: data.COMPRESSION_RATIO
        });
      }
      
      // Update state once with all results from this worker
      if (newResults.length > 0) {
        setResults(prev => [...prev, ...newResults]);
      }
      
      const isSuccess = data.result === 'SUCCESS';
      if (isSuccess && !searchEndedRef.current) {
        searchEndedRef.current = true;
        workersRef.current.forEach(w => w.terminate());
        workersRef.current = [];
        setActiveWorkers([]);
        resolveAllRef.current?.();
      } else {
        // Worker finished
        setActiveWorkers(prev => prev.filter(w => w.id !== cpuId));
        
        // Notify completion
        if (onComplete) onComplete();
      }
    }
  };

  const handleWorkerError = (cpuId: number, error: ErrorEvent) => {
    console.error(`Worker ${cpuId} error:`, error.message || error);
    setActiveWorkers(prev => prev.filter(w => w.id !== cpuId));
    setIsCalculating(false);
  };

  const calculate = async () => {
    if (!inputValue) return;
    
    setIsCalculating(true);
    setResults([]);
    setSearchFinished(false);
    searchEndedRef.current = false;
    isAbortedRef.current = false;
    resolveAllRef.current = null;
    
    // Start timer (update every 500ms to reduce re-renders)
    setElapsedTime(0);
    startTimeRef.current = Date.now();
    timerRef.current = setInterval(() => {
      setElapsedTime(Date.now() - startTimeRef.current);
    }, 500);
    
    // Calculate precision based on error mode
    let deltaZNum: number;
    const zNum = parseFloat(inputValue);
    
    if (errorMode === 'zero') {
      deltaZNum = 0;
    } else if (errorMode === 'manual' && manualError) {
      deltaZNum = parseFloat(manualError) || 0;
    } else {
      // automatic mode - use extractPrecision
      const autoPrecision = extractPrecision(inputValue);
      deltaZNum = parseFloat(autoPrecision.deltaZ || '0.5');
    }
    
    // Update precision display
    const relDeltaZ = zNum !== 0 ? deltaZNum / Math.abs(zNum) : 0;
    setPrecision({
      z: inputValue,
      deltaZ: deltaZNum === 0 ? '0' : deltaZNum.toExponential(2),
      relDeltaZ: relDeltaZ === 0 ? '0' : relDeltaZ.toExponential(2)
    });

    const shouldUseGpu =
      (computeMode === 'gpu' && gpuAvailable) ||
      (computeMode === 'auto' && gpuAvailable);

    if (shouldUseGpu) {
      setActiveWorkers([]);
      try {
        const gpuResults = await searchGPU(zNum, {
          minK: 1,
          maxK: searchDepth
        });

        const mappedResults: SearchResult[] = gpuResults.map(result => {
          let numericValue: string;
          try {
            numericValue = evaluateRPN(result.RPN).toString();
          } catch {
            numericValue = 'N/A';
          }

          return {
            cpuId: result.cpuId ?? 0,
            K: result.K,
            RPN: result.RPN,
            result: numericValue,
            REL_ERR: result.REL_ERR,
            status: result.status
          };
        });

        setResults(mappedResults);
      } catch (err) {
        console.error('GPU search failed:', err);
      } finally {
        if (timerRef.current) {
          clearInterval(timerRef.current);
          timerRef.current = null;
        }
        setElapsedTime(Date.now() - startTimeRef.current);
        setIsCalculating(false);
        setSearchFinished(true);
      }
      return;
    }

    // CPU/WASM computation
    const effectiveThreads = autoThreads ? detectedCPUs : threadCount;
    
    // Terminate existing workers
    workersRef.current.forEach(w => w.terminate());
    workersRef.current = [];
    
    // Create new workers with completion tracking
    const workers: Worker[] = [];
    const initialActiveWorkers: ActiveWorker[] = [];
    let completedCount = 0;

    const allComplete = new Promise<void>(resolve => {
      resolveAllRef.current = resolve;
    });
    


    //let resolveAll: () => void;
    //const allComplete = new Promise<void>(resolve => { resolveAll = resolve; });
    //resolveAllRef.current = resolveAll;
    
    for (let i = 0; i < effectiveThreads; i++) {
      //const worker = new Worker('/wasm/worker.js');
      const worker = new Worker(withBasePath('/wasm/worker.js'));
      const cpuId = i;
      
      const onComplete = () => {
        completedCount++;
        if (completedCount >= effectiveThreads) {
          resolveAllRef.current?.();
        }
      };
      
      worker.onmessage = (e) => handleWorkerMessage(cpuId, e, onComplete);
      worker.onerror = (e) => handleWorkerError(cpuId, e);
      
      workers.push(worker);
      initialActiveWorkers.push({ id: cpuId, status: 'running', currentK: 1 });
    }
    
    workersRef.current = workers;
    setActiveWorkers(initialActiveWorkers);
    
    // Start computation on each worker
    workers.forEach((worker, i) => {
      worker.postMessage({
        initDelay: 0,
        z: parseFloat(inputValue),
        inputPrecision: deltaZNum,
        MinCodeLength: 1,
        MaxCodeLength: searchDepth,
        cpuId: i,
        ncpus: effectiveThreads
      });
    });
    
    // Wait for all workers to complete
    await allComplete;
    
    // Stop timer
    if (timerRef.current) {
      clearInterval(timerRef.current);
      timerRef.current = null;
    }
    setElapsedTime(Date.now() - startTimeRef.current);
    
    if (!isAbortedRef.current) {
      setIsCalculating(false);
      setSearchFinished(true);
    }
  };

  const handleAbort = () => {
    isAbortedRef.current = true;
    workersRef.current.forEach(w => w.terminate());
    workersRef.current = [];
    setActiveWorkers([]);
    abortGPU();
    resolveAllRef.current?.();
    // Stop timer
    if (timerRef.current) {
      clearInterval(timerRef.current);
      timerRef.current = null;
    }
    setElapsedTime(Date.now() - startTimeRef.current);
    setIsCalculating(false);
  };

  const handleReset = () => {
    handleAbort();
    setInputValue('');
    setResults([]);
    setPrecision({});
    setSortColumn(null);
    setSortDirection('asc');
    setFilters(defaultFilters);
    setSearchFinished(false);
    setElapsedTime(0);
  };

  const handleExampleClick = (value: string) => {
    setInputValue(value);
  };

  return (
    <div className="flex h-screen w-screen bg-gray-50 dark:bg-[#1a1a1d] overflow-hidden">
      {/* Sidebar */}
      <Sidebar
        wasmLoaded={wasmLoaded}
        detectedCPUs={detectedCPUs}
        searchDepth={searchDepth}
        setSearchDepth={setSearchDepth}
        threadCount={threadCount}
        setThreadCount={setThreadCount}
        autoThreads={autoThreads}
        setAutoThreads={setAutoThreads}
        precision={precision}
        activeWorkers={activeWorkers}
        isCalculating={isCalculating}
        onAbort={handleAbort}
        onReset={handleReset}
        isOpen={!sidebarCollapsed}
        onToggle={() => setSidebarCollapsed(!sidebarCollapsed)}
        errorMode={errorMode}
        setErrorMode={setErrorMode}
        manualError={manualError}
        setManualError={setManualError}
        gpuAvailable={gpuAvailable}
        gpuName={gpuName}
        computeMode={computeMode}
        setComputeMode={setComputeMode}
      />

      {/* Main content */}
      <main className="flex-1 flex flex-col overflow-hidden">
        <InputBar
          inputValue={inputValue}
          setInputValue={setInputValue}
          isCalculating={isCalculating}
          onCalculate={calculate}
          onReset={handleReset}
          onAbort={handleAbort}
        />

        {/* Search Status */}
        {isCalculating && (
          <div className="bg-blue-500 text-white py-3 px-4 text-center flex items-center justify-center gap-4">
            <svg className="w-5 h-5 animate-spin" fill="none" viewBox="0 0 24 24">
              <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" />
              <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z" />
            </svg>
            <span className="font-bold">Searching for formulas...</span>
            <span className="font-mono">{(elapsedTime / 1000).toFixed(1)}s</span>
            {precision.deltaZ && (
              <span className="text-sm opacity-75">(±{precision.deltaZ})</span>
            )}
          </div>
        )}

        {results.length > 0 && bestResult ? (
          <div className="flex-1 min-h-0 overflow-hidden flex flex-col bg-white dark:bg-[#1a1a1d]">
            {/* Success banner */}
            {searchFinished && !isCalculating && (
              <div className="bg-green-500 text-white py-2 px-4 text-center text-sm">
                Found {results.length} result{results.length !== 1 ? 's' : ''} in {(elapsedTime / 1000).toFixed(2)}s
              </div>
            )}
            {/* Best result card */}
            <ResultCard result={bestResult} allResults={results} />
            {/* Results table */}
            <ResultsTable
              results={results}
              filters={filters}
              setFilters={setFilters}
              sortColumn={sortColumn}
              setSortColumn={setSortColumn}
              sortDirection={sortDirection}
              setSortDirection={setSortDirection}
            />
          </div>
        ) : (
          <EmptyState onExampleClick={handleExampleClick} />
        )}
      </main>
    </div>
  );
}
