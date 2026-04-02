'use client';

import { useState, useEffect, useRef, useCallback } from 'react';
import { ConstantRecognitionGPU, GPUInfo, GPUSearchResult, getGPUInstance } from '../lib/webgpu';

interface UseWebGPUReturn {
  gpuAvailable: boolean;
  gpuInfo: GPUInfo | null;
  isInitializing: boolean;
  isSearching: boolean;
  search: (target: number, options?: SearchOptions) => Promise<GPUSearchResult[]>;
  abort: () => void;
}

interface SearchOptions {
  minK?: number;
  maxK?: number;
  onProgress?: (info: { K: number; forms: number; evaluated: number }) => void;
  onResult?: (result: GPUSearchResult) => void;
}

export function useWebGPU(): UseWebGPUReturn {
  const [gpuAvailable, setGpuAvailable] = useState(false);
  const [gpuInfo, setGpuInfo] = useState<GPUInfo | null>(null);
  const [isInitializing, setIsInitializing] = useState(true);
  const [isSearching, setIsSearching] = useState(false);
  
  const gpuRef = useRef<ConstantRecognitionGPU | null>(null);
  const abortedRef = useRef(false);

  useEffect(() => {
    const initGPU = async () => {
      console.log('[WebGPU] Checking support...');
      console.log('[WebGPU] navigator.gpu:', navigator.gpu);
      
      if (!ConstantRecognitionGPU.isSupported()) {
        console.warn('[WebGPU] Not supported in this browser');
        setGpuAvailable(false);
        setGpuInfo({ supported: false, name: '', error: 'WebGPU not supported in this browser. Try Chrome 113+ with WebGPU flag enabled.' });
        setIsInitializing(false);
        return;
      }

      try {
        console.log('[WebGPU] Requesting adapter...');
        gpuRef.current = getGPUInstance();
        const info = await gpuRef.current.init();
        
        console.log('[WebGPU] Init result:', info);
        setGpuAvailable(info.supported);
        setGpuInfo(info);
      } catch (err) {
        console.error('[WebGPU] Init error:', err);
        setGpuAvailable(false);
        setGpuInfo({
          supported: false,
          name: '',
          error: err instanceof Error ? err.message : 'Unknown error'
        });
      }
      
      setIsInitializing(false);
    };

    initGPU();

    return () => {
      // Don't destroy on unmount - singleton instance
    };
  }, []);

  const search = useCallback(async (
    target: number,
    options: SearchOptions = {}
  ): Promise<GPUSearchResult[]> => {
    if (!gpuRef.current?.isReady()) {
      throw new Error('WebGPU not available');
    }

    setIsSearching(true);
    abortedRef.current = false;

    try {
      const results = await gpuRef.current.search(target, {
        ...options,
        onProgress: (info) => {
          if (abortedRef.current) return;
          options.onProgress?.(info);
        },
        onResult: (result) => {
          if (abortedRef.current) return;
          options.onResult?.(result);
        }
      });

      return results;
    } finally {
      setIsSearching(false);
    }
  }, []);

  const abort = useCallback(() => {
    abortedRef.current = true;
    setIsSearching(false);
  }, []);

  return {
    gpuAvailable,
    gpuInfo,
    isInitializing,
    isSearching,
    search,
    abort
  };
}
