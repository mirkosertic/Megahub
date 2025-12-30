import { defineConfig } from 'vite';
import compress from 'vite-plugin-compression';
import { resolve } from 'path';

export default defineConfig({
  build: {
    outDir: '../data',
    emptyOutDir: true,
    minify: 'esbuild',
    cssMinify: true,
    cssCodeSplit: false,
    rollupOptions: {
      input: {
        index: resolve(__dirname, 'index.html'),
        project: resolve(__dirname, 'project.html'),
      },
      output: {
        manualChunks: undefined,
                
        entryFileNames: '[name].js',
        chunkFileNames: '[name].js',
        assetFileNames: (assetInfo) => {
          if (assetInfo.name.endsWith('.css')) {
            return '[name][extname]'
          }
          return '[name][extname]'
        }
      }
    },
    // Increase warning limit if you get large bundle warnings
    chunkSizeWarningLimit: 1500    
  },
  plugins: [
    compress({ 
      algorithm: 'gzip', 
      ext: '.gz',
      deleteOriginFile: false
    })
  ]
});