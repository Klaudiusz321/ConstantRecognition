'use client';

import { SearchResult } from '../lib/types';
import { rpnToMathematica, createWolframLink, rpnToLatex } from '../lib/rpn';
import { Latex } from './Latex';

interface ResultCardProps {
  result: SearchResult;
  allResults?: SearchResult[];  // For calculating accuracy jump
}

// Calculate compression ratio
function getCompressionRatio(r: SearchResult): number {
  if (r.compressionRatio !== undefined && r.compressionRatio !== null) {
    return r.compressionRatio;
  }
  if (typeof r.REL_ERR === 'number' && r.K > 0) {
    const numerator = r.REL_ERR === 0 ? 16.0 : -Math.log10(r.REL_ERR);
    return numerator / r.K / Math.log10(36);
  }
  return 0;
}

// Check if there's a significant accuracy jump (order of magnitude improvement)
function hasAccuracyJump(result: SearchResult, allResults: SearchResult[]): boolean {
  if (!allResults || allResults.length < 2) return false;
  
  // Get best result for each K
  const bestByK = new Map<number, number>();
  allResults.forEach(r => {
    const current = bestByK.get(r.K);
    if (!current || r.REL_ERR < current) {
      bestByK.set(r.K, r.REL_ERR);
    }
  });
  
  // Sort K values
  const sortedK = Array.from(bestByK.keys()).sort((a, b) => a - b);
  
  // Find the K of current result and previous K
  const currentK = result.K;
  const currentIdx = sortedK.indexOf(currentK);
  if (currentIdx <= 0) return false;
  
  const prevK = sortedK[currentIdx - 1];
  const prevErr = bestByK.get(prevK) || 1;
  const currErr = result.REL_ERR;
  
  // Jump = improvement of at least 2 orders of magnitude
  return prevErr / currErr >= 100;
}

// Identification criteria thresholds
const CR_THRESHOLD = 1.0;  // CR > 1.0 means good compression

export function ResultCard({ result, allResults = [] }: ResultCardProps) {
  const cr = getCompressionRatio(result);
  const probability = Math.pow(36, -result.K);  // 1/36^K
  const hasJump = hasAccuracyJump(result, allResults);
  
  // Criteria checks
  const crPassed = cr >= CR_THRESHOLD;
  const probPassed = probability < 1e-6;  // Very unlikely by chance
  const jumpPassed = hasJump;
  
  const allPassed = crPassed && probPassed && jumpPassed;
  const passedCount = [crPassed, probPassed, jumpPassed].filter(Boolean).length;

  return (
    <div className="p-4 sm:p-6 bg-linear-to-r from-[#0066cc]/5 to-transparent dark:from-[#0066cc]/10 border-b border-gray-200 dark:border-[#2a2a2e] overflow-hidden">
      <div className="max-w-6xl mx-auto">
        {/* Main result info */}
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 sm:gap-6 mb-5">
          <div className="col-span-1 sm:col-span-2 lg:col-span-1 overflow-hidden">
            <label className="text-[10px] sm:text-xs font-medium text-gray-500 dark:text-gray-500 uppercase tracking-wider">Best Match</label>
            <div className="text-lg sm:text-xl text-gray-900 dark:text-white mt-1 overflow-x-auto">
              <Latex formula={rpnToLatex(result.RPN)} />
            </div>
          </div>
          <div>
            <label className="text-[10px] sm:text-xs font-medium text-gray-500 dark:text-gray-500 uppercase tracking-wider">Numeric Value</label>
            <div className="text-base sm:text-lg font-mono text-gray-700 dark:text-gray-300 mt-1 break-all">{result.result}</div>
          </div>
          <div>
            <label className="text-[10px] sm:text-xs font-medium text-gray-500 dark:text-gray-500 uppercase tracking-wider">Mathematica</label>
            <a 
              href={createWolframLink(rpnToMathematica(result.RPN))}
              target="_blank"
              rel="noopener noreferrer"
              className="text-sm font-mono text-[#0066cc] hover:underline mt-1 block break-all"
            >
              {rpnToMathematica(result.RPN)}
            </a>
          </div>
          <div>
            <label className="text-[10px] sm:text-xs font-medium text-gray-500 dark:text-gray-500 uppercase tracking-wider">Relative Error</label>
            <div className="text-sm font-mono text-gray-500 dark:text-gray-500 mt-1">{result.REL_ERR.toExponential(2)}</div>
          </div>
        </div>
        
        {/* Identification Criteria - Scientific Style */}
        <div className="flex flex-col sm:flex-row items-start sm:items-center gap-4 pt-4 border-t border-gray-200/50 dark:border-[#2a2a2e]/50">
          <div className="flex items-center gap-2">
            <span className="text-[10px] font-semibold text-gray-400 dark:text-gray-500 uppercase tracking-widest">Identification</span>
            <div className={`px-2 py-0.5 rounded text-[10px] font-bold uppercase tracking-wide ${
              allPassed 
                ? 'bg-emerald-500/20 text-emerald-600 dark:text-emerald-400' 
                : passedCount >= 2 
                  ? 'bg-amber-500/20 text-amber-600 dark:text-amber-400'
                  : 'bg-gray-500/20 text-gray-500 dark:text-gray-400'
            }`}>
              {allPassed ? 'Confirmed' : passedCount >= 2 ? 'Likely' : 'Uncertain'}
            </div>
          </div>
          
          <div className="flex items-center gap-3 sm:gap-4 flex-wrap">
            {/* Accuracy Jump */}
            <div className="flex items-center gap-2 group">
              <div className={`w-2 h-2 rounded-full transition-all ${
                jumpPassed 
                  ? 'bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.6)]' 
                  : 'bg-gray-300 dark:bg-gray-600'
              }`} />
              <div className="flex flex-col">
                <span className={`text-[10px] uppercase tracking-wide font-medium ${
                  jumpPassed ? 'text-emerald-600 dark:text-emerald-400' : 'text-gray-400 dark:text-gray-500'
                }`}>
                  Accuracy Jump
                </span>
                <span className={`text-xs font-mono ${
                  jumpPassed ? 'text-emerald-700 dark:text-emerald-300' : 'text-gray-500 dark:text-gray-400'
                }`}>
                  {jumpPassed ? '>100x' : '—'}
                </span>
              </div>
            </div>

            <div className="w-px h-8 bg-gray-200 dark:bg-gray-700 hidden sm:block" />

            {/* Compression Ratio */}
            <div className="flex items-center gap-2 group">
              <div className={`w-2 h-2 rounded-full transition-all ${
                crPassed 
                  ? 'bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.6)]' 
                  : 'bg-gray-300 dark:bg-gray-600'
              }`} />
              <div className="flex flex-col">
                <span className={`text-[10px] uppercase tracking-wide font-medium ${
                  crPassed ? 'text-emerald-600 dark:text-emerald-400' : 'text-gray-400 dark:text-gray-500'
                }`}>
                  Compression
                </span>
                <span className={`text-xs font-mono ${
                  crPassed ? 'text-emerald-700 dark:text-emerald-300' : 'text-gray-500 dark:text-gray-400'
                }`}>
                  CR={cr.toFixed(2)}
                </span>
              </div>
            </div>

            <div className="w-px h-8 bg-gray-200 dark:bg-gray-700 hidden sm:block" />

            {/* Probability */}
            <div className="flex items-center gap-2 group">
              <div className={`w-2 h-2 rounded-full transition-all ${
                probPassed 
                  ? 'bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.6)]' 
                  : 'bg-gray-300 dark:bg-gray-600'
              }`} />
              <div className="flex flex-col">
                <span className={`text-[10px] uppercase tracking-wide font-medium ${
                  probPassed ? 'text-emerald-600 dark:text-emerald-400' : 'text-gray-400 dark:text-gray-500'
                }`}>
                  Probability
                </span>
                <span className={`text-xs font-mono ${
                  probPassed ? 'text-emerald-700 dark:text-emerald-300' : 'text-gray-500 dark:text-gray-400'
                }`}>
                  P={probability.toExponential(0)}
                </span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
