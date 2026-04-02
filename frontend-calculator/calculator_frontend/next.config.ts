import type { NextConfig } from "next";


  
  // Accepts values like "/constant" or "constant/" and normalizes them to "/constant".
  // The regex strips leading/trailing slashes so we can add exactly one leading slash back.
  const rawBasePath = process.env.NEXT_PUBLIC_BASE_PATH;
  const normalizedBasePath = rawBasePath
    ? `/${rawBasePath.replace(/^\/+|\/+$/g, "")}`
    : undefined;

  const nextConfig: NextConfig = {

  
    // Generate a static export in `out/` when running `next build`
    output: "export",
    // Keep all asset URLs relative to an optional base path (or the current folder)
    basePath: normalizedBasePath,
    assetPrefix: normalizedBasePath ?? "./",
    trailingSlash: true,
    images: {
      unoptimized: true,
    },
    
  // Disable dev indicators that might cause layout issues
  devIndicators: false,
  
  // Pusta konfiguracja Turbopack (wystarczy dla większości przypadków)
  turbopack: {},
  
  // Zachowaj webpack dla fallback (jeśli ktoś użyje --webpack)
  webpack: (config, { isServer }) => {
    // Enable WebAssembly support
    config.experiments = {
      ...config.experiments,
      asyncWebAssembly: true,
      layers: true,
    };

    // Handle .wasm files
    config.module.rules.push({
      test: /\.wasm$/,
      type: "webassembly/async",
    });

    // Exclude WASM from server-side rendering
    if (isServer) {
      config.externals = config.externals || [];
    }

    return config;
  },
};

export default nextConfig;
