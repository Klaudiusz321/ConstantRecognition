export interface SearchResult {
  cpuId: number;
  K: number;
  RPN: string;
  result: string;
  REL_ERR: string;
  status?: string;
  results?: SearchResult[];
  compressionRatio?: number;
}

export interface Precision {
  z: string;
  deltaZ: string;
  relDeltaZ: string;
}

export interface Filters {
  successfullyIdentified: boolean;
  bestSoFar: boolean;
  bestForLengthK: boolean;
  runningIntermediate: boolean;
  abortedPointless: boolean;
}

export const defaultFilters: Filters = {
  successfullyIdentified: true,
  bestSoFar: false,
  bestForLengthK: false,
  runningIntermediate: false,
  abortedPointless: false
};
