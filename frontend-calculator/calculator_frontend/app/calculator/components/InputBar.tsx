'use client';

interface InputBarProps {
  inputValue: string;
  setInputValue: (value: string) => void;
  isCalculating: boolean;
  onCalculate: () => void;
  onReset: () => void;
  onAbort: () => void;
}

export function InputBar({
  inputValue,
  setInputValue,
  isCalculating,
  onCalculate,
  onReset,
  onAbort
}: InputBarProps) {
  return (
    <div className="p-4 sm:p-6 bg-white dark:bg-[#1a1a1d] border-b border-gray-200 dark:border-[#2a2a2e]">
      <div className="max-w-4xl mx-auto">
        <div className="flex flex-col sm:flex-row gap-3 sm:gap-3">
          <div className="flex-1 relative">
            <input
              type="text"
              value={inputValue}
              onChange={(e) => setInputValue(e.target.value)}
              placeholder="Enter a number, e.g. 3.14159265..."
              className="w-full px-4 py-3 rounded-lg bg-gray-50 dark:bg-[#111113] border border-gray-200 dark:border-[#2a2a2e] text-gray-900 dark:text-white placeholder-gray-400 dark:placeholder-gray-600 focus:border-[#0066cc] focus:outline-none focus:ring-1 focus:ring-[#0066cc] font-mono text-lg"
              onKeyDown={(e) => e.key === 'Enter' && onCalculate()}
            />
          </div>
          <div className="flex gap-2">
            <button
              onClick={onCalculate}
              disabled={!inputValue || isCalculating}
              className="flex-1 sm:flex-none px-6 py-3 bg-[#0066cc] hover:bg-[#0052a3] disabled:bg-gray-300 dark:disabled:bg-gray-700 text-white font-semibold rounded-lg transition-colors flex items-center justify-center gap-2 disabled:cursor-not-allowed"
            >
              {isCalculating ? (
                <>
                  <svg className="w-5 h-5 animate-spin" fill="none" viewBox="0 0 24 24">
                    <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4" />
                    <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z" />
                  </svg>
                  <span>Searching...</span>
                </>
              ) : (
                <span>Find Formula</span>
              )}
            </button>
            {isCalculating ? (
              <button
                onClick={onAbort}
                className="px-4 py-3 bg-red-500 hover:bg-red-600 text-white font-medium rounded-lg transition-colors flex items-center justify-center"
                title="Stop search"
              >
                <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
                </svg>
              </button>
            ) : (
              <button
                onClick={onReset}
                className="px-4 py-3 bg-gray-200 dark:bg-[#2a2a2e] hover:bg-gray-300 dark:hover:bg-[#3a3a3e] text-gray-700 dark:text-gray-300 font-medium rounded-lg transition-colors flex items-center justify-center gap-2"
                title="Reset"
              >
                <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
                </svg>
                <span>Reset</span>
              </button>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}
