import template from './component.html?raw';

// Import Blockly core.
import * as Blockly from 'blockly/core';
// Import the default blocks.
import * as libraryBlocks from 'blockly/blocks';
// Import a generator.
import {luaGenerator} from 'blockly/lua';
import {registerFieldColour} from '@blockly/field-colour';
// Import a message file.
import * as En from 'blockly/msg/en';

import {definition as mh_main_control_loop} from './mh_main_control_loop.js'
import {definition as mh_init} from './mh_init.js'
import {definition as mh_wait} from './mh_wait.js'
import {definition as mh_digitalwrite} from './mh_digitalwrite.js'
import {definition as mh_pinmode} from './mh_pinmode.js'
import {definition as mh_digitalread} from './mh_digitalread.js'
import {definition as mh_set_motor_speed} from './mh_set_motor_speed.js'
import {definition as mh_fastled_addleds} from './mh_fastled_addleds.js'
import {definition as mh_fastled_show} from './mh_fastled_show.js'
import {definition as mh_fastled_clear} from './mh_fastled_clear.js'
import {definition as mh_fastled_set} from './mh_fastled_set.js'
import {definition as mh_imu_yaw} from './mh_imu_yaw.js'
import {definition as mh_imu_pitch} from './mh_imu_pitch.js'
import {definition as mh_imu_roll} from './mh_imu_roll.js'
import {definition as mh_imu_acceleration_x} from './mh_imu_acceleration_x.js'
import {definition as mh_imu_acceleration_y} from './mh_imu_acceleration_y.js'
import {definition as mh_imu_acceleration_z} from './mh_imu_acceleration_z.js'
import {definition as ui_show_value} from './ui_show_value.js'
import {definition as mh_debug_free_heap} from './mh_debug_free_heap.js'
import {definition as lego_get_mode_dataset} from './lego_get_mode_dataset.js'
import {definition as lego_select_mode} from './lego_select_mode.js'
import {definition as gamepad_buttonpressed} from './mh_gamepad_buttonpressed.js'
import {definition as gamepad_value} from './mh_gamepad_value.js'
import {definition as gamepad_connected} from './mh_gamepad_connected.js'

import {colorLogic,
	colorMath,
	colorText,
	colorLists} from './colors.js'

// clang-format off
const customBlocks = {
	"mh_main_control_loop" : mh_main_control_loop,

	"mh_init": mh_init,
		
	"mh_wait" : mh_wait,

	"controls_if" : {
		category : 'Logic',
		colour : colorLogic,
	},
	"logic_compare" : {
		category : 'Logic',
		colour : colorLogic,
	},
	"logic_operation" : {
		category : 'Logic',
		colour : colorLogic,
	},
	"logic_negate" : {
		category : 'Logic',
		colour : colorLogic,
	},
	"logic_boolean" : {
		category : 'Logic',
		colour : colorLogic,
	},
	"logic_null" : {
		category : 'Logic',
		colour : colorLogic,
	},
	"logic_ternary" : {
		category : 'Logic',
		colour : colorLogic,
	},
	"logic_null" : {
		category : 'Logic',
		colour : colorLogic,
	},
	"controls_repeat_ext" : {
		category : 'Loop',
		colour : colorLogic,
	},
	"controls_for" : {
		category : 'Loop',
		colour : colorLogic,
	},
	"controls_flow_statements" : {
		category : 'Loop',
		colour : colorLogic,
	},

	"math_number" : {
		category : 'Math',
		colour : colorMath,
	},
	"math_arithmetic" : {
		category : 'Math',
		colour : colorMath,
	},
	"math_single" : {
		category : 'Math',
		colour : colorMath,
	},
	"math_trig" : {
		category : 'Math',
		colour : colorMath,
	},
	"math_constant" : {
		category : 'Math',
		colour : colorMath,
	},
	"math_number_property" : {
		category : 'Math',
		colour : colorMath,
	},
	"math_round" : {
		category : 'Math',
		colour : colorMath,
	},
	"math_on_list" : {
		category : 'Math',
		colour : colorMath,
	},
	"math_modulo" : {
		category : 'Math',
		colour : colorMath,
	},
	"math_constrain" : {
		category : 'Math',
		colour : colorMath,
	},
	"math_random_int" : {
		category : 'Math',
		colour : colorMath,
	},
	"math_random_float" : {
		category : 'Math',
		colour : colorMath,
	},
	"math_atan2" : {
		category : 'Math',
		colour : colorMath,
	},

	"text" : {
		category : 'Text',
		colour : colorText,
	},
	"text_join" : {
		category : 'Text',
		colour : colorText,
	},
	"text_append" : {
		category : 'Text',
		colour : colorText,
	},
	"text_length" : {
		category : 'Text',
		colour : colorText,
	},
	"text_isEmpty" : {
		category : 'Text',
		colour : colorText,
	},
	"text_indexOf" : {
		category : 'Text',
		colour : colorText,
	},
	"text_charAt" : {
		category : 'Text',
		colour : colorText,
	},
	"text_getSubstring" : {
		category : 'Text',
		colour : colorText,
	},
	"text_changeCase" : {
		category : 'Text',
		colour : colorText,
	},
	"text_trim" : {
		category : 'Text',
		colour : colorText,
	},
	"text_count" : {
		category : 'Text',
		colour : colorText,
	},
	"text_replace" : {
		category : 'Text',
		colour : colorText,
	},
	"text_print" : {
		category : 'Text',
		colour : colorText,
	},

	"lists_create_empty" : {
		category : 'Lists',
		colour : colorLists,
	},
	"lists_create_with" : {
		category : 'Lists',
		colour : colorLists,
	},
	"lists_repeat" : {
		category : 'Lists',
		colour : colorLists,
	},
	"lists_length" : {
		category : 'Lists',
		colour : colorLists,
	},
	"lists_isEmpty" : {
		category : 'Lists',
		colour : colorLists,
	},
	"lists_indexOf" : {
		category : 'Lists',
		colour : colorLists,
	},
	"lists_getIndex" : {
		category : 'Lists',
		colour : colorLists,
	},
	"lists_setIndex" : {
		category : 'Lists',
		colour : colorLists,
	},
	"lists_getSublist" : {
		category : 'Lists',
		colour : colorLists,
	},
	"lists_split" : {
		category : 'Lists',
		colour : colorLists,
	},
	"lists_sort" : {
		category : 'Lists',
		colour : colorLists,
	},
	"lists_reverse" : {
		category : 'Lists',
		colour : colorLists,
	},

	"mh_digitalwrite": mh_digitalwrite,
	
	"mh_pinmode": mh_pinmode,
	
	"mh_digitalread": mh_digitalread,
	
	"mh_set_motor_speed": mh_set_motor_speed,
	
	"mh_gamepad_buttonpressed": gamepad_buttonpressed,

	"mh_gamepad_value": gamepad_value,

	"mh_gamepad_connected": gamepad_connected,

	"lego_get_mode_dataset": lego_get_mode_dataset,

	"lego_select_mode": lego_select_mode,

	"mh_fastled_addleds": mh_fastled_addleds,
	
	"mh_fastled_show": mh_fastled_show,
	
	"mh_fastled_clear": mh_fastled_clear,
	
	"mh_fastled_set" : mh_fastled_set,

	"mh_imu_yaw": mh_imu_yaw,
	
	"mh_imu_pitch": mh_imu_pitch,
	
	"mh_imu_roll": mh_imu_roll,
	
	"mh_imu_acceleration_x": mh_imu_acceleration_x,
	
	"mh_imu_acceleration_y": mh_imu_acceleration_y,
	
	"mh_imu_acceleration_z" : mh_imu_acceleration_z,

	"ui_show_value" : ui_show_value,

	"mh_debug_free_heap": mh_debug_free_heap,
};

function generateToolbox(definitions) {
	const categories = {};

	// Blocks nach Kategorien gruppieren
	Object.entries(definitions).forEach(([ type, def ]) => {
		const categoryName = def.category || 'Sonstiges';

		if (!categories[categoryName]) {
			categories[categoryName] = {
				colour : def.colour || 0,
				blocks : []
			};
		}

		categories[categoryName].blocks.push({
			kind : 'block',
			type : type
		});
	});

	var box = {
		kind : 'categoryToolbox',
		contents : Object.entries(categories).map(([ name, data ]) => ({
			kind : 'category',
			name : name,
			colour : data.colour,
			contents : data.blocks
		}))
	};

	box.contents.push({
		kind : 'category',
		name : 'Variables',
		custom : 'VARIABLE',
		categorystyle : 'variable_category'
	});

	return box;
};
// clang-format on

class BlocklyHTMLElement extends HTMLElement {
	workspace = null;

	connectedCallback() {
		this.innerHTML = template;

		registerFieldColour();

		Blockly.setLocale(En);
		Blockly.ContextMenuItems.registerCommentOptions();

		// Register Blocks
		Object.entries(customBlocks).forEach(([ type, def ]) => {
			if (def.blockdefinition) {
				Blockly.Blocks[def.blockdefinition.type] = {
					init : function() {
						this.jsonInit(def.blockdefinition);
					}
				};
			} else {
				const originalInit = Blockly.Blocks[type].init;
				Blockly.Blocks[type].init = function() {
					originalInit.call(this);
					this.setColour(def.colour);
				};
			}
		});

		// Register Generators
		Object.entries(customBlocks).forEach(([ type, def ]) => {
			if (def.generator) {
				luaGenerator.forBlock[type] = function(block) {
					return def.generator(block, luaGenerator);
				};
			}
		});

		// Workspace Setup
		this.workspace = Blockly.inject(this, {
			toolbox : generateToolbox(customBlocks),
			zoom : {
					controls : true,
					wheel : true,
					startScale : 1.0,
					maxScale : 3,
					minScale : 0.3,
					scaleSpeed : 1.2
			},
			trashcan : true
		});
	};

	addChangeListener(listener) {
		this.workspace.addChangeListener(listener);
	};

	loadXML(xmlText) {
		try {
			this.workspace.clear();
			const xml = Blockly.utils.xml.textToDom(xmlText);
			Blockly.Xml.domToWorkspace(xml, this.workspace);
			console.log('Workspace loaded!');
			return true;

		} catch (error) {
			console.error('Error while parsing workspace:', error);
			return false;
		}
	};

	generateXML() {
		const xml = Blockly.Xml.workspaceToDom(this.workspace);
		const xmlText = Blockly.Xml.domToText(xml);
		return xmlText;
	};

	generateLUAPreview() {
		const code = luaGenerator.workspaceToCode(this.workspace);
		return code;
	};

	clearWorkspace() {
		if (confirm('Workspace wirklich zurücksetzen?')) {
			this.workspace.clear();
		}
	};
};

customElements.define('custom-blockly', BlocklyHTMLElement);
