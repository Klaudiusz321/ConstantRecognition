/**
 * WebGPU Module - Public API
 * 
 * This module provides WebGPU-accelerated constant recognition.
 * 
 * Usage:
 *   import { ConstantRecognitionGPU, getGPUInstance, evaluateShortRPN } from './webgpu';
 */

// Re-export types
export type { 
  GPUSearchResult, 
  GPUInfo, 
  FormDescriptor, 
  Candidate,
  SearchOptions 
} from './types';

// Re-export RPN utilities
export { 
  evaluateShortRPN, 
  indexToRPN,
  CONST_CHARS,
  UNARY_CHARS,
  BINARY_CHARS,
  N_CONST,
  N_UNARY,
  N_BINARY
} from './rpn-evaluator';

// Re-export form generator
export { 
  generateValidForms, 
  checkSyntax3,
  getTotalCombinations 
} from './form-generator';

// Re-export GPU engine
export { 
  ConstantRecognitionGPU, 
  getGPUInstance 
} from './gpu-engine';
