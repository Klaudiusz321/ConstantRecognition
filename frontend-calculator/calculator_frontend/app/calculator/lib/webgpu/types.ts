/**
 * WebGPU Type Declarations
 * For environments without @webgpu/types
 */

declare global {
  interface Navigator {
    gpu?: GPU;
  }
  interface GPU {
    requestAdapter(options?: GPURequestAdapterOptions): Promise<GPUAdapter | null>;
  }
  interface GPURequestAdapterOptions {
    powerPreference?: 'low-power' | 'high-performance';
  }
  interface GPUAdapter {
    requestDevice(descriptor?: GPUDeviceDescriptor): Promise<GPUDevice>;
    requestAdapterInfo(): Promise<GPUAdapterInfo>;
    name?: string;
  }
  interface GPUAdapterInfo {
    vendor?: string;
    architecture?: string;
    device?: string;
    description?: string;
  }
  interface GPUDeviceDescriptor {
    requiredLimits?: Record<string, number>;
  }
  interface GPUDevice {
    createShaderModule(descriptor: { code: string }): GPUShaderModule;
    createComputePipeline(descriptor: GPUComputePipelineDescriptor): GPUComputePipeline;
    createBuffer(descriptor: GPUBufferDescriptor): GPUBuffer;
    createBindGroup(descriptor: GPUBindGroupDescriptor): GPUBindGroup;
    createCommandEncoder(): GPUCommandEncoder;
    queue: GPUQueue;
    limits: Record<string, number>;
    destroy(): void;
    lost: Promise<GPUDeviceLostInfo>;
  }
  interface GPUDeviceLostInfo {
    message: string;
    reason: 'destroyed' | 'unknown';
  }
  interface GPUShaderModule {}
  interface GPUComputePipelineDescriptor {
    layout: 'auto' | GPUPipelineLayout;
    compute: { module: GPUShaderModule; entryPoint: string };
  }
  interface GPUPipelineLayout {}
  interface GPUComputePipeline {
    getBindGroupLayout(index: number): GPUBindGroupLayout;
  }
  interface GPUBindGroupLayout {}
  interface GPUBufferDescriptor {
    size: number;
    usage: number;
  }
  interface GPUBuffer {
    destroy(): void;
    mapAsync(mode: number): Promise<void>;
    getMappedRange(): ArrayBuffer;
    unmap(): void;
    size: number;
  }
  interface GPUBindGroupDescriptor {
    layout: GPUBindGroupLayout;
    entries: GPUBindGroupEntry[];
  }
  interface GPUBindGroupEntry {
    binding: number;
    resource: { buffer: GPUBuffer };
  }
  interface GPUBindGroup {}
  interface GPUQueue {
    writeBuffer(buffer: GPUBuffer, offset: number, data: ArrayBuffer | ArrayBufferView): void;
    submit(commandBuffers: GPUCommandBuffer[]): void;
  }
  interface GPUCommandEncoder {
    beginComputePass(): GPUComputePassEncoder;
    copyBufferToBuffer(src: GPUBuffer, srcOffset: number, dst: GPUBuffer, dstOffset: number, size: number): void;
    finish(): GPUCommandBuffer;
  }
  interface GPUComputePassEncoder {
    setPipeline(pipeline: GPUComputePipeline): void;
    setBindGroup(index: number, bindGroup: GPUBindGroup): void;
    dispatchWorkgroups(x: number, y?: number, z?: number): void;
    end(): void;
  }
  interface GPUCommandBuffer {}
}

// GPUBufferUsage constants
export const GPUBufferUsage = {
  MAP_READ: 0x0001,
  MAP_WRITE: 0x0002,
  COPY_SRC: 0x0004,
  COPY_DST: 0x0008,
  INDEX: 0x0010,
  VERTEX: 0x0020,
  UNIFORM: 0x0040,
  STORAGE: 0x0080,
  INDIRECT: 0x0100,
  QUERY_RESOLVE: 0x0200,
};

// GPUMapMode constants
export const GPUMapMode = {
  READ: 0x0001,
  WRITE: 0x0002,
};

// Application interfaces
export interface GPUSearchResult {
  RPN: string;
  K: number;
  REL_ERR: number;
  error: number;
  status: string;
  cpuId: number;
}

export interface GPUInfo {
  supported: boolean;
  name: string;
  error?: string;
}

export interface FormDescriptor {
  ternary: number[];
  radix: number[];
  totalCombinations: number;
}

export interface Candidate {
  error: number;
  idx: number;
  K: number;
  form: number[];
  radix: number[];
}

export interface SearchOptions {
  minK?: number;
  maxK?: number;
  onProgress?: (info: { K: number; forms: number; evaluated: number }) => void;
  onResult?: (result: GPUSearchResult) => void;
}
