'use client';

import { useState } from 'react';
import { ActiveWorker, Precision, ErrorMode, ComputeMode } from '../lib/types';

interface SidebarProps {
  wasmLoaded: boolean;
  detectedCPUs: number;
  searchDepth: number;
  setSearchDepth: (depth: number) => void;
  threadCount: number;
  setThreadCount: (count: number) => void;
  autoThreads: boolean;
  setAutoThreads: (auto: boolean) => void;
  precision: Precision;
  activeWorkers: ActiveWorker[];
  isCalculating: boolean;
  onAbort: () => void;
  onReset: () => void;
  isOpen: boolean;
  onToggle: () => void;
  // Error mode
  errorMode: ErrorMode;
  setErrorMode: (mode: ErrorMode) => void;
  manualError: string;
  setManualError: (value: string) => void;
  // GPU info (auto-detected)
  gpuAvailable: boolean;
  gpuName?: string;
  //Compute mode selection
  computeMode: ComputeMode;
  setComputeMode: (mode: ComputeMode) => void;
}

export function Sidebar({
  wasmLoaded,
  detectedCPUs,
  searchDepth,
  setSearchDepth,
  threadCount,
  setThreadCount,
  autoThreads,
  setAutoThreads,
  precision,
  activeWorkers,
  isCalculating,
  onAbort,
  onReset,
  isOpen,
  onToggle,
  errorMode,
  setErrorMode,
  manualError,
  setManualError,
  gpuAvailable,
  gpuName,
  computeMode,
  setComputeMode
}: SidebarProps) {
  const [showAdvanced, setShowAdvanced] = useState(false);

  return (
    <>
      {/* Toggle Arrow Button - visible when sidebar is collapsed */}
      {!isOpen && (
        <button
          onClick={onToggle}
          className="fixed top-1/2 left-0 -translate-y-1/2 z-50 p-2 bg-white dark:bg-[#1a1a1d] rounded-r-lg shadow-lg border border-l-0 border-gray-200 dark:border-[#2a2a2e] hover:bg-gray-50 dark:hover:bg-[#2a2a2e] transition-colors"
          aria-label="Open sidebar"
        >
          <svg className="w-5 h-5 text-gray-600 dark:text-gray-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 5l7 7-7 7" />
          </svg>
        </button>
      )}

      {/* Sidebar */}
      <aside className={`
        relative
        bg-white dark:bg-[#1a1a1d] 
        border-r border-gray-200 dark:border-[#2a2a2e] 
        flex flex-col
        transition-all duration-300 ease-in-out
        overflow-x-hidden
        ${isOpen ? 'w-80 min-w-80' : 'w-0 min-w-0 overflow-hidden'}
      `}>
        {/* Header with collapse button */}
        <div className="p-4 border-b border-gray-200 dark:border-[#2a2a2e]">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-3">
              <img 
                src="/cdaaebfdc71641160f831c2a2fb564ce8d081055.png" 
                alt="Logo" 
                className="w-10 h-10 rounded-lg object-cover"
              />
              <div>
                <h1 className="font-semibold text-gray-900 dark:text-white text-sm">Constant Recognizer</h1>
                <p className="text-[10px] text-gray-500">Jagiellonian University</p>
              </div>
            </div>
            {/* Collapse button */}
            <button
              onClick={onToggle}
              className="p-1.5 rounded-md hover:bg-gray-100 dark:hover:bg-[#2a2a2e] transition-colors"
              aria-label="Collapse sidebar"
            >
              <svg className="w-5 h-5 text-gray-500" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 19l-7-7 7-7" />
              </svg>
            </button>
          </div>
        </div>

        {/* Settings */}
        <div className="flex-1 p-4 space-y-6 overflow-y-auto">
          {/* Status */}
          <div className="space-y-2">
            <label className="text-xs lg:text-[10px] font-medium text-gray-500 dark:text-gray-500 uppercase tracking-wider">Status</label>
            <div className="flex items-center gap-2 text-base lg:text-sm">
              <div className={`w-3 h-3 lg:w-2 lg:h-2 rounded-full ${wasmLoaded ? 'bg-green-500' : 'bg-amber-500'}`} />
              <span className="text-gray-700 dark:text-gray-300">{wasmLoaded ? 'WASM Ready' : 'Demo Mode'}</span>
            </div>
            <div className="text-sm lg:text-xs text-gray-500 dark:text-gray-500">
              {detectedCPUs} logical CPUs detected
            </div>
            {gpuAvailable && (
              <div className="flex items-center gap-2 text-sm lg:text-xs text-green-600 dark:text-green-400">
                <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 3v2m6-2v2M9 19v2m6-2v2M5 9H3m2 6H3m18-6h-2m2 6h-2M7 19h10a2 2 0 002-2V7a2 2 0 00-2-2H7a2 2 0 00-2 2v10a2 2 0 002 2zM9 9h6v6H9V9z" />
                </svg>
                <span>WebGPU: {gpuName || 'Available'}</span>
              </div>
            )}
          </div>

{/* Compute Backend Selection */}
          <div className="space-y-2">
            <label className="text-[10px] font-medium text-gray-500 dark:text-gray-500 uppercase tracking-wider">
              Compute Backend
            </label>
            <div className="flex rounded-md shadow-sm">
              <button
                type="button"
                onClick={() => setComputeMode('auto')}
                disabled={isCalculating}
                className={`flex-1 px-3 py-2 text-xs font-medium rounded-l-md border transition-colors
                  ${computeMode === 'auto' 
                    ? 'bg-[#0066cc] text-white border-[#0066cc]' 
                    : 'bg-white dark:bg-[#111113] text-gray-700 dark:text-gray-300 border-gray-300 dark:border-gray-600 hover:bg-gray-50 dark:hover:bg-[#2a2a2e]'}
                  disabled:opacity-50 disabled:cursor-not-allowed`}
              >
                Auto
              </button>
              <button
                type="button"
                onClick={() => setComputeMode('cpu')}
                disabled={isCalculating}
                className={`flex-1 px-3 py-2 text-xs font-medium border-t border-b transition-colors
                  ${computeMode === 'cpu' 
                    ? 'bg-[#0066cc] text-white border-[#0066cc]' 
                    : 'bg-white dark:bg-[#111113] text-gray-700 dark:text-gray-300 border-gray-300 dark:border-gray-600 hover:bg-gray-50 dark:hover:bg-[#2a2a2e]'}
                  disabled:opacity-50 disabled:cursor-not-allowed`}
              >
                CPU
              </button>
              <button
                type="button"
                onClick={() => setComputeMode('gpu')}
                disabled={isCalculating || !gpuAvailable}
                className={`flex-1 px-3 py-2 text-xs font-medium rounded-r-md border transition-colors
                  ${computeMode === 'gpu' 
                    ? 'bg-[#0066cc] text-white border-[#0066cc]' 
                    : 'bg-white dark:bg-[#111113] text-gray-700 dark:text-gray-300 border-gray-300 dark:border-gray-600 hover:bg-gray-50 dark:hover:bg-[#2a2a2e]'}
                  disabled:opacity-50 disabled:cursor-not-allowed`}
              >
                GPU {!gpuAvailable && '(N/A)'}
              </button>
            </div>
            {/* Status text */}
            <div className="text-[10px] text-gray-400 dark:text-gray-500">
              {computeMode === 'auto' && (
                gpuAvailable 
                  ? <span className="text-green-600 dark:text-green-400">Will use GPU (WebGPU available)</span>
                  : <span>Will use CPU (WebGPU not available)</span>
              )}
              {computeMode === 'cpu' && (
                <span>WASM Workers ({autoThreads ? detectedCPUs : threadCount} threads)</span>
              )}
              {computeMode === 'gpu' && (
                gpuAvailable 
                  ? <span className="text-green-600 dark:text-green-400">{gpuName || 'WebGPU'}</span>
                  : <span className="text-amber-600 dark:text-amber-400">⚠️ GPU unavailable, will use CPU</span>
              )}
            </div>
          </div>

          {/* Precision Info */}
          {precision.z && (
            <div className="space-y-2">
              <label className="text-xs lg:text-[10px] font-medium text-gray-500 dark:text-gray-500 uppercase tracking-wider">
                Search Target
              </label>
              <div className="text-sm lg:text-xs font-mono text-gray-600 dark:text-gray-400 space-y-1 bg-gray-50 dark:bg-[#111113] p-3 lg:p-2 rounded">
                <div>z = {precision.z}</div>
                <div>Δz = {precision.deltaZ}</div>
                <div>δz/z = {precision.relDeltaZ}</div>
              </div>
            </div>
          )}

          {/* Active Workers */}
          {activeWorkers.length > 0 && (
            <div className="space-y-2">
              <label className="text-xs lg:text-[10px] font-medium text-gray-500 dark:text-gray-500 uppercase tracking-wider">
                Active Workers ({activeWorkers.length})
              </label>
              <div className="space-y-1 max-h-32 overflow-y-auto">
                {activeWorkers.map(w => (
                  <div key={w.id} className="flex items-center gap-2 text-sm lg:text-xs">
                    <div className="w-2 h-2 rounded-full bg-blue-500 animate-pulse" />
                    <span className="text-gray-600 dark:text-gray-400">
                      CPU {w.id}: searching...
                    </span>
                  </div>
                ))}
              </div>
            </div>
          )}

          {/* Complexity (K) - always visible */}
          <div className="space-y-2">
            <label className="text-[10px] font-medium text-gray-500 dark:text-gray-500 uppercase tracking-wider">
              Complexity (K)
            </label>
            <div className="flex items-center gap-3">
              <input
                type="range"
                min="2"
                max="9"
                value={searchDepth}
                onChange={(e) => setSearchDepth(parseInt(e.target.value))}
                className="flex-1 accent-[#0066cc] h-2"
              />
              <span className="font-mono text-sm font-bold text-gray-900 dark:text-white w-4">{searchDepth}</span>
            </div>
            <p className="text-[10px] text-gray-400">Search expressions with exactly K symbols</p>
          </div>

          {/* Threads - only visible when CPU will be used */}
          {(computeMode === 'cpu' || (computeMode === 'auto' && !gpuAvailable)) && (
            <div className="space-y-2">
              <label className="text-[10px] font-medium text-gray-500 dark:text-gray-500 uppercase tracking-wider">
                CPU Threads
              </label>
              <div className="flex items-center gap-3">
                <input
                  type="range"
                  min="1"
                  max="32"
                  value={threadCount}
                  onChange={(e) => setThreadCount(parseInt(e.target.value))}
                  disabled={autoThreads}
                  className="flex-1 accent-[#0066cc] disabled:opacity-40 h-2"
                />
                <span className="font-mono text-sm font-bold text-gray-900 dark:text-white w-8">
                  {autoThreads ? 'Auto' : threadCount}
                </span>
              </div>
              <label className="flex items-center gap-2 text-xs text-gray-600 dark:text-gray-400 cursor-pointer">
                <input
                  type="checkbox"
                  checked={autoThreads}
                  onChange={(e) => setAutoThreads(e.target.checked)}
                  className="accent-[#0066cc] w-4 h-4"
                />
                Auto-detect ({detectedCPUs} CPUs)
              </label>
            </div>
          )}

          {/* Advanced Options Toggle */}
          <div className="border-t border-gray-200 dark:border-[#2a2a2e] pt-4">
            <button
              onClick={() => setShowAdvanced(!showAdvanced)}
              className="flex items-center justify-between w-full text-sm text-gray-600 dark:text-gray-400 hover:text-gray-900 dark:hover:text-white transition-colors"
            >
              <span className="font-medium">Advanced Options</span>
              <svg 
                className={`w-4 h-4 transition-transform ${showAdvanced ? 'rotate-180' : ''}`} 
                fill="none" 
                stroke="currentColor" 
                viewBox="0 0 24 24"
              >
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 9l-7 7-7-7" />
              </svg>
            </button>
          </div>

          {/* Advanced Options Content */}
          {showAdvanced && (
            <div className="space-y-6 pb-2">
              
              {/* Uncertainty Mode */}
              <div className="space-y-2">
                <label className="text-[10px] font-medium text-gray-500 dark:text-gray-500 uppercase tracking-wider">
                  Uncertainty (±)
                </label>
                <div className="space-y-2">
                  <label className="flex items-center gap-2 cursor-pointer">
                    <input
                      type="radio"
                      name="errorMode"
                      checked={errorMode === 'automatic'}
                      onChange={() => setErrorMode('automatic')}
                      className="w-4 h-4 accent-[#0066cc]"
                    />
                    <span className="text-sm text-gray-700 dark:text-gray-300">Auto</span>
                    <span className="text-xs text-gray-400">(from decimals)</span>
                  </label>
                  <label className="flex items-center gap-2 cursor-pointer">
                    <input
                      type="radio"
                      name="errorMode"
                      checked={errorMode === 'zero'}
                      onChange={() => setErrorMode('zero')}
                      className="w-4 h-4 accent-[#0066cc]"
                    />
                    <span className="text-sm text-gray-700 dark:text-gray-300">Exact</span>
                    <span className="text-xs text-gray-400">(± 0)</span>
                  </label>
                  <label className="flex items-center gap-2 cursor-pointer">
                    <input
                      type="radio"
                      name="errorMode"
                      checked={errorMode === 'manual'}
                      onChange={() => setErrorMode('manual')}
                      className="w-4 h-4 accent-[#0066cc]"
                    />
                    <span className="text-sm text-gray-700 dark:text-gray-300">Manual</span>
                  </label>
                  {errorMode === 'manual' && (
                    <div className="flex items-center gap-2 ml-6">
                      <span className="text-gray-500">±</span>
                      <input
                        type="text"
                        value={manualError}
                        onChange={(e) => setManualError(e.target.value)}
                        placeholder="0.000001"
                        className="w-32 px-2 py-1 rounded border border-gray-300 dark:border-[#2a2a2e] bg-white dark:bg-[#111113] text-gray-900 dark:text-white font-mono text-sm"
                      />
                    </div>
                  )}
                </div>
              </div>
            </div>
          )}

          {/* Action Buttons */}
          <div className="space-y-3 lg:space-y-2">
            {isCalculating && (
              <button
                onClick={onAbort}
                className="w-full px-4 py-3 lg:py-2 bg-red-500 hover:bg-red-600 text-white font-medium rounded-lg transition-colors flex items-center justify-center gap-2 text-base lg:text-sm"
              >
                <svg className="w-5 h-5 lg:w-4 lg:h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
                </svg>
                Abort
              </button>
            )}
            
          </div>
        </div>

        {/* Footer */}
        <div className="p-4 border-t border-gray-200 dark:border-[#2a2a2e]">
          <a 
            href="https://github.com/Klaudiusz321/ConstantRecognition" 
            target="_blank" 
            rel="noopener noreferrer"
            className="flex items-center gap-2 text-xs text-gray-500 hover:text-[#0066cc] transition-colors"
          >
            <svg className="w-4 h-4" fill="currentColor" viewBox="0 0 24 24"><path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0024 12c0-6.63-5.37-12-12-12z"/></svg>
            GitHub
          </a>
        </div>
      </aside>
    </>
  );
}
