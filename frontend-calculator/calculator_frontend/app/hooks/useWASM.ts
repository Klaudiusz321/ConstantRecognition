'use client';

import { useState, useEffect } from 'react';
import { createMockWASM } from '../lib/mockWASM';

// Toggle between real WASM and mock
const USE_MOCK_WASM = true; // Zmień na false gdy będziesz miał prawdziwy WASM

interface WASMModule {
  ccall: (
    name: string,
    returnType: string,
    argTypes: string[],
    args: any[]
  ) => any;
}

interface SearchResult {
  RPN: string;
  Mathematica: string;
  Error: number;
  results?: any[];
}

export function useWASM() {
  const [Module, setModule] = useState<WASMModule | null>(null);
  const [isReady, setIsReady] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    let mounted = true;

    const loadWASM = async () => {
      try {
        if (USE_MOCK_WASM) {
          // Use mock WASM for development
          const mockModule = await createMockWASM();
          if (mounted) {
            setModule(mockModule as any);
            setIsReady(true);
            console.log('✅ Mock WASM module loaded successfully');
          }
          return;
        }

        // Real WASM loading (requires compiled files)
        const script = document.createElement('script');
        script.src = '/wasm/rpn_function.js';
        script.async = true;

        script.onload = () => {
          // @ts-ignore - WASM module is loaded globally
          if (window.Module) {
            // @ts-ignore
            window.Module.onRuntimeInitialized = () => {
              if (mounted) {
                // @ts-ignore
                setModule(window.Module);
                setIsReady(true);
                console.log('✅ WASM module loaded successfully');
              }
            };
          }
        };

        script.onerror = () => {
          if (mounted) {
            setError('Failed to load WASM module');
            console.error('❌ Failed to load WASM module');
          }
        };

        document.body.appendChild(script);

        return () => {
          mounted = false;
          document.body.removeChild(script);
        };
      } catch (err) {
        if (mounted) {
          setError('Error initializing WASM');
          console.error('❌ WASM initialization error:', err);
        }
      }
    };

    loadWASM();
  }, []);

  const searchRPN = async (
    z: number,
    deltaZ: number = 0,
    minCodeLength: number = 1,
    maxCodeLength: number = 5,
    cpuId: number = 0,
    ncpus: number = 1
  ): Promise<SearchResult | null> => {
    if (!Module || !isReady) {
      console.warn('⚠️ WASM module not ready');
      return null;
    }

    setIsLoading(true);
    setError(null);

    try {
      const resultString = Module.ccall(
        'search_RPN',
        'string',
        ['number', 'number', 'number', 'number', 'number', 'number'],
        [z, deltaZ, minCodeLength, maxCodeLength, cpuId, ncpus]
      );

      const result = JSON.parse(resultString) as SearchResult;
      console.log('🔢 Calculation result:', result);
      return result;
    } catch (err) {
      const errorMsg = err instanceof Error ? err.message : 'Calculation failed';
      setError(errorMsg);
      console.error('❌ Calculation error:', err);
      return null;
    } finally {
      setIsLoading(false);
    }
  };

  return {
    Module,
    isReady,
    isLoading,
    error,
    searchRPN,
  };
}
