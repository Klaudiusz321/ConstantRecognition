/**
 * WebGPU Compute Engine for Constant Recognition
 * Main GPU computation class
 */

import { GPUBufferUsage, GPUMapMode } from './types';
import type { GPUInfo, GPUSearchResult, FormDescriptor, Candidate, SearchOptions } from './types';
import { evaluateShortRPN, indexToRPN } from './rpn-evaluator';
import { generateValidForms } from './form-generator';

// Helper to add base path for production builds
const withBasePath = (path: string) => {
  const base = process.env.NEXT_PUBLIC_BASE_PATH?.replace(/\/+$/g, '') ?? '';
  const normalizedBase = base ? (base.startsWith('/') ? base : `/${base}`) : '';
  const normalizedPath = path.startsWith('/') ? path : `/${path}`;
  if (typeof window === 'undefined') return `${normalizedBase}${normalizedPath}`;
  return new URL(`${normalizedBase}${normalizedPath}`, window.location.origin).toString();
};

export class ConstantRecognitionGPU {
  private device: GPUDevice | null = null;
  private pipeline: GPUComputePipeline | null = null;
  private shaderModule: GPUShaderModule | null = null;
  private initialized = false;

  /**
   * Initialize WebGPU device and compile shader
   */
  async init(): Promise<GPUInfo> {
    if (!navigator.gpu) {
      return { supported: false, name: '', error: 'WebGPU not supported' };
    }

    try {
      const adapter = await navigator.gpu.requestAdapter({
        powerPreference: 'high-performance'
      });

      if (!adapter) {
        return { supported: false, name: '', error: 'No GPU adapter found' };
      }

      this.device = await adapter.requestDevice({
        requiredLimits: {
          maxStorageBufferBindingSize: 1024 * 1024 * 64, // 64MB max
          maxBufferSize: 1024 * 1024 * 64,
        }
      });

      // Handle device lost
      this.device.lost.then((info) => {
        console.error('[WebGPU] Device lost:', info.message);
        this.initialized = false;
        this.device = null;
      });

      // Load shader from public folder (with base path for production)
      const shaderResponse = await fetch(withBasePath('/wasm/rpn_shader.wgsl'));
      if (!shaderResponse.ok) {
        throw new Error(`Failed to load shader: ${shaderResponse.status} ${shaderResponse.statusText}`);
      }
      const shaderCode = await shaderResponse.text();
      
      // Verify we got WGSL, not HTML error page
      if (shaderCode.trim().startsWith('<!DOCTYPE') || shaderCode.trim().startsWith('<html')) {
        throw new Error('Shader file not found - received HTML instead of WGSL');
      }
      
      this.shaderModule = this.device.createShaderModule({ code: shaderCode });

      this.pipeline = this.device.createComputePipeline({
        layout: 'auto',
        compute: {
          module: this.shaderModule,
          entryPoint: 'main'
        }
      });

      this.initialized = true;

      // Get adapter info
      let gpuName = 'WebGPU Device';
      try {
        if (typeof adapter.requestAdapterInfo === 'function') {
          const info = await adapter.requestAdapterInfo();
          gpuName = info.description || info.device || info.vendor || 'WebGPU Device';
        } else if ((adapter as any).name) {
          gpuName = (adapter as any).name;
        }
      } catch (e) {
        console.warn('[WebGPU] Could not get adapter info:', e);
      }
      
      return { supported: true, name: gpuName };
    } catch (err) {
      return {
        supported: false,
        name: '',
        error: err instanceof Error ? err.message : 'Unknown error'
      };
    }
  }

  /**
   * Check if GPU is ready for computation
   */
  isReady(): boolean {
    return this.initialized && this.device !== null;
  }

  /**
   * Search for formulas matching target value
   */
  async search(target: number, options: SearchOptions = {}): Promise<GPUSearchResult[]> {
    if (!this.isReady()) {
      throw new Error('WebGPU not initialized');
    }

    const { minK = 1, maxK = 7, onProgress, onResult } = options;

    console.log('[GPU] Search params - target:', target, 'minK:', minK, 'maxK:', maxK);

    // FP32 threshold - larger because GPU uses FP32
    // Close matches will be verified in FP64 by CPU
    const relativeThreshold = 1e-4;
    const threshold = relativeThreshold * Math.max(Math.abs(target), 1.0);

    const allCandidates: Candidate[] = [];
    let totalEvaluated = 0;

    // Search each K level
    for (let K = minK; K <= maxK; K++) {
      const forms = generateValidForms(K);

      if (onProgress) {
        onProgress({ K, forms: forms.length, evaluated: totalEvaluated });
      }

      // Evaluate each form
      for (const form of forms) {
        const candidates = await this.evaluateForm(target, threshold, K, form);
        totalEvaluated += form.totalCombinations;

        // CPU verification in FP64
        for (const c of candidates) {
          const rpn = indexToRPN(c.idx, c.form, c.radix, c.K);
          const fp64Value = evaluateShortRPN(rpn);
          const fp64Error = Math.abs(fp64Value - target);
          
          if (!isFinite(fp64Value) || isNaN(fp64Value)) continue;
          
          c.error = fp64Error;
          allCandidates.push(c);
        }
      }
    }

    // Sort by error, then by K (prefer shorter formulas)
    allCandidates.sort((a, b) => {
      const errorDiff = a.error - b.error;
      if (Math.abs(errorDiff) > 1e-15) return errorDiff;
      return a.K - b.K;
    });

    // Deduplicate by value (keep shortest formula)
    const seenValues = new Map<string, boolean>();
    const uniqueCandidates = allCandidates.filter(c => {
      const rpn = indexToRPN(c.idx, c.form, c.radix, c.K);
      const valueKey = evaluateShortRPN(rpn).toPrecision(12);
      if (seenValues.has(valueKey)) return false;
      seenValues.set(valueKey, true);
      return true;
    });

    // Return top 100
    const top100 = uniqueCandidates.slice(0, 100);
    
    const results = top100.map((c, i) => {
      const rpn = indexToRPN(c.idx, c.form, c.radix, c.K);
      const fp64Error = Math.abs(evaluateShortRPN(rpn) - target);
      const result: GPUSearchResult = {
        RPN: rpn,
        K: c.K,
        REL_ERR: fp64Error / Math.abs(target),
        error: fp64Error,
        status: i === 0 ? 'SUCCESS' : 'GPU_VERIFIED',
        cpuId: 1
      };
      
      if (onResult) onResult(result);
      return result;
    });
    
    return results;
  }

  /**
   * Evaluate a single form (with batching for large forms)
   */
  private async evaluateForm(
    target: number,
    threshold: number,
    K: number,
    form: FormDescriptor
  ): Promise<Candidate[]> {
    if (!this.device || !this.pipeline) {
      throw new Error('GPU not ready');
    }

    const { totalCombinations } = form;
    const MAX_BATCH_SIZE = 1000000; // 1M per batch
    
    const allCandidates: Candidate[] = [];
    const numBatches = Math.ceil(totalCombinations / MAX_BATCH_SIZE);
    
    for (let batch = 0; batch < numBatches; batch++) {
      if (!this.device || !this.initialized) {
        console.error('[GPU] Device lost during computation');
        break;
      }
      
      const batchStart = batch * MAX_BATCH_SIZE;
      const batchSize = Math.min(MAX_BATCH_SIZE, totalCombinations - batchStart);
      
      try {
        const candidates = await this.evaluateFormBatch(
          target, threshold, K, form, batchStart, batchSize
        );
        allCandidates.push(...candidates);
        
        if (numBatches > 1) {
          await new Promise(r => setTimeout(r, 1));
        }
      } catch (err) {
        console.warn(`[GPU] Batch ${batch}/${numBatches} failed:`, err);
      }
    }
    
    return allCandidates;
  }

  /**
   * Evaluate a single batch of combinations on GPU
   */
  private async evaluateFormBatch(
    target: number,
    threshold: number,
    K: number,
    form: FormDescriptor,
    batchStart: number,
    batchSize: number
  ): Promise<Candidate[]> {
    if (!this.device || !this.pipeline) {
      throw new Error('GPU not ready');
    }

    const { ternary, radix } = form;
    
    const MAX_RESULTS = 1024;
    
    let paramsBuffer: GPUBuffer | null = null;
    let ternaryBuffer: GPUBuffer | null = null;
    let resultsBuffer: GPUBuffer | null = null;
    let counterBuffer: GPUBuffer | null = null;
    let readBuffer: GPUBuffer | null = null;
    let counterReadBuffer: GPUBuffer | null = null;
    
    try {
      // Create buffers
      paramsBuffer = this.device.createBuffer({
        size: 128,
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST
      });

      ternaryBuffer = this.device.createBuffer({
        size: Math.max(K * 4, 16),
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST
      });

      // Fixed size buffer for results (max 1024 results)
      resultsBuffer = this.device.createBuffer({
        size: MAX_RESULTS * 16,
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC
      });

      // Counter buffer for atomic operations
      counterBuffer = this.device.createBuffer({
        size: 4,
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC | GPUBufferUsage.COPY_DST
      });

      readBuffer = this.device.createBuffer({
        size: MAX_RESULTS * 16,
        usage: GPUBufferUsage.MAP_READ | GPUBufferUsage.COPY_DST
      });

      counterReadBuffer = this.device.createBuffer({
        size: 4,
        usage: GPUBufferUsage.MAP_READ | GPUBufferUsage.COPY_DST
      });

      // Prepare params buffer
      const paramsData = new ArrayBuffer(128);
      const paramsView = new DataView(paramsData);
      paramsView.setFloat32(0, target, true);
      paramsView.setFloat32(4, threshold, true);
      paramsView.setUint32(8, K, true);
      paramsView.setUint32(12, batchSize, true);
      
      // form_radix_0 at offset 16
      for (let i = 0; i < 4; i++) {
        paramsView.setUint32(16 + i * 4, radix[i] || 1, true);
      }
      // form_radix_1 at offset 32
      for (let i = 0; i < 4; i++) {
        paramsView.setUint32(32 + i * 4, radix[i + 4] || 1, true);
      }
      // form_radix_2 at offset 48
      for (let i = 0; i < 4; i++) {
        paramsView.setUint32(48 + i * 4, radix[i + 8] || 1, true);
      }
      // batch_start at offset 64
      paramsView.setUint32(64, batchStart, true);

      // Write buffers
      this.device.queue.writeBuffer(paramsBuffer, 0, paramsData);
      this.device.queue.writeBuffer(ternaryBuffer, 0, new Uint32Array(ternary));
      // Initialize counter to 0
      this.device.queue.writeBuffer(counterBuffer, 0, new Uint32Array([0]));

      // Create bind group
      const bindGroup = this.device.createBindGroup({
        layout: this.pipeline.getBindGroupLayout(0),
        entries: [
          { binding: 0, resource: { buffer: paramsBuffer } },
          { binding: 1, resource: { buffer: ternaryBuffer } },
          { binding: 2, resource: { buffer: resultsBuffer } },
          { binding: 3, resource: { buffer: counterBuffer } }
        ]
      });

      // Dispatch compute
      const commandEncoder = this.device.createCommandEncoder();
      const passEncoder = commandEncoder.beginComputePass();

      passEncoder.setPipeline(this.pipeline);
      passEncoder.setBindGroup(0, bindGroup);
      passEncoder.dispatchWorkgroups(Math.ceil(batchSize / 256));
      passEncoder.end();

      commandEncoder.copyBufferToBuffer(resultsBuffer, 0, readBuffer, 0, MAX_RESULTS * 16);
      commandEncoder.copyBufferToBuffer(counterBuffer, 0, counterReadBuffer, 0, 4);
      this.device.queue.submit([commandEncoder.finish()]);

      // Read counter first to know how many results we have
      await counterReadBuffer.mapAsync(GPUMapMode.READ);
      const counterData = new Uint32Array(counterReadBuffer.getMappedRange());
      const resultCount = Math.min(counterData[0], MAX_RESULTS);
      counterReadBuffer.unmap();

      // Read results
      await readBuffer.mapAsync(GPUMapMode.READ);
      const mappedRange = readBuffer.getMappedRange();
      const dataView = new DataView(mappedRange);

      const candidates: Candidate[] = [];
      for (let i = 0; i < resultCount; i++) {
        const byteOffset = i * 16;
        const error = dataView.getFloat32(byteOffset, true);
        const idx = dataView.getUint32(byteOffset + 4, true);
        const valid = dataView.getUint32(byteOffset + 8, true);

        if (valid === 1 && error < threshold) {
          candidates.push({
            error,
            idx,
            K,
            form: [...ternary],
            radix: [...radix]
          });
        }
      }

      readBuffer.unmap();
      return candidates;
      
    } finally {
      try {
        paramsBuffer?.destroy();
        ternaryBuffer?.destroy();
        resultsBuffer?.destroy();
        counterBuffer?.destroy();
        readBuffer?.destroy();
        counterReadBuffer?.destroy();
      } catch (e) {
        // Ignore cleanup errors
      }
    }
  }

  /**
   * Destroy GPU resources
   */
  destroy(): void {
    if (this.device) {
      this.device.destroy();
      this.device = null;
    }
    this.initialized = false;
  }

  /**
   * Check if WebGPU is supported in this environment
   */
  static isSupported(): boolean {
    return typeof navigator !== 'undefined' && !!navigator.gpu;
  }
}

// Singleton instance
let gpuInstance: ConstantRecognitionGPU | null = null;

export function getGPUInstance(): ConstantRecognitionGPU {
  if (!gpuInstance) {
    gpuInstance = new ConstantRecognitionGPU();
  }
  return gpuInstance;
}
