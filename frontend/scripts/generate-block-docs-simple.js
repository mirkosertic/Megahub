#!/usr/bin/env node

/**
 * Blockly Block Documentation Generator with SVG Rendering
 *
 * This script generates comprehensive documentation for all Blockly blocks
 * used in the Megahub project by analyzing block definitions and rendering
 * visual SVG representations of each block.
 */

import {readFile,
	writeFile,
	mkdir} from 'fs/promises';
import {existsSync} from 'fs';
import {dirname,
	join} from 'path';
import {fileURLToPath} from 'url';
import {execSync} from 'child_process';
import puppeteer from 'puppeteer';

// Get current directory
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const PROJECT_ROOT = join(__dirname, '..', '..');
const FRONTEND_ROOT = join(__dirname, '..');

// Configuration from environment variables or defaults
const config = {
	mdPath : process.env.BLOCKS_MD_PATH || join(PROJECT_ROOT, 'BLOCKS.md'),
	imgDir : join(PROJECT_ROOT, 'docs', 'blocks'),
	htmlRendererPath : join(FRONTEND_ROOT, 'scripts', 'block-renderer.html'),
	rendererConfigPath : join(FRONTEND_ROOT, 'scripts', 'vite.config.js'),
};

console.log('Blockly Block Documentation Generator with PNG Screenshots');
console.log('=========================================================');
console.log('Configuration:');
console.log(`  Markdown Output: ${config.mdPath}`);
console.log(`  Image Directory: ${config.imgDir}`);
console.log(`  HTML Renderer: ${config.htmlRendererPath}`);
console.log('');

// Statistics
const stats = {
	total : 0,
	custom : 0,
	standard : 0,
	imagesGenerated : 0,
	imagesFailed : 0,
};

/**
 * Load custom block definitions
 */
async function loadBlockDefinitions() {
	// Import individual block definitions
	const blockModules = {
		mh_startthread : await import('../src/components/blockly/mh_startthread.js'),
		mh_stopthread : await import('../src/components/blockly/mh_stopthread.js'),
		mh_init : await import('../src/components/blockly/mh_init.js'),
		mh_wait : await import('../src/components/blockly/mh_wait.js'),
		mh_digitalwrite : await import('../src/components/blockly/mh_digitalwrite.js'),
		mh_pinmode : await import('../src/components/blockly/mh_pinmode.js'),
		mh_digitalread : await import('../src/components/blockly/mh_digitalread.js'),
		mh_set_motor_speed : await import('../src/components/blockly/mh_set_motor_speed.js'),
		mh_fastled_addleds : await import('../src/components/blockly/mh_fastled_addleds.js'),
		mh_fastled_show : await import('../src/components/blockly/mh_fastled_show.js'),
		mh_fastled_clear : await import('../src/components/blockly/mh_fastled_clear.js'),
		mh_fastled_set : await import('../src/components/blockly/mh_fastled_set.js'),
		mh_imu_value : await import('../src/components/blockly/mh_imu_value.js'),
		ui_show_value : await import('../src/components/blockly/ui_show_value.js'),
		mh_debug_free_heap : await import('../src/components/blockly/mh_debug_free_heap.js'),
		mh_port : await import('../src/components/blockly/mh_port.js'),
		lego_get_mode_dataset : await import('../src/components/blockly/lego_get_mode_dataset.js'),
		lego_select_mode : await import('../src/components/blockly/lego_select_mode.js'),
		mh_gamepad_gamepad : await import('../src/components/blockly/mh_gamepad_gamepad.js'),
		mh_gamepad_buttonpressed : await import('../src/components/blockly/mh_gamepad_buttonpressed.js'),
		mh_gamepad_buttonsraw : await import('../src/components/blockly/mh_gamepad_buttonsraw.js'),
		mh_gamepad_value : await import('../src/components/blockly/mh_gamepad_value.js'),
		mh_gamepad_connected : await import('../src/components/blockly/mh_gamepad_connected.js'),
		mh_debug_millis : await import('../src/components/blockly/mh_debug_millis.js'),
		mh_alg_pid : await import('../src/components/blockly/mh_alg_pid.js'),
	};

	const colors = await import('../src/components/blockly/colors.js');

	// Standard Blockly blocks with their descriptions
	const standardBlockDescriptions = {
		// Logic
		controls_if : 'Conditional block that executes code based on a condition',
		logic_compare : 'Compare two values (equal, not equal, less than, greater than, etc.)',
		logic_operation : 'Logical operations (AND, OR)',
		logic_negate : 'Negate a boolean value (NOT)',
		logic_boolean : 'Boolean value (true or false)',
		logic_null : 'Null value',
		logic_ternary : 'Ternary conditional (if-then-else expression)',

		// Loop
		controls_repeat_ext : 'Repeat a set of statements a specified number of times',
		controls_for : 'Count from a start number to an end number by a given increment',
		controls_flow_statements : 'Break out of or continue a loop',

		// Math
		math_number : 'A number value',
		math_arithmetic : 'Arithmetic operations (add, subtract, multiply, divide, power)',
		math_single : 'Single number operations (square root, absolute, negate, etc.)',
		math_trig : 'Trigonometric functions (sin, cos, tan, asin, acos, atan)',
		math_constant : 'Mathematical constants (pi, e, phi, sqrt(2), etc.)',
		math_number_property : 'Check if a number has a property (even, odd, prime, etc.)',
		math_round : 'Round a number (round, round up, round down)',
		math_on_list : 'Perform operation on a list (sum, min, max, average, etc.)',
		math_modulo : 'Remainder of division',
		math_constrain : 'Constrain a number to be within specified limits',
		math_random_int : 'Random integer between two numbers',
		math_random_float : 'Random fraction between 0 and 1',
		math_atan2 : 'Arctangent of the quotient of two numbers',

		// Text
		text : 'A text string',
		text_join : 'Join multiple text strings together',
		text_append : 'Append text to a variable',
		text_length : 'Get the length of a text string',
		text_isEmpty : 'Check if a text string is empty',
		text_indexOf : 'Find the position of text within text',
		text_charAt : 'Get a character at a specific position',
		text_getSubstring : 'Get a substring from text',
		text_changeCase : 'Change the case of text (UPPERCASE, lowercase, Title Case)',
		text_trim : 'Trim spaces from the start and/or end of text',
		text_count : 'Count occurrences of text within text',
		text_replace : 'Replace text within text',
		text_print : 'Print text to the console',

		// Lists
		lists_create_empty : 'Create an empty list',
		lists_create_with : 'Create a list with specified values',
		lists_repeat : 'Create a list with a value repeated a number of times',
		lists_length : 'Get the length of a list',
		lists_isEmpty : 'Check if a list is empty',
		lists_indexOf : 'Find the position of an item in a list',
		lists_getIndex : 'Get an item from a list at a specific position',
		lists_setIndex : 'Set an item in a list at a specific position',
		lists_getSublist : 'Get a sublist from a list',
		lists_split : 'Split text into a list, or join a list into text',
		lists_sort : 'Sort a list',
		lists_reverse : 'Reverse a list',
	};

	// Build customBlocks structure
	const customBlocks = {
		// Custom blocks
		mh_init : blockModules.mh_init.definition,

		// Standard blocks with custom colors
		controls_if : {category : 'Logic', colour : colors.colorLogic,			   description : standardBlockDescriptions.controls_if},
		logic_compare : {category : 'Logic', colour : colors.colorLogic,			 description : standardBlockDescriptions.logic_compare},
		logic_operation : {category : 'Logic', colour : colors.colorLogic,		   description : standardBlockDescriptions.logic_operation},
		logic_negate : {category : 'Logic', colour : colors.colorLogic,				description : standardBlockDescriptions.logic_negate},
		logic_boolean : {category : 'Logic', colour : colors.colorLogic,			 description : standardBlockDescriptions.logic_boolean},
		logic_null : {category : 'Logic', colour : colors.colorLogic,				  description : standardBlockDescriptions.logic_null},
		logic_ternary : {category : 'Logic', colour : colors.colorLogic,			 description : standardBlockDescriptions.logic_ternary},
		controls_repeat_ext : { category : 'Loop', colour : colors.colorLogic,	  description : standardBlockDescriptions.controls_repeat_ext},
		controls_for : { category : 'Loop', colour : colors.colorLogic,			   description : standardBlockDescriptions.controls_for},
		controls_flow_statements : { category : 'Loop', colour : colors.colorLogic, description : standardBlockDescriptions.controls_flow_statements},

		math_number : { category : 'Math',  colour : colors.colorMath,				 description : standardBlockDescriptions.math_number},
		math_arithmetic : { category : 'Math',  colour : colors.colorMath,			 description : standardBlockDescriptions.math_arithmetic},
		math_single : { category : 'Math',  colour : colors.colorMath,				 description : standardBlockDescriptions.math_single},
		math_trig : { category : 'Math',	colour : colors.colorMath,				   description : standardBlockDescriptions.math_trig},
		math_constant : { category : 'Math',	colour : colors.colorMath,			   description : standardBlockDescriptions.math_constant},
		math_number_property : { category : 'Math',  colour : colors.colorMath,	  description : standardBlockDescriptions.math_number_property},
		math_round : { category : 'Math',	 colour : colors.colorMath,				description : standardBlockDescriptions.math_round},
		math_on_list : { category : 'Math',  colour : colors.colorMath,			  description : standardBlockDescriptions.math_on_list},
		math_modulo : { category : 'Math',  colour : colors.colorMath,				 description : standardBlockDescriptions.math_modulo},
		math_constrain : { category : 'Math',	 colour : colors.colorMath,			description : standardBlockDescriptions.math_constrain},
		math_random_int : { category : 'Math',  colour : colors.colorMath,			 description : standardBlockDescriptions.math_random_int},
		math_random_float : { category : 'Math',	colour : colors.colorMath,		   description : standardBlockDescriptions.math_random_float},
		math_atan2 : { category : 'Math',	 colour : colors.colorMath,				description : standardBlockDescriptions.math_atan2},

		text : { category : 'Text',  colour : colors.colorText,					  description : standardBlockDescriptions.text},
		text_join : { category : 'Text',	colour : colors.colorText,				   description : standardBlockDescriptions.text_join},
		text_append : { category : 'Text',  colour : colors.colorText,				 description : standardBlockDescriptions.text_append},
		text_length : { category : 'Text',  colour : colors.colorText,				 description : standardBlockDescriptions.text_length},
		text_isEmpty : { category : 'Text',  colour : colors.colorText,			  description : standardBlockDescriptions.text_isEmpty},
		text_indexOf : { category : 'Text',  colour : colors.colorText,			  description : standardBlockDescriptions.text_indexOf},
		text_charAt : { category : 'Text',  colour : colors.colorText,				 description : standardBlockDescriptions.text_charAt},
		text_getSubstring : { category : 'Text',	colour : colors.colorText,		   description : standardBlockDescriptions.text_getSubstring},
		text_changeCase : { category : 'Text',  colour : colors.colorText,			 description : standardBlockDescriptions.text_changeCase},
		text_trim : { category : 'Text',	colour : colors.colorText,				   description : standardBlockDescriptions.text_trim},
		text_count : { category : 'Text',	 colour : colors.colorText,				description : standardBlockDescriptions.text_count},
		text_replace : { category : 'Text',  colour : colors.colorText,			  description : standardBlockDescriptions.text_replace},
		text_print : { category : 'Text',	 colour : colors.colorText,				description : standardBlockDescriptions.text_print},

		lists_create_empty : {category : 'Lists', colour : colors.colorLists,		  description : standardBlockDescriptions.lists_create_empty},
		lists_create_with : {category : 'Lists', colour : colors.colorLists,		 description : standardBlockDescriptions.lists_create_with},
		lists_repeat : {category : 'Lists', colour : colors.colorLists,				description : standardBlockDescriptions.lists_repeat},
		lists_length : {category : 'Lists', colour : colors.colorLists,				description : standardBlockDescriptions.lists_length},
		lists_isEmpty : {category : 'Lists', colour : colors.colorLists,			 description : standardBlockDescriptions.lists_isEmpty},
		lists_indexOf : {category : 'Lists', colour : colors.colorLists,			 description : standardBlockDescriptions.lists_indexOf},
		lists_getIndex : {category : 'Lists', colour : colors.colorLists,			  description : standardBlockDescriptions.lists_getIndex},
		lists_setIndex : {category : 'Lists', colour : colors.colorLists,			  description : standardBlockDescriptions.lists_setIndex},
		lists_getSublist : {category : 'Lists', colour : colors.colorLists,			description : standardBlockDescriptions.lists_getSublist},
		lists_split : {category : 'Lists', colour : colors.colorLists,			   description : standardBlockDescriptions.lists_split},
		lists_sort : {category : 'Lists', colour : colors.colorLists,				  description : standardBlockDescriptions.lists_sort},
		lists_reverse : {category : 'Lists', colour : colors.colorLists,			 description : standardBlockDescriptions.lists_reverse},

		// Custom I/O blocks
		mh_digitalwrite : blockModules.mh_digitalwrite.definition,
		mh_pinmode : blockModules.mh_pinmode.definition,
		mh_digitalread : blockModules.mh_digitalread.definition,
		mh_port : blockModules.mh_port.definition,
		mh_set_motor_speed : blockModules.mh_set_motor_speed.definition,

		// Gamepad blocks
		mh_gamepad_gamepad : blockModules.mh_gamepad_gamepad.definition,
		mh_gamepad_buttonpressed : blockModules.mh_gamepad_buttonpressed.definition,
		mh_gamepad_buttonsraw : blockModules.mh_gamepad_buttonsraw.definition,
		mh_gamepad_value : blockModules.mh_gamepad_value.definition,
		mh_gamepad_connected : blockModules.mh_gamepad_connected.definition,

		// LEGO blocks
		lego_get_mode_dataset : blockModules.lego_get_mode_dataset.definition,
		lego_select_mode : blockModules.lego_select_mode.definition,

		// FastLED blocks
		mh_fastled_addleds : blockModules.mh_fastled_addleds.definition,
		mh_fastled_show : blockModules.mh_fastled_show.definition,
		mh_fastled_clear : blockModules.mh_fastled_clear.definition,
		mh_fastled_set : blockModules.mh_fastled_set.definition,

		// IMU blocks
		mh_imu_value : blockModules.mh_imu_value.definition,

		// UI blocks
		ui_show_value : blockModules.ui_show_value.definition,

		// Debug blocks
		mh_debug_free_heap : blockModules.mh_debug_free_heap.definition,
		mh_debug_millis : blockModules.mh_debug_millis.definition,

		// Control flow blocks
		mh_startthread : blockModules.mh_startthread.definition,
		mh_wait : blockModules.mh_wait.definition,
		mh_stopthread : blockModules.mh_stopthread.definition,

		// Algorithms
		mh_alg_pid : blockModules.mh_alg_pid.definition
	};

	return {customBlocks, colors};
}

/**
 * Extract metadata from block definition
 */
function extractBlockMetadata(blockType, blockDef) {
	const metadata = {
		type : blockType,
		category : blockDef.category || 'Unknown',
		colour : blockDef.colour,
		isCustom : !!blockDef.blockdefinition,
		blockdefinition : blockDef.blockdefinition || null,
		inputsForToolbox : blockDef.inputsForToolbox || null,
		tooltip : '',
		message : '',
		inputs : [],
		fields : [],
		hasStatements : false,
		hasOutput : false,
	};

	if (blockDef.blockdefinition) {
		const def = blockDef.blockdefinition;
		metadata.tooltip = def.tooltip || '';
		metadata.message = def.message0 || '';
		metadata.hasStatements = !!(def.previousStatement || def.nextStatement);
		metadata.hasOutput = !!def.output;

		// Extract inputs and fields
		if (def.args0) {
			def.args0.forEach(arg => {
				if (arg.type === 'input_value' || arg.type === 'input_statement' || arg.type === 'input_dummy') {
					metadata.inputs.push({
						name : arg.name,
						type : arg.type,
						check : arg.check || null,
					});
				} else if (arg.type?.startsWith('field_')) {
					metadata.fields.push({
						name : arg.name,
						type : arg.type,
						options : arg.options || null,
						text : arg.text || null,
					});
				}
			});
		}

		stats.custom++;
	} else {
		// Standard block
		metadata.tooltip = blockDef.description || 'Standard Blockly block with Megahub colors';
		stats.standard++;
	}

	stats.total++;
	return metadata;
}

/**
 * Initialize Puppeteer and render block screenshots as PNG
 */
async function renderBlockSVGs(blocks) {
	console.log('\nLaunching browser for PNG screenshot rendering...');

	let browser = null;
	try {
		browser = await puppeteer.launch({
			headless : true,
			args : [ '--no-sandbox', '--disable-setuid-sandbox' ]
		});

		const page = await browser.newPage();

		// Set white background for screenshots
		await page.setViewport({
			width : 1200,
			height : 800,
			deviceScaleFactor : 2 // 2x for higher quality images
		});

		// Load the HTML renderer
		const rendererUrl = `file://${config.htmlRendererPath}`;
		console.log(`Loading renderer: ${rendererUrl}`);
		await page.goto(rendererUrl, {
			waitUntil : 'networkidle0',
			timeout : 120000
		});

		// Wait for Blockly to be ready
		await page.waitForFunction(() => window.Blockly && window.blockRenderer, {timeout : 30000});

		console.log('Browser ready, rendering blocks...\n');

		// Process each block
		for (const block of blocks) {
			const {type, metadata} = block;
			const category = metadata.category.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/©/g, '');
			const categoryDir = join(config.imgDir, category);

			try {
				// Ensure category directory exists
				if (!existsSync(categoryDir)) {
					await mkdir(categoryDir, {recursive : true});
				}

				const pngPath = join(categoryDir, `${type}.png`);
				let boundingBox = null;

				if (metadata.isCustom && metadata.blockdefinition) {
					// Register and render custom block
					const success = await page.evaluate((blockType, blockDef) => {
						return window.blockRenderer.registerBlock(blockType, blockDef);
					}, type, metadata.blockdefinition);

					if (success) {
						boundingBox = await page.evaluate((blockType, inputsForToolbox) => {
							return window.blockRenderer.renderBlockToSVG(blockType, inputsForToolbox);
						}, type, metadata.inputsForToolbox);
					}
				} else {
					// Render standard block with custom color
					boundingBox = await page.evaluate((blockType, colour, inputsForToolbox) => {
						return window.blockRenderer.renderStandardBlock(blockType, colour, inputsForToolbox);
					}, type, metadata.colour, metadata.inputsForToolbox);
				}

				if (boundingBox) {
					// Take screenshot of just the block area
					await page.screenshot({
						path : pngPath,
						clip : {
								x : boundingBox.x,
								y : boundingBox.y,
								width : boundingBox.width,
								height : boundingBox.height
						},
						omitBackground : false  // Keep white background
					});

					block.imagePath = `docs/blocks/${category}/${type}.png`;
					stats.imagesGenerated++;
					console.log(`  ✓ ${type} -> ${category}/${type}.png`);
				} else {
					stats.imagesFailed++;
					console.log(`  ✗ ${type} - Failed to render`);
				}
			} catch (error) {
				stats.imagesFailed++;
				console.log(`  ✗ ${type} - Error: ${error.message}`);
			}
		}

		console.log('\nPNG screenshot rendering complete.');

	} catch (error) {
		console.error('Error during PNG rendering:', error);
		throw error;
	} finally {
		if (browser) {
			await browser.close();
		}
	}
}

/**
 * Generate markdown documentation
 */
async function generateMarkdown(blocks) {
	const timestamp = new Date().toISOString().split('T')[0];

	// Group blocks by category
	const categories = {};
	blocks.forEach(block => {
		const cat = block.metadata.category;
		if (!categories[cat]) {
			categories[cat] = [];
		}
		categories[cat].push(block);
	});

	// Sort categories
	const categoryOrder = [
		'Control flow',
		'Logic',
		'Loop',
		'Math',
		'Text',
		'Lists',
		'I/O',
		'LEGO©',
		'Gamepad',
		'FastLED',
		'IMU',
		'UI',
		'Algorithms',
		'Debug',
	];

	const sortedCategories = Object.keys(categories).sort((a, b) => {
		const indexA = categoryOrder.indexOf(a);
		const indexB = categoryOrder.indexOf(b);
		if (indexA === -1 && indexB === -1)
			return a.localeCompare(b);
		if (indexA === -1)
			return 1;
		if (indexB === -1)
			return -1;
		return indexA - indexB;
	});

	let markdown = `# Blockly Blocks Documentation\n\n`;
	markdown += `**Generated:** ${timestamp}\n\n`;
	markdown += `This documentation covers all ${stats.total} blocks used in the Megahub project, including ${stats.custom} custom blocks and ${stats.standard} standard Blockly blocks with custom colors.\n\n`;
	markdown += `Block images are rendered as high-quality PNG screenshots to accurately show all block features including text inputs, checkboxes, and statement blocks.\n\n`;

	// Table of contents
	markdown += `## Table of Contents\n\n`;
	sortedCategories.forEach(cat => {
		const anchor = cat.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/©/g, '');
		markdown += `- [${cat}](#${anchor}) (${categories[cat].length} blocks)\n`;
	});
	markdown += `\n`;

	// Generate documentation for each category
	sortedCategories.forEach(cat => {
		markdown += `## ${cat}\n\n`;

		const categoryBlocks = categories[cat];
		categoryBlocks.forEach(block => {
			const {type, metadata, imagePath} = block;

			markdown += `### ${type}\n\n`;

			// Include PNG image if available
			if (imagePath) {
				markdown += `![${type}](${imagePath})\n\n`;
			}

			// Description
			if (metadata.tooltip) {
				markdown += `**Description:** ${metadata.tooltip}\n\n`;
			}

			// Block type
			const blockTypeStr = metadata.isCustom ? 'Custom' : 'Standard Blockly';
			let blockKind = '';
			if (metadata.hasStatements) {
				blockKind = 'Statement Block';
			} else if (metadata.hasOutput) {
				blockKind = 'Value Block';
			} else if (!metadata.isCustom) {
				blockKind = 'Block';
			}

			if (blockKind) {
				markdown += `**Type:** ${blockTypeStr} ${blockKind}\n\n`;
			}

			// Message
			if (metadata.message) {
				markdown += `**Message:** \`${metadata.message}\`\n\n`;
			}

			// Inputs
			if (metadata.inputs.length > 0) {
				markdown += `**Inputs:**\n\n`;
				markdown += `| Name | Type | Check |\n`;
				markdown += `|------|------|-------|\n`;
				metadata.inputs.forEach(input => {
					markdown += `| ${input.name} | ${input.type} | ${input.check || 'Any'} |\n`;
				});
				markdown += `\n`;
			}

			// Fields
			if (metadata.fields.length > 0) {
				markdown += `**Fields:**\n\n`;
				markdown += `| Name | Type | Options/Default |\n`;
				markdown += `|------|------|----------------|\n`;
				metadata.fields.forEach(field => {
					let options = '-';
					if (field.options) {
						options = field.options.map(opt => `\`${opt[0]}\``).join(', ');
					} else if (field.text) {
						options = `Default: \`${field.text}\``;
					}
					markdown += `| ${field.name} | ${field.type} | ${options} |\n`;
				});
				markdown += `\n`;
			}

			markdown += `---\n\n`;
		});
	});

	// Statistics
	markdown += `## Generation Statistics\n\n`;
	markdown += `- Total blocks: ${stats.total}\n`;
	markdown += `- Custom Megahub blocks: ${stats.custom}\n`;
	markdown += `- Standard Blockly blocks: ${stats.standard}\n`;
	markdown += `- PNG images generated: ${stats.imagesGenerated}\n`;
	if (stats.imagesFailed > 0) {
		markdown += `- PNG generation failed: ${stats.imagesFailed}\n`;
	}
	markdown += `\n`;
	markdown += `---\n\n`;
	markdown += `*Documentation generated automatically from block definitions with high-quality PNG screenshots.*\n`;

	return markdown;
}

/**
 * Build the block renderer bundle using Vite
 */
async function buildRenderer() {
	console.log('Building block renderer with Vite...');
	try {
		execSync(`vite build --config "${config.rendererConfigPath}"`, {
			cwd : FRONTEND_ROOT,
			stdio : 'inherit'
		});
		console.log('Block renderer built successfully.\n');
	} catch (error) {
		console.error('Failed to build block renderer:', error.message);
		throw error;
	}
}

/**
 * Main execution
 */
async function main() {
	try {
		// Build the renderer first
		await buildRenderer();

		// Create necessary directories
		const mdDir = dirname(config.mdPath);
		if (!existsSync(mdDir)) {
			await mkdir(mdDir, {recursive : true});
			console.log(`Created markdown directory: ${mdDir}`);
		}

		if (!existsSync(config.imgDir)) {
			await mkdir(config.imgDir, {recursive : true});
			console.log(`Created image directory: ${config.imgDir}`);
		}

		console.log('Loading block definitions...');
		const {customBlocks} = await loadBlockDefinitions();

		console.log(`Processing ${Object.keys(customBlocks).length} blocks...`);

		const blocks = [];

		// Process each block
		for (const [blockType, blockDef] of Object.entries(customBlocks)) {
			const metadata = extractBlockMetadata(blockType, blockDef);
			blocks.push({type : blockType, metadata});
		}

		console.log(`\nExtracted metadata for ${blocks.length} blocks.`);

		// Render PNG screenshots
		await renderBlockSVGs(blocks);

		console.log('\nGenerating markdown documentation...');
		const markdown = await generateMarkdown(blocks);
		await writeFile(config.mdPath, markdown, 'utf-8');

		console.log('\n=========================================================');
		console.log('Documentation generation complete!');
		console.log('=========================================================');
		console.log(`Total blocks: ${stats.total}`);
		console.log(`  Custom Megahub blocks: ${stats.custom}`);
		console.log(`  Standard Blockly blocks: ${stats.standard}`);
		console.log(`PNG images: ${stats.imagesGenerated} generated, ${stats.imagesFailed} failed`);
		console.log(`\nOutput files:`);
		console.log(`  Markdown: ${config.mdPath}`);
		console.log(`  Images: ${config.imgDir}`);
		console.log('');

		process.exit(0);

	} catch (error) {
		console.error('\nError:', error.message);
		console.error(error.stack);
		process.exit(1);
	}
}

main();
