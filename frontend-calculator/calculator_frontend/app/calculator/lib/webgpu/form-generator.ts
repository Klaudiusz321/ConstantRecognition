/**
 * RPN Form Generator
 * Generates syntactically valid RPN forms for a given K
 */

import { N_CONST, N_UNARY, N_BINARY } from './rpn-evaluator';
import type { FormDescriptor } from './types';

/**
 * Check if a ternary form produces valid RPN syntax
 * Uses stack simulation to verify the form is valid
 * 
 * @param ternary - Array where 0=const (push), 1=unary (pop+push), 2=binary (pop+pop+push)
 * @returns true if the form produces exactly one result on the stack
 */
export function checkSyntax3(ternary: number[]): boolean {
  let stack = 0;
  for (const c of ternary) {
    switch (c) {
      case 0: stack++; break;          // Constant: push 1
      case 1: if (stack < 1) return false; break;  // Unary: pop 1, push 1 (net 0)
      case 2: if (stack < 2) return false; stack--; break;  // Binary: pop 2, push 1 (net -1)
    }
  }
  return stack === 1;
}

/**
 * Generate all syntactically valid RPN forms for a given K
 * 
 * @param K - Formula length (number of tokens)
 * @returns Array of valid form descriptors
 */
export function generateValidForms(K: number): FormDescriptor[] {
  const forms: FormDescriptor[] = [];
  const total = Math.pow(3, K);

  for (let i = 0; i < total; i++) {
    // Convert index to ternary representation
    const form: number[] = [];
    let val = i;
    for (let j = 0; j < K; j++) {
      form.push(val % 3);
      val = Math.floor(val / 3);
    }

    // Only include syntactically valid forms
    if (checkSyntax3(form)) {
      // Calculate radix for each position based on form type
      const radix = form.map(t => {
        if (t === 0) return N_CONST;   // 13 constants
        if (t === 1) return N_UNARY;   // 18 unary ops
        return N_BINARY;               // 5 binary ops
      });

      // Total combinations for this form
      const totalCombinations = radix.reduce((a, b) => a * b, 1);

      forms.push({ ternary: form, radix, totalCombinations });
    }
  }

  return forms;
}

/**
 * Get the total number of combinations for all valid forms at K
 * 
 * @param K - Formula length
 * @returns Total combinations across all valid forms
 */
export function getTotalCombinations(K: number): number {
  const forms = generateValidForms(K);
  return forms.reduce((sum, form) => sum + form.totalCombinations, 0);
}
