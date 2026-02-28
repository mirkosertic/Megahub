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
		},

		// ─── Vitest configuration ─────────────────────────────────────────────
		test: {
			// Use happy-dom instead of jsdom — it has proper Shadow DOM support
			// which is needed for testing Web Components with shadowRoot
			environment: 'happy-dom',

			// Make describe/it/expect/vi available without importing them
			globals: true,

			// Test files co-located with source or in a dedicated test/ folder
			include: ['src/**/*.test.js', 'test/**/*.test.js'],

			// Run setup before each test file (resets shared state)
			setupFiles: ['./test/setup.js'],

			// Coverage via V8 (built into Node) — no extra binary needed
			coverage: {
				provider: 'v8',
				include: ['src/app/**', 'src/components/**'],
				// Blockly block definitions are generated/external-owned
				exclude: ['src/components/blockly/**'],
			},
		},
	}
});
// clang-format on
