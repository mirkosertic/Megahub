import js from '@eslint/js';
import globals from 'globals';
import eslintConfigPrettier from 'eslint-config-prettier';

export default [
    js.configs.recommended,

    // Source files — browser environment
    {
        files: ['src/**/*.js'],
        languageOptions: {
            globals: {
                ...globals.browser,
            },
        },
        rules: {
            'no-unused-vars': ['warn', { argsIgnorePattern: '^_' }],
            'no-var': 'error',
            'prefer-const': 'warn',
            eqeqeq: ['error', 'always'],
        },
    },

    // Test files — vitest globals added on top of browser
    {
        files: ['test/**/*.js', 'src/**/*.test.js'],
        languageOptions: {
            globals: {
                ...globals.browser,
                describe: 'readonly',
                it: 'readonly',
                expect: 'readonly',
                vi: 'readonly',
                beforeEach: 'readonly',
                afterEach: 'readonly',
                beforeAll: 'readonly',
                afterAll: 'readonly',
            },
        },
    },

    // Config and build scripts — Node.js environment
    {
        files: ['*.config.js', 'scripts/**/*.js'],
        languageOptions: {
            globals: {
                ...globals.node,
            },
        },
    },

    // Disable ESLint formatting rules that conflict with Prettier
    eslintConfigPrettier,
];
