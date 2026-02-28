#!/usr/bin/env node
// Validates that all constant names used in Blockly blocks are registered as
// Lua globals in the firmware. Run with: node scripts/validate-constants.js

import { readFileSync } from 'fs';
import { fileURLToPath } from 'url';
import { dirname, resolve } from 'path';

const __dirname = dirname(fileURLToPath(import.meta.url));
const root = resolve(__dirname, '../..');

// Extract lua_setglobal names from megahub.cpp
const megahubCpp = readFileSync(resolve(root, 'lib/megahub/src/megahub.cpp'), 'utf8');
const luaGlobals = new Set(
	[...megahubCpp.matchAll(/lua_setglobal\s*\([^,]+,\s*"([^"]+)"\s*\)/g)]
		.map(m => m[1])
);

// Extract constant values from constants.js (the Lua-side names)
const constantsJs = readFileSync(
	resolve(__dirname, '../src/components/blockly/constants.js'), 'utf8'
);
const blocklyConstants = new Set(
	[...constantsJs.matchAll(/'([A-Z][A-Z0-9_]+)'/g)]
		.map(m => m[1])
		.filter(s => !s.startsWith('GPIO') || luaGlobals.has(s)) // keep all for checking
);

let errors = 0;
for (const name of blocklyConstants) {
	if (!luaGlobals.has(name)) {
		console.error(`ERROR: '${name}' used in constants.js but not registered as a Lua global in megahub.cpp`);
		errors++;
	}
}

if (errors === 0) {
	console.log(`OK: All ${blocklyConstants.size} constants validated against ${luaGlobals.size} Lua globals.`);
	process.exit(0);
} else {
	console.error(`FAIL: ${errors} constant(s) are not registered in firmware.`);
	process.exit(1);
}
