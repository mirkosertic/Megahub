import { defineConfig } from 'vite';
import compress from 'vite-plugin-compression';
import { resolve } from 'path';

export default defineConfig({
  build: {
    outDir: '../data',
    emptyOutDir: true,
    minify: 'esbuild',
    cssMinify: true,
    rollupOptions: {
      input: {
        main: resolve(__dirname, 'index.html'),
      },
      output: {
        entryFileNames: '[name].js',
        chunkFileNames: 'chunks/[name]-[hash].js',
        assetFileNames: (assetInfo) => {
          if (assetInfo.name.endsWith('.css')) {
            return 'style.css';
          }
          if (assetInfo.name.endsWith('.html')) {
            return '[name].html';
          }
          return 'assets/[name].[ext]';
        }
      }
    }
  },
  plugins: [
    compress({ 
      algorithm: 'gzip', 
      ext: '.gz',
      deleteOriginFile: false
    })
  ]
});