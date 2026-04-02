// Mock WASM module for development
export const mockWASMModule = {
  ccall: (name: string, returnType: string, argTypes: string[], args: any[]) => {
    console.log('🔧 Mock WASM call:', { name, args });
    
    if (name === 'search_RPN') {
      const [z, deltaZ, minLength, maxLength, cpuId, ncpus] = args;
      
      // Symuluj wynik
      return JSON.stringify({
        RPN: 'PI, TWO, SQRT, PLUS',
        Mathematica: '(Pi + Sqrt[2])',
        Error: 1.23e-10,
        cpuId: cpuId,
        results: [],
      });
    }
    
    return '{}';
  },
};

export function createMockWASM() {
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve(mockWASMModule);
    }, 500); // Symuluj ładowanie
  });
}
