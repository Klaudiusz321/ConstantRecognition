// Types and interfaces for the calculator

// Error mode for uncertainty handling
export type ErrorMode = 'zero' | 'automatic' | 'manual';

// Compute mode - Auto, CPU (WASM), or GPU (WebGPU)
export type ComputeMode = 'auto' | 'cpu' | 'gpu';

export interface SearchResult {
  cpuId: number;
  K: number;
  RPN: string;
  result: string;
  REL_ERR: number;
  status: string;
  compressionRatio?: number;
  fp32Evals?: number;
  fp64Evals?: number;
}

export interface Filters {
  maxRelErr: number | null;  // Max relative error threshold (e.g. 1e-10)
  minCR: number | null;      // Min compression ratio threshold
  searchQuery: string;       // Search in RPN/formula
  showSuccess: boolean;      // Show SUCCESS results
  showFailure: boolean;      // Show FAILURE results  
  showAborted: boolean;      // Show ABORTED results
}

export interface ErrorSettings {
  autoError: boolean;      // true = auto-detect from input precision
  customError: string;     // manual error value (e.g. "0.001" or "1e-6")
}

export interface Precision {
  z?: string;
  deltaZ?: string;
  relDeltaZ?: string;
}

export interface ActiveWorker {
  id: number;
  status: string;
  currentK: number;
}

export const defaultFilters: Filters = {
  maxRelErr: null,
  minCR: null,
  searchQuery: '',
  showSuccess: true,
  showFailure: true,
  showAborted: true,
};

export const defaultErrorSettings: ErrorSettings = {
  autoError: true,
  customError: '0',
};
export const examples = [
  {
    value: '1.17809724509617246442', 
    label: '\\displaystyle \\int_0^{\\infty} \\left(\\frac{\\sin(x)}{x}\\right)^3 \\, dx', 
    description: 'Borwein Integral (Sinc^3)'
  },
  {
    value: '0.5778636748955', 
    label: '\\displaystyle \\int_0^{\\infty} \\frac{\\cos(x)}{x^2 + 1} \\, dx', 
    description: 'Recall classic integrals'
  },
  {
    value: '0.51404189589007076139762973957688', 
    label: '\\displaystyle \\int\\limits_0^1 \\frac{\\arctan(\\sqrt{2+x^2})}{(x^2 + 1)\\sqrt{2+x^2}} \\, dx', 
    description: 'Fix Wolfram Mathematica failure'
  },
  {
    value: '-0.45158270528945',
    label: '\\displaystyle \\sum_{n=1}^{\\infty} (-1)^n \\ln{\\left( 1+\\frac{1}{n} \\right)}',
    description: 'Discover sum'
  },
  {
    value: '10.185916357881301489208560855841',
    label: '\\frac{3}{128} (56 \\sqrt{5}-125) _3F_2 \\left(\\frac{3}{2},\\frac{3}{2},\\frac{3}{2};2,2;\\frac{1}{128} (47-21 \\sqrt{5} ) \\right)+\\frac{4 (5 \\sqrt{5}-1) K(\\frac{1}{32} (16-7   \\sqrt{3}-\\sqrt{15}))^2}{\\pi^2}',
    description: 'Simplify expressions'
  }
];