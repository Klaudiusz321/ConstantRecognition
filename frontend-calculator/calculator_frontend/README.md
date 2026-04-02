# Calculator Frontend

This is a Next.js 16 frontend for the constant recognizer. The project is configured for **static export**, so running `npm run build` emits a fully static site in the `out/` directory that can be served by any plain HTTP server (no Node.js runtime needed at deploy time).

## Development
- Install dependencies: `npm install`
- Start the dev server: `npm run dev`

## Static production build
- Build the site: `npm run build`
  - The static files are written to `out/`
- Serve locally for a quick check (example): `npx serve@latest out`
- Deploy by copying the `out/` directory to any HTTP server (e.g., `nginx`, `httpd`, `python -m http.server`).

### Hosting in a subdirectory or behind a reverse proxy path
If the site will be served from a non-root path (for example `https://example.com/constant/`), build with the base
path baked in so all assets resolve correctly:

```
NEXT_PUBLIC_BASE_PATH=/constant npm run build
```

This sets both the Next.js `basePath` and the URLs used to load the WASM worker so the static files under `out/`
remain portable.

**Example for FreeBSD server**

Target URL: `http://th.if.uj.edu.pl/~odrzywolek/WASM/calculator/`

1. In PowerShell, set the base path (note: use the path portion, not the full URL):
   ```powershell
   $env:NEXT_PUBLIC_BASE_PATH = "/~odrzywolek/WASM/calculator"
   npm run build
   ```
2. Copy the generated `out/` directory to `http://th.if.uj.edu.pl/~odrzywolek/WASM/calculator/` on the server (so
   `out/index.html` ends up at `.../calculator/index.html`, `out/wasm/worker.js` at `.../calculator/wasm/worker.js`,
   etc.).
3. Serve the contents of `out/` with any HTTP server (Apache `httpd`, `nginx`, etc.) — no Node.js runtime is needed
   on the server because everything is static.

If you prefer to run the Next.js server instead of exporting static files, omit `output: "export"` in `next.config.ts` and use `npm run start` after `npm run build`.
