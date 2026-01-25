import {resolve} from 'path';
import {defineConfig} from 'vite';

export default defineConfig({
  build: {
    outDir: resolve(__dirname, 'dist'),
    emptyOutDir: true,
    lib: {
      entry: resolve(__dirname, 'block-renderer.js'),
      name: 'BlockRenderer',
      formats: ['iife'],
      fileName: () => 'block-renderer.js'
    },
    rollupOptions: {
      output: {
        // Don't hash the filename - we reference it directly
        entryFileNames: 'block-renderer.js',
        // Inline everything to make it easier for Puppeteer
        inlineDynamicImports: true,
        // Make sure window.blockRenderer is available
        extend: true
      }
    }
  }
});
