#!/usr/bin/env node

/**
 * Blockly Block Documentation Generator
 *
 * This script generates comprehensive documentation for all Blockly blocks
 * used in the Megahub project, including both custom blocks and standard
 * Blockly blocks with customized colors.
 *
 * Features:
 * - Renders blocks to SVG images
 * - Extracts metadata (inputs, fields, tooltips, categories)
 * - Generates BLOCKS.md with visual and textual documentation
 * - Configurable output paths via environment variables
 */

import { JSDOM } from 'jsdom';
import { readFile, writeFile, mkdir } from 'fs/promises';
import { existsSync } from 'fs';
import { dirname, join, relative } from 'path';
import { fileURLToPath } from 'url';

// Get current directory
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const PROJECT_ROOT = join(__dirname, '..', '..');
const FRONTEND_ROOT = join(__dirname, '..');

// Configuration from environment variables or defaults
const config = {
  svgDir: process.env.BLOCKS_SVG_DIR || join(PROJECT_ROOT, 'docs', 'blocks'),
  mdPath: process.env.BLOCKS_MD_PATH || join(PROJECT_ROOT, 'BLOCKS.md'),
  embedSvg: process.env.BLOCKS_SVG_EMBED !== 'false', // Default to true
};

console.log('Blockly Block Documentation Generator');
console.log('=====================================');
console.log('Configuration:');
console.log(`  SVG Output: ${config.svgDir}`);
console.log(`  Markdown Output: ${config.mdPath}`);
console.log(`  Embed SVG: ${config.embedSvg}`);
console.log('');

// Statistics
const stats = {
  total: 0,
  custom: 0,
  standard: 0,
  success: 0,
  failed: 0,
  failedBlocks: [],
};

/**
 * Setup DOM environment for Blockly
 */
async function setupBlocklyEnvironment() {
  const html = `
    <!DOCTYPE html>
    <html>
      <head>
        <meta charset="utf-8">
        <style>
          .blocklyText { font-family: sans-serif; font-size: 10pt; }
        </style>
      </head>
      <body>
        <div id="blocklyDiv" style="height: 600px; width: 800px;"></div>
      </body>
    </html>
  `;

  const dom = new JSDOM(html, {
    runScripts: 'dangerously',
    resources: 'usable',
    pretendToBeVisual: true,
  });

  // Set global variables for Blockly BEFORE importing
  const { window } = dom;

  // Override read-only navigator
  Object.defineProperty(global, 'navigator', {
    writable: true,
    value: {
      userAgent: 'node.js',
      platform: 'node',
    }
  });

  global.window = window;
  global.document = window.document;
  global.Element = window.Element;
  global.HTMLElement = window.HTMLElement;
  global.SVGElement = window.SVGElement;
  global.XMLSerializer = window.XMLSerializer;

  // Import Blockly and related modules
  const BlocklyModule = await import('blockly/core');
  const Blockly = BlocklyModule.default || BlocklyModule;
  const libraryBlocks = await import('blockly/blocks');
  const fieldColourModule = await import('@blockly/field-colour');
  const registerFieldColour = fieldColourModule.registerFieldColour || fieldColourModule.default;
  const EnModule = await import('blockly/msg/en');
  const En = EnModule.default || EnModule;

  // Set locale
  if (Blockly.setLocale) {
    Blockly.setLocale(En);
  } else {
    // Fallback for older versions
    Blockly.Msg = En;
  }

  // Register field colour
  if (typeof registerFieldColour === 'function') {
    registerFieldColour();
  }

  return { Blockly, libraryBlocks };
}

/**
 * Load custom block definitions
 */
async function loadBlockDefinitions() {
  const componentPath = join(FRONTEND_ROOT, 'src', 'components', 'blockly', 'component.js');
  const componentCode = await readFile(componentPath, 'utf-8');

  // Extract customBlocks object (lines 44-292)
  const customBlocksMatch = componentCode.match(/const customBlocks = \{([\s\S]+?)\n\};/);
  if (!customBlocksMatch) {
    throw new Error('Could not find customBlocks definition in component.js');
  }

  // Import individual block definitions
  const blockModules = {
    mh_startthread: await import('../src/components/blockly/mh_startthread.js'),
    mh_stopthread: await import('../src/components/blockly/mh_stopthread.js'),
    mh_init: await import('../src/components/blockly/mh_init.js'),
    mh_wait: await import('../src/components/blockly/mh_wait.js'),
    mh_digitalwrite: await import('../src/components/blockly/mh_digitalwrite.js'),
    mh_pinmode: await import('../src/components/blockly/mh_pinmode.js'),
    mh_digitalread: await import('../src/components/blockly/mh_digitalread.js'),
    mh_set_motor_speed: await import('../src/components/blockly/mh_set_motor_speed.js'),
    mh_fastled_addleds: await import('../src/components/blockly/mh_fastled_addleds.js'),
    mh_fastled_show: await import('../src/components/blockly/mh_fastled_show.js'),
    mh_fastled_clear: await import('../src/components/blockly/mh_fastled_clear.js'),
    mh_fastled_set: await import('../src/components/blockly/mh_fastled_set.js'),
    mh_imu_value: await import('../src/components/blockly/mh_imu_value.js'),
    ui_show_value: await import('../src/components/blockly/ui_show_value.js'),
    mh_debug_free_heap: await import('../src/components/blockly/mh_debug_free_heap.js'),
    mh_port: await import('../src/components/blockly/mh_port.js'),
    lego_get_mode_dataset: await import('../src/components/blockly/lego_get_mode_dataset.js'),
    lego_select_mode: await import('../src/components/blockly/lego_select_mode.js'),
    mh_gamepad_gamepad: await import('../src/components/blockly/mh_gamepad_gamepad.js'),
    mh_gamepad_buttonpressed: await import('../src/components/blockly/mh_gamepad_buttonpressed.js'),
    mh_gamepad_buttonsraw: await import('../src/components/blockly/mh_gamepad_buttonsraw.js'),
    mh_gamepad_value: await import('../src/components/blockly/mh_gamepad_value.js'),
    mh_gamepad_connected: await import('../src/components/blockly/mh_gamepad_connected.js'),
    mh_debug_millis: await import('../src/components/blockly/mh_debug_millis.js'),
  };

  const colors = await import('../src/components/blockly/colors.js');

  // Build customBlocks structure
  const customBlocks = {
    // Custom blocks
    mh_init: blockModules.mh_init.definition,

    // Standard blocks with custom colors
    controls_if: { category: 'Logic', colour: colors.colorLogic },
    logic_compare: { category: 'Logic', colour: colors.colorLogic },
    logic_operation: { category: 'Logic', colour: colors.colorLogic },
    logic_negate: { category: 'Logic', colour: colors.colorLogic },
    logic_boolean: { category: 'Logic', colour: colors.colorLogic },
    logic_null: { category: 'Logic', colour: colors.colorLogic },
    logic_ternary: { category: 'Logic', colour: colors.colorLogic },
    controls_repeat_ext: { category: 'Loop', colour: colors.colorLogic },
    controls_for: { category: 'Loop', colour: colors.colorLogic },
    controls_flow_statements: { category: 'Loop', colour: colors.colorLogic },

    math_number: { category: 'Math', colour: colors.colorMath },
    math_arithmetic: { category: 'Math', colour: colors.colorMath },
    math_single: { category: 'Math', colour: colors.colorMath },
    math_trig: { category: 'Math', colour: colors.colorMath },
    math_constant: { category: 'Math', colour: colors.colorMath },
    math_number_property: { category: 'Math', colour: colors.colorMath },
    math_round: { category: 'Math', colour: colors.colorMath },
    math_on_list: { category: 'Math', colour: colors.colorMath },
    math_modulo: { category: 'Math', colour: colors.colorMath },
    math_constrain: { category: 'Math', colour: colors.colorMath },
    math_random_int: { category: 'Math', colour: colors.colorMath },
    math_random_float: { category: 'Math', colour: colors.colorMath },
    math_atan2: { category: 'Math', colour: colors.colorMath },

    text: { category: 'Text', colour: colors.colorText },
    text_join: { category: 'Text', colour: colors.colorText },
    text_append: { category: 'Text', colour: colors.colorText },
    text_length: { category: 'Text', colour: colors.colorText },
    text_isEmpty: { category: 'Text', colour: colors.colorText },
    text_indexOf: { category: 'Text', colour: colors.colorText },
    text_charAt: { category: 'Text', colour: colors.colorText },
    text_getSubstring: { category: 'Text', colour: colors.colorText },
    text_changeCase: { category: 'Text', colour: colors.colorText },
    text_trim: { category: 'Text', colour: colors.colorText },
    text_count: { category: 'Text', colour: colors.colorText },
    text_replace: { category: 'Text', colour: colors.colorText },
    text_print: { category: 'Text', colour: colors.colorText },

    lists_create_empty: { category: 'Lists', colour: colors.colorLists },
    lists_create_with: { category: 'Lists', colour: colors.colorLists },
    lists_repeat: { category: 'Lists', colour: colors.colorLists },
    lists_length: { category: 'Lists', colour: colors.colorLists },
    lists_isEmpty: { category: 'Lists', colour: colors.colorLists },
    lists_indexOf: { category: 'Lists', colour: colors.colorLists },
    lists_getIndex: { category: 'Lists', colour: colors.colorLists },
    lists_setIndex: { category: 'Lists', colour: colors.colorLists },
    lists_getSublist: { category: 'Lists', colour: colors.colorLists },
    lists_split: { category: 'Lists', colour: colors.colorLists },
    lists_sort: { category: 'Lists', colour: colors.colorLists },
    lists_reverse: { category: 'Lists', colour: colors.colorLists },

    // Custom I/O blocks
    mh_digitalwrite: blockModules.mh_digitalwrite.definition,
    mh_pinmode: blockModules.mh_pinmode.definition,
    mh_digitalread: blockModules.mh_digitalread.definition,
    mh_port: blockModules.mh_port.definition,
    mh_set_motor_speed: blockModules.mh_set_motor_speed.definition,

    // Gamepad blocks
    mh_gamepad_gamepad: blockModules.mh_gamepad_gamepad.definition,
    mh_gamepad_buttonpressed: blockModules.mh_gamepad_buttonpressed.definition,
    mh_gamepad_buttonsraw: blockModules.mh_gamepad_buttonsraw.definition,
    mh_gamepad_value: blockModules.mh_gamepad_value.definition,
    mh_gamepad_connected: blockModules.mh_gamepad_connected.definition,

    // LEGO blocks
    lego_get_mode_dataset: blockModules.lego_get_mode_dataset.definition,
    lego_select_mode: blockModules.lego_select_mode.definition,

    // FastLED blocks
    mh_fastled_addleds: blockModules.mh_fastled_addleds.definition,
    mh_fastled_show: blockModules.mh_fastled_show.definition,
    mh_fastled_clear: blockModules.mh_fastled_clear.definition,
    mh_fastled_set: blockModules.mh_fastled_set.definition,

    // IMU blocks
    mh_imu_value: blockModules.mh_imu_value.definition,

    // UI blocks
    ui_show_value: blockModules.ui_show_value.definition,

    // Debug blocks
    mh_debug_free_heap: blockModules.mh_debug_free_heap.definition,
    mh_debug_millis: blockModules.mh_debug_millis.definition,

    // Control flow blocks
    mh_startthread: blockModules.mh_startthread.definition,
    mh_wait: blockModules.mh_wait.definition,
    mh_stopthread: blockModules.mh_stopthread.definition,
  };

  return { customBlocks, colors };
}

/**
 * Register blocks with Blockly
 */
function registerBlocks(Blockly, customBlocks) {
  Object.entries(customBlocks).forEach(([type, def]) => {
    if (def.blockdefinition) {
      // Custom block with definition
      Blockly.Blocks[def.blockdefinition.type] = {
        init: function() {
          this.jsonInit(def.blockdefinition);
        }
      };
      stats.custom++;
    } else {
      // Standard block with custom color
      const originalInit = Blockly.Blocks[type]?.init;
      if (originalInit) {
        Blockly.Blocks[type].init = function() {
          originalInit.call(this);
          this.setColour(def.colour);
        };
        stats.standard++;
      }
    }
    stats.total++;
  });
}

/**
 * Render a block to SVG
 */
function renderBlockToSVG(Blockly, workspace, blockType) {
  try {
    const block = workspace.newBlock(blockType);

    // Initialize the block's SVG representation
    if (typeof block.initSvg === 'function') {
      block.initSvg();
    }

    // Render the block
    if (typeof block.render === 'function') {
      block.render();
    }

    // Get the SVG element
    const blockSvg = block.getSvgRoot();
    if (!blockSvg) {
      throw new Error('Block SVG root not found');
    }

    const bbox = blockSvg.getBBox();

    // Create a clean SVG container
    const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    svg.setAttribute('xmlns', 'http://www.w3.org/2000/svg');
    svg.setAttribute('width', Math.ceil(bbox.width + 10));
    svg.setAttribute('height', Math.ceil(bbox.height + 10));
    svg.setAttribute('viewBox', `${bbox.x - 5} ${bbox.y - 5} ${bbox.width + 10} ${bbox.height + 10}`);

    // Clone the block SVG
    const clonedBlock = blockSvg.cloneNode(true);
    svg.appendChild(clonedBlock);

    const svgString = svg.outerHTML;

    // Clean up
    block.dispose();

    return svgString;
  } catch (error) {
    console.warn(`  Warning: Failed to render block "${blockType}": ${error.message}`);
    stats.failed++;
    stats.failedBlocks.push(blockType);
    return null;
  }
}

/**
 * Extract metadata from block definition
 */
function extractBlockMetadata(blockType, blockDef, Blockly) {
  const metadata = {
    type: blockType,
    category: blockDef.category || 'Unknown',
    colour: blockDef.colour,
    isCustom: !!blockDef.blockdefinition,
    tooltip: '',
    inputs: [],
    fields: [],
    hasStatements: false,
    hasOutput: false,
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
            name: arg.name,
            type: arg.type,
            check: arg.check || null,
          });
        } else if (arg.type?.startsWith('field_')) {
          metadata.fields.push({
            name: arg.name,
            type: arg.type,
            options: arg.options || null,
            text: arg.text || null,
          });
        }
      });
    }
  } else {
    // Standard block - get info from Blockly
    const block = Blockly.Blocks[blockType];
    if (block) {
      metadata.tooltip = 'Standard Blockly block with Megahub colors';
    }
  }

  return metadata;
}

/**
 * Convert SVG to data URI
 */
function svgToDataUri(svg) {
  const base64 = Buffer.from(svg).toString('base64');
  return `data:image/svg+xml;base64,${base64}`;
}

/**
 * Generate markdown documentation
 */
async function generateMarkdown(blocks, svgMap) {
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
    'Debug',
  ];

  const sortedCategories = Object.keys(categories).sort((a, b) => {
    const indexA = categoryOrder.indexOf(a);
    const indexB = categoryOrder.indexOf(b);
    if (indexA === -1 && indexB === -1) return a.localeCompare(b);
    if (indexA === -1) return 1;
    if (indexB === -1) return -1;
    return indexA - indexB;
  });

  let markdown = `# Blockly Blocks Documentation\n\n`;
  markdown += `**Generated:** ${timestamp}\n\n`;
  markdown += `This documentation covers all ${stats.total} blocks used in the Megahub project, including ${stats.custom} custom blocks and ${stats.standard} standard Blockly blocks with custom colors.\n\n`;

  // Table of contents
  markdown += `## Table of Contents\n\n`;
  sortedCategories.forEach(cat => {
    const anchor = cat.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/©/g, '');
    markdown += `- [${cat}](#${anchor})\n`;
  });
  markdown += `\n`;

  // Generate documentation for each category
  sortedCategories.forEach(cat => {
    markdown += `## ${cat}\n\n`;

    const categoryBlocks = categories[cat];
    categoryBlocks.forEach(block => {
      const { type, metadata } = block;
      const svg = svgMap[type];

      markdown += `### ${type}\n\n`;

      // SVG image
      if (svg) {
        if (config.embedSvg) {
          const dataUri = svgToDataUri(svg);
          markdown += `![${type}](${dataUri})\n\n`;
        } else {
          const svgPath = join(config.svgDir, `${type}.svg`);
          const relativePath = relative(dirname(config.mdPath), svgPath);
          markdown += `![${type}](${relativePath})\n\n`;
        }
      } else {
        markdown += `*[SVG rendering failed]*\n\n`;
      }

      // Description
      if (metadata.tooltip) {
        markdown += `**Description:** ${metadata.tooltip}\n\n`;
      }

      // Block type
      const blockTypeStr = metadata.isCustom ? 'Custom' : 'Standard';
      let blockKind = 'Unknown';
      if (metadata.hasStatements) {
        blockKind = 'Statement Block';
      } else if (metadata.hasOutput) {
        blockKind = 'Value Block';
      }
      markdown += `**Type:** ${blockTypeStr} ${blockKind}\n\n`;

      // Message
      if (metadata.message) {
        markdown += `**Message:** ${metadata.message}\n\n`;
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
        markdown += `| Name | Type | Options |\n`;
        markdown += `|------|------|--------|\n`;
        metadata.fields.forEach(field => {
          let options = '-';
          if (field.options) {
            options = field.options.map(opt => opt[0]).join(', ');
          } else if (field.text) {
            options = `Default: "${field.text}"`;
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
  markdown += `- Custom blocks: ${stats.custom}\n`;
  markdown += `- Standard blocks: ${stats.standard}\n`;
  markdown += `- Successfully rendered: ${stats.success}\n`;
  markdown += `- Failed to render: ${stats.failed}\n`;

  if (stats.failedBlocks.length > 0) {
    markdown += `\n**Failed blocks:**\n`;
    stats.failedBlocks.forEach(block => {
      markdown += `- ${block}\n`;
    });
  }

  return markdown;
}

/**
 * Main execution
 */
async function main() {
  try {
    // Create output directories
    if (!existsSync(config.svgDir)) {
      await mkdir(config.svgDir, { recursive: true });
      console.log(`Created SVG directory: ${config.svgDir}`);
    }

    const mdDir = dirname(config.mdPath);
    if (!existsSync(mdDir)) {
      await mkdir(mdDir, { recursive: true });
      console.log(`Created markdown directory: ${mdDir}`);
    }

    console.log('Setting up Blockly environment...');
    const { Blockly } = await setupBlocklyEnvironment();

    console.log('Loading block definitions...');
    const { customBlocks } = await loadBlockDefinitions();

    console.log('Registering blocks...');
    registerBlocks(Blockly, customBlocks);

    console.log(`Processing ${stats.total} blocks...`);

    // Create injected workspace for proper SVG rendering
    const container = document.getElementById('blocklyDiv');
    const workspace = Blockly.inject(container, {
      readOnly: true,
      media: 'https://unpkg.com/blockly/media/',
      zoom: {
        controls: false,
        wheel: false,
        startScale: 1.0
      },
      trashcan: false
    });

    const blocks = [];
    const svgMap = {};

    // Process each block
    for (const [blockType, blockDef] of Object.entries(customBlocks)) {
      process.stdout.write(`  Rendering ${blockType}... `);

      const svg = renderBlockToSVG(Blockly, workspace, blockType);
      const metadata = extractBlockMetadata(blockType, blockDef, Blockly);

      if (svg) {
        svgMap[blockType] = svg;

        // Save SVG file if not embedding
        if (!config.embedSvg) {
          const svgPath = join(config.svgDir, `${blockType}.svg`);
          await writeFile(svgPath, svg, 'utf-8');
        }

        stats.success++;
        console.log('OK');
      } else {
        console.log('FAILED');
      }

      blocks.push({ type: blockType, metadata, svg });
    }

    console.log('\nGenerating markdown documentation...');
    const markdown = await generateMarkdown(blocks, svgMap);
    await writeFile(config.mdPath, markdown, 'utf-8');

    console.log('\n=====================================');
    console.log('Documentation generation complete!');
    console.log('=====================================');
    console.log(`Total blocks: ${stats.total}`);
    console.log(`  Custom: ${stats.custom}`);
    console.log(`  Standard: ${stats.standard}`);
    console.log(`Successfully rendered: ${stats.success}`);
    console.log(`Failed: ${stats.failed}`);
    console.log(`\nOutput files:`);
    console.log(`  Markdown: ${config.mdPath}`);
    if (!config.embedSvg) {
      console.log(`  SVG images: ${config.svgDir}`);
    }

    process.exit(stats.failed > 0 ? 1 : 0);

  } catch (error) {
    console.error('\nError:', error.message);
    console.error(error.stack);
    process.exit(1);
  }
}

main();
