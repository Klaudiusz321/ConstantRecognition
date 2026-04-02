'use client';

import { useEffect, useState } from 'react';
import katex from 'katex';

interface LatexProps {
  formula: string;
  className?: string;
}

export function Latex({ formula, className = '' }: LatexProps) {
  const [html, setHtml] = useState<string>('');

  useEffect(() => {
    try {
      const rendered = katex.renderToString(formula, {
        throwOnError: false,
        displayMode: false,
        trust: true,
        strict: false,
      });
      setHtml(rendered);
    } catch {
      setHtml(formula);
    }
  }, [formula]);

  if (!html) {
    return <span className={className}>{formula}</span>;
  }

  return (
    <span 
      className={className}
      dangerouslySetInnerHTML={{ __html: html }} 
    />
  );
}
