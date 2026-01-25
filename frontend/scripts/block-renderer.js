/**
 * Vite-based Block Renderer for Documentation
 *
 * This module uses the same Blockly imports and configuration as the main
 * application to ensure visual consistency in generated documentation.
 */

// Import Blockly core - same as main app
import * as Blockly from 'blockly/core';
import * as libraryBlocks from 'blockly/blocks';
import {luaGenerator} from 'blockly/lua';
import {registerFieldColour} from '@blockly/field-colour';
import * as En from 'blockly/msg/en';

// Import custom block definitions
import {definition as mh_startthread} from '../src/components/blockly/mh_startthread.js';
import {definition as mh_stopthread} from '../src/components/blockly/mh_stopthread.js';
import {definition as mh_init} from '../src/components/blockly/mh_init.js';
import {definition as mh_wait} from '../src/components/blockly/mh_wait.js';
import {definition as mh_digitalwrite} from '../src/components/blockly/mh_digitalwrite.js';
import {definition as mh_pinmode} from '../src/components/blockly/mh_pinmode.js';
import {definition as mh_digitalread} from '../src/components/blockly/mh_digitalread.js';
import {definition as mh_set_motor_speed} from '../src/components/blockly/mh_set_motor_speed.js';
import {definition as mh_fastled_addleds} from '../src/components/blockly/mh_fastled_addleds.js';
import {definition as mh_fastled_show} from '../src/components/blockly/mh_fastled_show.js';
import {definition as mh_fastled_clear} from '../src/components/blockly/mh_fastled_clear.js';
import {definition as mh_fastled_set} from '../src/components/blockly/mh_fastled_set.js';
import {definition as mh_imu_value} from '../src/components/blockly/mh_imu_value.js';
import {definition as ui_show_value} from '../src/components/blockly/ui_show_value.js';
import {definition as mh_debug_free_heap} from '../src/components/blockly/mh_debug_free_heap.js';
import {definition as mh_port} from '../src/components/blockly/mh_port.js';
import {definition as lego_get_mode_dataset} from '../src/components/blockly/lego_get_mode_dataset.js';
import {definition as lego_select_mode} from '../src/components/blockly/lego_select_mode.js';
import {definition as gamepad_gamepad} from '../src/components/blockly/mh_gamepad_gamepad.js';
import {definition as gamepad_buttonpressed} from '../src/components/blockly/mh_gamepad_buttonpressed.js';
import {definition as gamepad_buttonsraw} from '../src/components/blockly/mh_gamepad_buttonsraw.js';
import {definition as gamepad_value} from '../src/components/blockly/mh_gamepad_value.js';
import {definition as gamepad_connected} from '../src/components/blockly/mh_gamepad_connected.js';
import {definition as mh_debug_millis} from '../src/components/blockly/mh_debug_millis.js';

// Import colors
import {
  colorLogic,
  colorMath,
  colorText,
  colorLists,
} from '../src/components/blockly/colors.js';

// Build customBlocks structure - same as component.js
const customBlocks = {
  "mh_init": mh_init,

  "controls_if": { category: 'Logic', colour: colorLogic },
  "logic_compare": { category: 'Logic', colour: colorLogic },
  "logic_operation": { category: 'Logic', colour: colorLogic },
  "logic_negate": { category: 'Logic', colour: colorLogic },
  "logic_boolean": { category: 'Logic', colour: colorLogic },
  "logic_null": { category: 'Logic', colour: colorLogic },
  "logic_ternary": { category: 'Logic', colour: colorLogic },
  "controls_repeat_ext": { category: 'Loop', colour: colorLogic },
  "controls_for": { category: 'Loop', colour: colorLogic },
  "controls_flow_statements": { category: 'Loop', colour: colorLogic },

  "math_number": { category: 'Math', colour: colorMath },
  "math_arithmetic": { category: 'Math', colour: colorMath },
  "math_single": { category: 'Math', colour: colorMath },
  "math_trig": { category: 'Math', colour: colorMath },
  "math_constant": { category: 'Math', colour: colorMath },
  "math_number_property": { category: 'Math', colour: colorMath },
  "math_round": { category: 'Math', colour: colorMath },
  "math_on_list": { category: 'Math', colour: colorMath },
  "math_modulo": { category: 'Math', colour: colorMath },
  "math_constrain": { category: 'Math', colour: colorMath },
  "math_random_int": { category: 'Math', colour: colorMath },
  "math_random_float": { category: 'Math', colour: colorMath },
  "math_atan2": { category: 'Math', colour: colorMath },

  "text": { category: 'Text', colour: colorText },
  "text_join": { category: 'Text', colour: colorText },
  "text_append": { category: 'Text', colour: colorText },
  "text_length": { category: 'Text', colour: colorText },
  "text_isEmpty": { category: 'Text', colour: colorText },
  "text_indexOf": { category: 'Text', colour: colorText },
  "text_charAt": { category: 'Text', colour: colorText },
  "text_getSubstring": { category: 'Text', colour: colorText },
  "text_changeCase": { category: 'Text', colour: colorText },
  "text_trim": { category: 'Text', colour: colorText },
  "text_count": { category: 'Text', colour: colorText },
  "text_replace": { category: 'Text', colour: colorText },
  "text_print": { category: 'Text', colour: colorText },

  "lists_create_empty": { category: 'Lists', colour: colorLists },
  "lists_create_with": { category: 'Lists', colour: colorLists },
  "lists_repeat": { category: 'Lists', colour: colorLists },
  "lists_length": { category: 'Lists', colour: colorLists },
  "lists_isEmpty": { category: 'Lists', colour: colorLists },
  "lists_indexOf": { category: 'Lists', colour: colorLists },
  "lists_getIndex": { category: 'Lists', colour: colorLists },
  "lists_setIndex": { category: 'Lists', colour: colorLists },
  "lists_getSublist": { category: 'Lists', colour: colorLists },
  "lists_split": { category: 'Lists', colour: colorLists },
  "lists_sort": { category: 'Lists', colour: colorLists },
  "lists_reverse": { category: 'Lists', colour: colorLists },

  "mh_digitalwrite": mh_digitalwrite,
  "mh_pinmode": mh_pinmode,
  "mh_digitalread": mh_digitalread,
  "mh_port": mh_port,
  "mh_set_motor_speed": mh_set_motor_speed,
  "mh_gamepad_gamepad": gamepad_gamepad,
  "mh_gamepad_buttonpressed": gamepad_buttonpressed,
  "mh_gamepad_buttonsraw": gamepad_buttonsraw,
  "mh_gamepad_value": gamepad_value,
  "mh_gamepad_connected": gamepad_connected,
  "lego_get_mode_dataset": lego_get_mode_dataset,
  "lego_select_mode": lego_select_mode,
  "mh_fastled_addleds": mh_fastled_addleds,
  "mh_fastled_show": mh_fastled_show,
  "mh_fastled_clear": mh_fastled_clear,
  "mh_fastled_set": mh_fastled_set,
  "mh_imu_value": mh_imu_value,
  "ui_show_value": ui_show_value,
  "mh_debug_free_heap": mh_debug_free_heap,
  "mh_debug_millis": mh_debug_millis,
  "mh_startthread": mh_startthread,
  "mh_wait": mh_wait,
  "mh_stopthread": mh_stopthread
};

// Global workspace instance
let workspace = null;
let registeredBlocks = new Set();
let isInitialized = false;

/**
 * Initialize Blockly workspace with same configuration as main app
 */
function initWorkspace() {
  if (workspace) {
    workspace.dispose();
  }

  // Register field colour plugin - MUST be done before workspace creation
  if (!isInitialized) {
    registerFieldColour();
    Blockly.setLocale(En);
    isInitialized = true;
  }

  // Register all blocks
  Object.entries(customBlocks).forEach(([type, def]) => {
    if (def.blockdefinition) {
      // Custom block
      Blockly.Blocks[def.blockdefinition.type] = {
        init: function() {
          this.jsonInit(def.blockdefinition);
        }
      };
      registeredBlocks.add(def.blockdefinition.type);
    } else {
      // Standard block with custom color
      if (!registeredBlocks.has(type)) {
        const originalInit = Blockly.Blocks[type]?.init;
        if (originalInit) {
          Blockly.Blocks[type].init = function() {
            originalInit.call(this);
            this.setColour(def.colour);
          };
          registeredBlocks.add(type);
        }
      }
    }
  });

  // Create workspace with minimal chrome for clean screenshots
  workspace = Blockly.inject('blocklyDiv', {
    toolbox: '<xml></xml>',
    renderer: 'geras', // Default Blockly renderer (same as main app)
    scrollbars: false,
    trashcan: false,
    zoom: false,
    grid: false,
    move: {
      scrollbars: false,
      drag: false,
      wheel: false,
    },
  });

  return true;
}

/**
 * Attach shadow blocks to a parent block based on inputsForToolbox definition
 */
function attachShadowBlocks(block, inputsForToolbox) {
  if (!inputsForToolbox) return;

  Object.entries(inputsForToolbox).forEach(([inputName, inputDef]) => {
    if (inputDef.shadow) {
      try {
        // Create shadow block
        const shadowBlock = workspace.newBlock(inputDef.shadow.type);

        // Set fields from shadow definition
        if (inputDef.shadow.fields) {
          Object.entries(inputDef.shadow.fields).forEach(([fieldName, value]) => {
            shadowBlock.setFieldValue(value, fieldName);
          });
        }

        // Mark as shadow
        shadowBlock.setShadow(true);
        shadowBlock.initSvg();
        shadowBlock.render();

        // Connect to parent block's input
        const input = block.getInput(inputName);
        if (input && input.connection && shadowBlock.outputConnection) {
          input.connection.connect(shadowBlock.outputConnection);
        }
      } catch (error) {
        console.error(`Error attaching shadow block to ${inputName}:`, error);
      }
    }
  });
}

/**
 * Register a custom block definition
 */
function registerBlock(blockType, blockDefinition) {
  try {
    if (!registeredBlocks.has(blockType)) {
      Blockly.Blocks[blockType] = {
        init: function() {
          this.jsonInit(blockDefinition);
        }
      };
      registeredBlocks.add(blockType);
    }
    return true;
  } catch (error) {
    console.error(`Error registering block ${blockType}:`, error);
    return false;
  }
}

/**
 * Render a block and return its bounding box for screenshot
 */
function renderBlockToSVG(blockType, inputsForToolbox = null) {
  try {
    // Clear workspace
    workspace.clear();

    // Create the block
    const block = workspace.newBlock(blockType);
    block.initSvg();

    // Attach shadow blocks if defined
    if (inputsForToolbox) {
      attachShadowBlocks(block, inputsForToolbox);
    }

    block.render();

    // Move block away from workspace edge to avoid capturing artifacts
    block.moveTo(new Blockly.utils.Coordinate(50, 50));

    // Get the block's SVG element
    const blockSvg = block.getSvgRoot();

    // Get the bounding box relative to the viewport
    const bbox = blockSvg.getBoundingClientRect();

    // Return tight bounding box for screenshot
    return {
      x: Math.floor(bbox.x),
      y: Math.floor(bbox.y),
      width: Math.ceil(bbox.width) + 1,
      height: Math.ceil(bbox.height) + 1
    };
  } catch (error) {
    console.error(`Error rendering block ${blockType}:`, error);
    return null;
  }
}

/**
 * Render a standard Blockly block with custom color and return bounding box
 */
function renderStandardBlock(blockType, colour, inputsForToolbox = null) {
  try {
    // Clear workspace
    workspace.clear();

    // Create the block
    const block = workspace.newBlock(blockType);

    // Override the color if specified
    if (colour !== undefined && colour !== null) {
      block.setColour(colour);
    }

    block.initSvg();

    // Attach shadow blocks if defined
    if (inputsForToolbox) {
      attachShadowBlocks(block, inputsForToolbox);
    }

    block.render();

    // Move block away from workspace edge to avoid capturing artifacts
    block.moveTo(new Blockly.utils.Coordinate(50, 50));

    // Get the block's SVG element
    const blockSvg = block.getSvgRoot();

    // Get the bounding box relative to the viewport
    const bbox = blockSvg.getBoundingClientRect();

    // Return tight bounding box for screenshot
    return {
      x: Math.floor(bbox.x),
      y: Math.floor(bbox.y),
      width: Math.ceil(bbox.width) + 1,
      height: Math.ceil(bbox.height) + 1
    };
  } catch (error) {
    console.error(`Error rendering standard block ${blockType}:`, error);
    return null;
  }
}

// Make functions available globally for Puppeteer
window.blockRenderer = {
  initWorkspace,
  registerBlock,
  renderBlockToSVG,
  renderStandardBlock
};

// Auto-initialize when DOM is ready
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', initWorkspace);
} else {
  initWorkspace();
}

console.log('Block renderer ready (Vite-based, matches main app)');
