'use client';

import { useState, useEffect, useRef, useCallback } from 'react';
import Link from 'next/link';

// Mathematical particle for floating animation
interface Particle {
  id: number;
  x: number;
  y: number;
  size: number;
  symbol: string;
  speed: number;
  opacity: number;
  rotation: number;
  rotationSpeed: number;
}

// Equation floating in background
interface FloatingEquation {
  id: number;
  x: number;
  y: number;
  equation: string;
  speed: number;
  opacity: number;
}

// Pure mathematical symbols
const mathSymbols = [
  'π', 'e', 'φ', '∞', '∫', '∑', '∏', '√', '∂', '∇',
  'Γ', 'ζ', 'sin', 'cos', 'tan', 'ln', 'log', 'exp',
  '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '+', '−', '×', '÷', '=', '^', '(', ')'
];

// Mathematical equations/formulas
const equations = [
  'e^(iπ) + 1 = 0',
  'Γ(n) = (n-1)!',
  'φ = (1+√5)/2',
  'ζ(2) = π²/6',
  'sin²x + cos²x = 1',
  'e = lim(1+1/n)^n',
  '∫e^x dx = e^x',
  'd/dx sin(x) = cos(x)',
  'ln(e) = 1',
  'π ≈ 3.14159...',
  'e ≈ 2.71828...',
  'φ ≈ 1.61803...'
];

export default function LandingPage() {
  const [particles, setParticles] = useState<Particle[]>([]);
  const [floatingEqs, setFloatingEqs] = useState<FloatingEquation[]>([]);
  const [mousePos, setMousePos] = useState({ x: 0, y: 0 });
  const [isLoaded, setIsLoaded] = useState(false);
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const animationRef = useRef<number | undefined>(undefined);

  // Initialize particles and equations
  useEffect(() => {
    const newParticles: Particle[] = [];
    for (let i = 0; i < 60; i++) {
      newParticles.push({
        id: i,
        x: Math.random() * 100,
        y: Math.random() * 100,
        size: Math.random() * 28 + 14,
        symbol: mathSymbols[Math.floor(Math.random() * mathSymbols.length)],
        speed: Math.random() * 0.15 + 0.03,
        opacity: Math.random() * 0.12 + 0.03,
        rotation: Math.random() * 360,
        rotationSpeed: (Math.random() - 0.5) * 0.5
      });
    }
    setParticles(newParticles);

    const newEqs: FloatingEquation[] = [];
    for (let i = 0; i < 8; i++) {
      newEqs.push({
        id: i,
        x: Math.random() * 80 + 10,
        y: Math.random() * 100,
        equation: equations[Math.floor(Math.random() * equations.length)],
        speed: Math.random() * 0.08 + 0.02,
        opacity: Math.random() * 0.08 + 0.02
      });
    }
    setFloatingEqs(newEqs);

    setIsLoaded(true);
  }, []);

  // Animate particles
  useEffect(() => {
    const interval = setInterval(() => {
      setParticles(prev => prev.map(p => ({
        ...p,
        y: p.y - p.speed < -10 ? 110 : p.y - p.speed,
        x: p.x + Math.sin(p.y * 0.02) * 0.03,
        rotation: p.rotation + p.rotationSpeed
      })));
      setFloatingEqs(prev => prev.map(eq => ({
        ...eq,
        y: eq.y - eq.speed < -5 ? 105 : eq.y - eq.speed
      })));
    }, 50);
    return () => clearInterval(interval);
  }, []);

  // Mouse parallax effect
  const handleMouseMove = useCallback((e: React.MouseEvent) => {
    setMousePos({
      x: (e.clientX / window.innerWidth - 0.5) * 12,
      y: (e.clientY / window.innerHeight - 0.5) * 12
    });
  }, []);

  // Canvas background with grid and connections
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const resize = () => {
      canvas.width = window.innerWidth;
      canvas.height = window.innerHeight;
    };
    resize();
    window.addEventListener('resize', resize);

    // Grid dots
    const nodes: { x: number; y: number; vx: number; vy: number; baseX: number; baseY: number }[] = [];
    const gridSize = 80;
    for (let x = 0; x < canvas.width + gridSize; x += gridSize) {
      for (let y = 0; y < canvas.height + gridSize; y += gridSize) {
        nodes.push({
          x: x + (Math.random() - 0.5) * 20,
          y: y + (Math.random() - 0.5) * 20,
          baseX: x,
          baseY: y,
          vx: (Math.random() - 0.5) * 0.2,
          vy: (Math.random() - 0.5) * 0.2
        });
      }
    }

    const animate = () => {
      // Dark background matching calculator (#0a0a0b is close to gray-950)
      ctx.fillStyle = 'rgba(10, 10, 11, 0.08)';
      ctx.fillRect(0, 0, canvas.width, canvas.height);

      nodes.forEach((node, i) => {
        // Gentle floating motion
        node.x += node.vx;
        node.y += node.vy;
        
        // Return to base position
        node.vx += (node.baseX - node.x) * 0.001;
        node.vy += (node.baseY - node.y) * 0.001;
        
        // Damping
        node.vx *= 0.99;
        node.vy *= 0.99;

        // Draw connections to nearby nodes (subtle blue-gray matching app)
        nodes.forEach((other, j) => {
          if (i >= j) return;
          const dx = other.x - node.x;
          const dy = other.y - node.y;
          const dist = Math.sqrt(dx * dx + dy * dy);

          if (dist < 100) {
            const opacity = (1 - dist / 100) * 0.04;
            ctx.strokeStyle = `rgba(100, 116, 139, ${opacity})`; // slate-500
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.moveTo(node.x, node.y);
            ctx.lineTo(other.x, other.y);
            ctx.stroke();
          }
        });

        // Draw node (subtle gray dot)
        ctx.fillStyle = 'rgba(100, 116, 139, 0.15)';
        ctx.beginPath();
        ctx.arc(node.x, node.y, 1, 0, Math.PI * 2);
        ctx.fill();
      });

      animationRef.current = requestAnimationFrame(animate);
    };

    animate();

    return () => {
      window.removeEventListener('resize', resize);
      if (animationRef.current) cancelAnimationFrame(animationRef.current);
    };
  }, []);

  return (
    <div 
      className="h-screen w-screen bg-[#0a0a0b] text-white overflow-hidden relative flex items-center justify-center"
      onMouseMove={handleMouseMove}
    >
      {/* Canvas background */}
      <canvas 
        ref={canvasRef} 
        className="fixed inset-0 z-0"
      />

      {/* Floating equations - more visible */}
      <div className="fixed inset-0 z-1 pointer-events-none overflow-hidden">
        {floatingEqs.map(eq => (
          <div
            key={eq.id}
            className="absolute font-mono text-slate-500 whitespace-nowrap"
            style={{
              left: `${eq.x}%`,
              top: `${eq.y}%`,
              fontSize: '16px',
              opacity: eq.opacity + 0.08,
              transform: `translate(${mousePos.x * 0.02}px, ${mousePos.y * 0.02}px)`,
            }}
          >
            {eq.equation}
          </div>
        ))}
      </div>

      {/* Floating math symbols - more visible */}
      <div className="fixed inset-0 z-2 pointer-events-none overflow-hidden">
        {particles.map(particle => (
          <div
            key={particle.id}
            className="absolute font-mono text-slate-400"
            style={{
              left: `${particle.x}%`,
              top: `${particle.y}%`,
              fontSize: `${particle.size}px`,
              opacity: particle.opacity + 0.05,
              transform: `rotate(${particle.rotation}deg) translate(${mousePos.x * 0.04}px, ${mousePos.y * 0.04}px)`,
            }}
          >
            {particle.symbol}
          </div>
        ))}
      </div>

      {/* Decorative math elements - top corners */}
      <div className="fixed top-8 left-8 z-3 font-mono text-slate-600 text-sm opacity-40">
        <div>∫₀^∞ e^(-x²) dx = √π/2</div>
      </div>
      <div className="fixed top-8 right-8 z-3 font-mono text-slate-600 text-sm opacity-40 text-right">
        <div>Γ(½) = √π</div>
      </div>

      {/* Footer */}
      <footer className="fixed bottom-0 left-0 right-0 z-10 py-4 px-6">
        <div className="flex items-center justify-between text-slate-500 text-sm">
          <div>
            © Andrzej Odrzywołek & Klaudiusz Sroka, UJ 2025
          </div>
          <a 
            href="https://github.com/Klaudiusz321/ConstantRecognition" 
            target="_blank" 
            rel="noopener noreferrer"
            className="flex items-center gap-2 hover:text-white transition-colors"
          >
            <svg className="w-5 h-5" fill="currentColor" viewBox="0 0 24 24">
              <path fillRule="evenodd" clipRule="evenodd" d="M12 2C6.477 2 2 6.477 2 12c0 4.42 2.865 8.17 6.839 9.49.5.092.682-.217.682-.482 0-.237-.008-.866-.013-1.7-2.782.604-3.369-1.34-3.369-1.34-.454-1.156-1.11-1.464-1.11-1.464-.908-.62.069-.608.069-.608 1.003.07 1.531 1.03 1.531 1.03.892 1.529 2.341 1.087 2.91.831.092-.646.35-1.086.636-1.336-2.22-.253-4.555-1.11-4.555-4.943 0-1.091.39-1.984 1.029-2.683-.103-.253-.446-1.27.098-2.647 0 0 .84-.269 2.75 1.025A9.578 9.578 0 0112 6.836c.85.004 1.705.115 2.504.337 1.909-1.294 2.747-1.025 2.747-1.025.546 1.377.203 2.394.1 2.647.64.699 1.028 1.592 1.028 2.683 0 3.842-2.339 4.687-4.566 4.935.359.309.678.919.678 1.852 0 1.336-.012 2.415-.012 2.743 0 .267.18.578.688.48C19.138 20.167 22 16.418 22 12c0-5.523-4.477-10-10-10z" />
            </svg>
            GitHub
          </a>
        </div>
      </footer>

      {/* Center content */}
      <div 
        className="relative z-10 text-center px-6"
        style={{
          transform: `translate(${mousePos.x * 0.4}px, ${mousePos.y * 0.4}px)`
        }}
      >
        {/* Logo with math symbols orbit */}
        <div className={`mb-10 transition-all duration-1000 ${isLoaded ? 'opacity-100 scale-100' : 'opacity-0 scale-90'}`}>
          <div className="relative inline-block">
            {/* Orbiting symbols */}
            <div className="absolute inset-0 animate-spin" style={{ animationDuration: '20s' }}>
              <span className="absolute -top-3 left-1/2 -translate-x-1/2 text-slate-500 text-lg">e</span>
              <span className="absolute top-1/2 -right-3 -translate-y-1/2 text-slate-500 text-lg">φ</span>
              <span className="absolute -bottom-3 left-1/2 -translate-x-1/2 text-slate-500 text-lg">∞</span>
              <span className="absolute top-1/2 -left-3 -translate-y-1/2 text-slate-500 text-lg">Γ</span>
            </div>
            {/* Main logo */}
            <div className="w-28 h-28 rounded-3xl bg-[#1a1a1c] border border-slate-800 flex items-center justify-center shadow-2xl overflow-hidden">
              <img 
                src="favicon-192.png" 
                alt="Logo" 
                className="w-20 h-20 object-cover"
              />
            </div>
          </div>
        </div>

        {/* Title */}
        <h1 className={`text-5xl md:text-7xl lg:text-8xl font-black mb-6 transition-all duration-1000 delay-200 ${isLoaded ? 'opacity-100 translate-y-0' : 'opacity-0 translate-y-8'}`}>
          <span className="text-white">
            Constant
          </span>
          <br />
          <span className="text-slate-400">
            Recognition
          </span>
        </h1>

        {/* Tagline with math decoration */}
        <div className={`mb-12 transition-all duration-1000 delay-400 ${isLoaded ? 'opacity-100 translate-y-0' : 'opacity-0 translate-y-8'}`}>
          <p className="text-lg md:text-xl text-slate-500 max-w-lg mx-auto">
            <span className="text-slate-600 font-mono">3.14159...</span> → <span className="text-white font-mono">π</span>
          </p>
          <p className="text-sm text-slate-600 mt-2">
            Find the mathematical formula behind any number
          </p>
        </div>

        {/* CTA Button */}
        <div className={`transition-all duration-1000 delay-600 ${isLoaded ? 'opacity-100 translate-y-0' : 'opacity-0 translate-y-8'}`}>
          <Link href="/calculator">
            <button className="group relative px-10 py-4 text-lg font-semibold rounded-xl bg-[#1a1a1c] border border-slate-700 hover:border-slate-500 hover:bg-[#2a2a2e] transition-all duration-300 shadow-xl hover:shadow-2xl hover:scale-105">
              <span className="flex items-center gap-3 text-white">
                Enter Calculator
                <svg className="w-5 h-5 group-hover:translate-x-1 transition-transform text-slate-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 7l5 5m0 0l-5 5m5-5H6" />
                </svg>
              </span>
            </button>
          </Link>
        </div>

        {/* Bottom info */}
        <div className={`mt-12 transition-all duration-1000 delay-800 ${isLoaded ? 'opacity-100' : 'opacity-0'}`}>
          <div className="flex items-center justify-center gap-4 text-slate-600 text-xs">
            <span className="flex items-center gap-1.5">
              <span className="w-1.5 h-1.5 rounded-full bg-green-500/60"></span>
              WebAssembly
            </span>
            <span className="text-slate-700">•</span>
            <span>36 Operations</span>
            <span className="text-slate-700">•</span>
            <span>Parallel Processing</span>
          </div>
        </div>
      </div>
    </div>
  );
}

