import {resolve} from 'path';
import {defineConfig} from 'vite';
import compress from 'vite-plugin-compression';

// clang-format off
export default defineConfig(({mode}) => {
	return {
		build: {
			outDir: `../data/${mode}`,
			emptyOutDir: true,
			minify: 'esbuild',
			cssMinify: true,
			cssCodeSplit: false,
			rollupOptions: {
							input: {
								index: resolve(__dirname, 'index.html'),
								bttest: resolve(__dirname, 'bttest.html'),
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
				algorithm : 'gzip',
				ext : '.gz',
				deleteOriginFile : false
			})
		],
		define: {
			'import.meta.env.VITE_BUILD_MODE': JSON.stringify(mode)
		}
	}
});
// clang-format on
