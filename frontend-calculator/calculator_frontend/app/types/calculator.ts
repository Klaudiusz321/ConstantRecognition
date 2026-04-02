// Typy przycisków kalkulatora
export type ButtonType = 'constant' | 'unary' | 'binary';

export interface CalculatorButton {
  id: string;
  label: string;
  type: ButtonType;
  code: string; // Kod RPN (np. 'PI', 'LOG', 'PLUS')
  description?: string;
  category?: 'constants' | 'arithmetic' | 'trigonometry' | 'special';
  color?: string;
}

// Konfiguracja kalkulatora
export interface CalculatorConfig {
  id: string;
  name: string;
  description: string;
  author?: string;
  version: string;
  buttons: CalculatorButton[];
  layout?: {
    rows: number;
    columns: number;
  };
  encoding?: {
    base: number; // np. 36 dla 0-9,a-z
    mapping: Record<string, string>; // np. {'0': 'PI', '1': 'EULER'}
  };
}

// Preset kalkulatorów
export const PRESET_CALCULATORS: Record<string, CalculatorConfig> = {
  casio: {
    id: 'casio-hl-815l',
    name: 'Casio HL-815L',
    description: '36-button scientific calculator',
    version: '1.0',
    buttons: [
      // Constants (stack pushers)
      { id: '0', label: 'π', type: 'constant', code: 'PI', category: 'constants', color: '#3B82F6' },
      { id: '1', label: 'e', type: 'constant', code: 'EULER', category: 'constants', color: '#3B82F6' },
      { id: '2', label: '-1', type: 'constant', code: 'NEG', category: 'constants', color: '#3B82F6' },
      { id: '3', label: 'φ', type: 'constant', code: 'GOLDENRATIO', category: 'constants', color: '#3B82F6' },
      { id: 'o', label: '1', type: 'constant', code: 'ONE', category: 'constants', color: '#6B7280' },
      { id: 'p', label: '2', type: 'constant', code: 'TWO', category: 'constants', color: '#6B7280' },
      { id: 'q', label: '3', type: 'constant', code: 'THREE', category: 'constants', color: '#6B7280' },
      { id: 'r', label: '4', type: 'constant', code: 'FOUR', category: 'constants', color: '#6B7280' },
      { id: 's', label: '5', type: 'constant', code: 'FIVE', category: 'constants', color: '#6B7280' },
      { id: 't', label: '6', type: 'constant', code: 'SIX', category: 'constants', color: '#6B7280' },
      { id: 'u', label: '7', type: 'constant', code: 'SEVEN', category: 'constants', color: '#6B7280' },
      { id: 'v', label: '8', type: 'constant', code: 'EIGHT', category: 'constants', color: '#6B7280' },
      { id: 'w', label: '9', type: 'constant', code: 'NINE', category: 'constants', color: '#6B7280' },
      
      // Unary functions
      { id: '4', label: 'log', type: 'unary', code: 'LOG', category: 'special', color: '#10B981' },
      { id: '5', label: 'exp', type: 'unary', code: 'EXP', category: 'special', color: '#10B981' },
      { id: '8', label: '1/x', type: 'unary', code: 'INV', category: 'arithmetic', color: '#F59E0B' },
      { id: '9', label: 'Γ', type: 'unary', code: 'GAMMA', category: 'special', color: '#10B981' },
      { id: 'a', label: '√', type: 'unary', code: 'SQRT', category: 'arithmetic', color: '#F59E0B' },
      { id: 'b', label: 'x²', type: 'unary', code: 'SQR', category: 'arithmetic', color: '#F59E0B' },
      
      // Trigonometric
      { id: 'c', label: 'sin', type: 'unary', code: 'SIN', category: 'trigonometry', color: '#8B5CF6' },
      { id: 'd', label: 'asin', type: 'unary', code: 'ARCSIN', category: 'trigonometry', color: '#8B5CF6' },
      { id: 'e', label: 'cos', type: 'unary', code: 'COS', category: 'trigonometry', color: '#8B5CF6' },
      { id: 'f', label: 'acos', type: 'unary', code: 'ARCCOS', category: 'trigonometry', color: '#8B5CF6' },
      { id: 'g', label: 'tan', type: 'unary', code: 'TAN', category: 'trigonometry', color: '#8B5CF6' },
      { id: 'h', label: 'atan', type: 'unary', code: 'ARCTAN', category: 'trigonometry', color: '#8B5CF6' },
      
      // Hyperbolic
      { id: 'i', label: 'sinh', type: 'unary', code: 'SINH', category: 'trigonometry', color: '#8B5CF6' },
      { id: 'j', label: 'asinh', type: 'unary', code: 'ARCSINH', category: 'trigonometry', color: '#8B5CF6' },
      { id: 'k', label: 'cosh', type: 'unary', code: 'COSH', category: 'trigonometry', color: '#8B5CF6' },
      { id: 'l', label: 'acosh', type: 'unary', code: 'ARCCOSH', category: 'trigonometry', color: '#8B5CF6' },
      { id: 'm', label: 'tanh', type: 'unary', code: 'TANH', category: 'trigonometry', color: '#8B5CF6' },
      { id: 'n', label: 'atanh', type: 'unary', code: 'ARCTANH', category: 'trigonometry', color: '#8B5CF6' },
      
      // Binary operators
      { id: '6', label: '+', type: 'binary', code: 'PLUS', category: 'arithmetic', color: '#EF4444' },
      { id: '7', label: '×', type: 'binary', code: 'TIMES', category: 'arithmetic', color: '#EF4444' },
      { id: 'x', label: '−', type: 'binary', code: 'SUBTRACT', category: 'arithmetic', color: '#EF4444' },
      { id: 'y', label: '÷', type: 'binary', code: 'DIVIDE', category: 'arithmetic', color: '#EF4444' },
      { id: 'z', label: '^', type: 'binary', code: 'POWER', category: 'arithmetic', color: '#EF4444' },
    ],
    layout: {
      rows: 6,
      columns: 6,
    },
    encoding: {
      base: 36,
      mapping: {}, // Will be auto-generated from buttons
    },
  },
};

// Complete base-36 encoding mapping from C source
// {0->PI, 1->EULER, 2->NEG, 3->GOLDENRATIO, 4->LOG, 5->EXP, 6->PLUS, 7->TIMES, 
//  8->INV, 9->GAMMA, a->SQRT, b->SQR, c->SIN, d->ARCSIN, e->COS, f->ARCCOS, 
//  g->TAN, h->ARCTAN, i->SINH, j->ARCSINH, k->COSH, l->ARCCOSH, m->TANH, 
//  n->ARCTANH, o->ONE, p->TWO, q->THREE, r->FOUR, s->FIVE, t->SIX, u->SEVEN, 
//  v->EIGHT, w->NINE, x->SUBTRACT, y->DIVIDE, z->POWER}
export const BASE36_ENCODING_MAP: Record<string, string> = {
  '0': 'PI',
  '1': 'EULER',
  '2': 'NEG',
  '3': 'GOLDENRATIO',
  '4': 'LOG',
  '5': 'EXP',
  '6': 'PLUS',
  '7': 'TIMES',
  '8': 'INV',
  '9': 'GAMMA',
  'a': 'SQRT',
  'b': 'SQR',
  'c': 'SIN',
  'd': 'ARCSIN',
  'e': 'COS',
  'f': 'ARCCOS',
  'g': 'TAN',
  'h': 'ARCTAN',
  'i': 'SINH',
  'j': 'ARCSINH',
  'k': 'COSH',
  'l': 'ARCCOSH',
  'm': 'TANH',
  'n': 'ARCTANH',
  'o': 'ONE',
  'p': 'TWO',
  'q': 'THREE',
  'r': 'FOUR',
  's': 'FIVE',
  't': 'SIX',
  'u': 'SEVEN',
  'v': 'EIGHT',
  'w': 'NINE',
  'x': 'SUBTRACT',
  'y': 'DIVIDE',
  'z': 'POWER',
};

// Reverse mapping: CODE -> base36 character
export const CODE_TO_BASE36: Record<string, string> = Object.fromEntries(
  Object.entries(BASE36_ENCODING_MAP).map(([k, v]) => [v, k])
);

// Helper: Get button by base-36 code
export function getButtonByCode(code: string): CalculatorButton | undefined {
  return PRESET_CALCULATORS.casio.buttons.find(b => b.id === code);
}

// Helper: Get button by RPN code name
export function getButtonByRPNCode(rpnCode: string): CalculatorButton | undefined {
  return PRESET_CALCULATORS.casio.buttons.find(b => b.code === rpnCode);
}

// Helper: Convert RPN code sequence to base-36 string
export function rpnToBase36(rpnCodes: string[]): string {
  return rpnCodes.map(code => CODE_TO_BASE36[code] || '?').join('');
}

// Helper: Convert base-36 string to RPN code array
export function base36ToRPN(base36: string): string[] {
  return base36.split('').map(char => BASE36_ENCODING_MAP[char] || 'UNKNOWN');
}

