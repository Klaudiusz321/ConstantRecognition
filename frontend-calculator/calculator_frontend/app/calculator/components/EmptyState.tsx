'use client';
import { examples } from '../lib/types';
import { Latex } from './Latex';


interface EmptyStateProps {
  onExampleClick: (value: string) => void;
}

export function EmptyState({ onExampleClick }: EmptyStateProps) {


  return (
    <div className="flex-1 flex items-center justify-center">
      <div className="text-center max-w-md px-4">
        <div className="w-16 h-16 mx-auto mb-6 rounded-full bg-gray-100 dark:bg-[#2a2a2e] flex items-center justify-center">
          <svg className="w-8 h-8 text-gray-400 dark:text-gray-500" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M9 7h6m0 10v-3m-3 3h.01M9 17h.01M9 14h.01M12 14h.01M15 11h.01M12 11h.01M9 11h.01M7 21h10a2 2 0 002-2V5a2 2 0 00-2-2H7a2 2 0 00-2 2v14a2 2 0 002 2z" />
          </svg>
        </div>
        <h2 className="text-xl font-semibold text-gray-900 dark:text-white mb-2">
          Ready to Identify
        </h2>
        <p className="text-sm text-gray-500 dark:text-gray-500 mb-6">
          Enter a decimal number above to search for matching mathematical expressions using 36 operations.
        </p>
        <div className="flex flex-wrap gap-2 justify-center">
          {examples.map(ex => (
            <button
              key={ex.value}
              onClick={() => onExampleClick(ex.value)}
              className="px-4 py-2 text-sm bg-gray-100 dark:bg-[#2a2a2e] hover:bg-gray-200 dark:hover:bg-[#3a3a3e] text-gray-700 dark:text-gray-300 rounded-lg transition-colors group relative"
              title={ex.description}
            >
              <Latex formula={ex.label} />
              <span className="absolute -bottom-8 left-1/2 -translate-x-1/2 px-2 py-1 bg-gray-800 text-white text-xs rounded opacity-0 group-hover:opacity-100 transition-opacity whitespace-nowrap pointer-events-none">
                {ex.description}
              </span>
            </button>
          ))}
        </div>
      </div>
    </div>
  );
}
