/**
 * WebGPU Module - Backward Compatibility Layer
 * 
 * This file maintains backward compatibility for existing imports.
 * The actual implementation is now in ./webgpu/ folder:
 * 
 *   - types.ts       - TypeScript types and interfaces
 *   - rpn-evaluator.ts - Short-form RPN evaluation
 *   - form-generator.ts - Valid RPN form generation
 *   - gpu-engine.ts  - Main GPU computation class
 *   - index.ts       - Public API exports
 */

export * from './webgpu/index';
