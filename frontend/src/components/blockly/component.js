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

import {definition as mh_startthread} from './mh_startthread.js'
import {definition as mh_stopthread} from './mh_stopthread.js'
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
import {definition as mh_imu_value} from './mh_imu_value.js'
import {definition as ui_show_value} from './ui_show_value.js'
import {definition as mh_debug_free_heap} from './mh_debug_free_heap.js'
import {definition as mh_port} from './mh_port.js'
import {definition as lego_get_mode_dataset} from './lego_get_mode_dataset.js'
import {definition as lego_select_mode} from './lego_select_mode.js'
import {definition as gamepad_gamepad} from './mh_gamepad_gamepad.js'
import {definition as gamepad_buttonpressed} from './mh_gamepad_buttonpressed.js'
import {definition as gamepad_value} from './mh_gamepad_value.js'
import {definition as gamepad_connected} from './mh_gamepad_connected.js'
import {definition as mh_debug_millis} from './mh_debug_millis.js'

import {colorLogic,
	colorMath,
	colorText,
	colorLists} from './colors.js'

// clang-format off
const customBlocks = {
	"mh_init": mh_init,
		
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

	"mh_port": mh_port,	
	
	"mh_set_motor_speed": mh_set_motor_speed,

	"mh_gamepad_gamepad": gamepad_gamepad,	
	
	"mh_gamepad_buttonpressed": gamepad_buttonpressed,

	"mh_gamepad_value": gamepad_value,

	"mh_gamepad_connected": gamepad_connected,

	"lego_get_mode_dataset": lego_get_mode_dataset,

	"lego_select_mode": lego_select_mode,

	"mh_fastled_addleds": mh_fastled_addleds,
	
	"mh_fastled_show": mh_fastled_show,
	
	"mh_fastled_clear": mh_fastled_clear,
	
	"mh_fastled_set" : mh_fastled_set,

	"mh_imu_value": mh_imu_value,
	
	"ui_show_value" : ui_show_value,

	"mh_debug_free_heap": mh_debug_free_heap,

	"mh_debug_millis": mh_debug_millis,
	
	"mh_startthread" : mh_startthread,

	"mh_wait" : mh_wait,

	"mh_stopthread": mh_stopthread
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

		if (def.inputsForToolbox) {
			categories[categoryName].blocks.push({
				kind : 'block',
				type : type,
				inputs: def.inputsForToolbox
			});
		} else {
			categories[categoryName].blocks.push({
				kind : 'block',
				type : type
			});
		}
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
		this.workspace.clear();
	};

	getPerformanceColor(ratio) {
		// ratio: 0 = fast (green), 1 = slow (red)
		if (ratio < 0.3) {
			return '#4ec9b0'; // VSCode teal (fast)
		} else if (ratio < 0.6) {
			return '#dcdcaa'; // VSCode yellow (moderate)
		} else if (ratio < 0.8) {
			return '#ce9178'; // VSCode orange (slow)
		} else {
			return '#f48771'; // VSCode red (very slow)
		}
	};

	addProfilingOverlay(blockId, minDuration, avgDuration, maxDuration) {
		const workspace = Blockly.getMainWorkspace();
		const block = workspace.getBlockById(blockId);

		if (!block)
			return;

		const blockSvg = block.getSvgRoot();

		// IMPORTANT: Remove old overlay BEFORE getting bbox
		let overlay = blockSvg.querySelector('.profile-overlay');
		if (overlay) {
			overlay.remove();
		}

		// Now get bbox without the overlay affecting it
		const bbox = blockSvg.getBBox();

		// Create new overlay group
		overlay = document.createElementNS('http://www.w3.org/2000/svg', 'g');
		overlay.classList.add('profile-overlay');

		// Calculate performance color based on average duration
		const maxDurationThreshold = 50000; // 50ms in microseconds
		const perfRatio = Math.min(avgDuration / maxDurationThreshold, 1);
		const perfColor = this.getPerformanceColor(perfRatio);

		// Outer glow/border for the entire block
		const glowRect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
		glowRect.setAttribute('x', -2);
		glowRect.setAttribute('y', -2);
		glowRect.setAttribute('width', bbox.width + 4);
		glowRect.setAttribute('height', bbox.height + 4);
		glowRect.setAttribute('rx', 4);
		glowRect.setAttribute('fill', 'none');
		glowRect.setAttribute('stroke', perfColor);
		glowRect.setAttribute('stroke-width', '1.5');
		glowRect.setAttribute('opacity', '0.4');
		glowRect.style.filter = 'drop-shadow(0 0 3px ' + perfColor + ')';

		// Badge container - wider for 7-digit numbers
		const badgeWidth = 200;
		const badgeHeight = 30;
		const badgeX = bbox.width - badgeWidth - 5;
		const badgeY = -badgeHeight - 5;

		// Badge background with subtle gradient
		const defs = document.createElementNS('http://www.w3.org/2000/svg', 'defs');
		const gradient = document.createElementNS('http://www.w3.org/2000/svg', 'linearGradient');
		gradient.setAttribute('id', `profile-grad-${blockId}`);
		gradient.setAttribute('x1', '0%');
		gradient.setAttribute('y1', '0%');
		gradient.setAttribute('x2', '0%');
		gradient.setAttribute('y2', '100%');

		const stop1 = document.createElementNS('http://www.w3.org/2000/svg', 'stop');
		stop1.setAttribute('offset', '0%');
		stop1.setAttribute('stop-color', '#2d2d30'); // VSCode dark bg
		stop1.setAttribute('stop-opacity', '0.95');

		const stop2 = document.createElementNS('http://www.w3.org/2000/svg', 'stop');
		stop2.setAttribute('offset', '100%');
		stop2.setAttribute('stop-color', '#1e1e1e'); // VSCode darker
		stop2.setAttribute('stop-opacity', '0.95');

		gradient.appendChild(stop1);
		gradient.appendChild(stop2);
		defs.appendChild(gradient);
		overlay.appendChild(defs);

		// Badge background rectangle
		const badge = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
		badge.setAttribute('x', badgeX);
		badge.setAttribute('y', badgeY);
		badge.setAttribute('width', badgeWidth);
		badge.setAttribute('height', badgeHeight);
		badge.setAttribute('rx', 3);
		badge.setAttribute('fill', `url(#profile-grad-${blockId})`);
		badge.setAttribute('stroke', perfColor);
		badge.setAttribute('stroke-width', '1');
		badge.setAttribute('opacity', '0.95');

		// Helper function to format microseconds
		function formatMicros(micros) {
			// Always show in microseconds with comma separators for readability
			return Math.round(micros).toLocaleString() + 'µs';
		}

		// Column positions with more space between them
		const col1X = badgeX + 8;
		const col2X = badgeX + 72;
		const col3X = badgeX + 136;

		// First row: Min / Avg / Max labels
		const labelY = badgeY + 11;
		const labelSize = '8';
		const labelColor = '#858585'; // VSCode dim text

		const minLabel = document.createElementNS('http://www.w3.org/2000/svg', 'text');
		minLabel.setAttribute('x', col1X);
		minLabel.setAttribute('y', labelY);
		minLabel.setAttribute('fill', labelColor);
		minLabel.setAttribute('font-size', labelSize);
		minLabel.setAttribute('font-family', "'Consolas', 'Monaco', 'Courier New', monospace");
		minLabel.textContent = 'min';

		const avgLabel = document.createElementNS('http://www.w3.org/2000/svg', 'text');
		avgLabel.setAttribute('x', col2X);
		avgLabel.setAttribute('y', labelY);
		avgLabel.setAttribute('fill', labelColor);
		avgLabel.setAttribute('font-size', labelSize);
		avgLabel.setAttribute('font-family', "'Consolas', 'Monaco', 'Courier New', monospace");
		avgLabel.textContent = 'avg';

		const maxLabel = document.createElementNS('http://www.w3.org/2000/svg', 'text');
		maxLabel.setAttribute('x', col3X);
		maxLabel.setAttribute('y', labelY);
		maxLabel.setAttribute('fill', labelColor);
		maxLabel.setAttribute('font-size', labelSize);
		maxLabel.setAttribute('font-family', "'Consolas', 'Monaco', 'Courier New', monospace");
		maxLabel.textContent = 'max';

		// Second row: Min / Avg / Max values
		const valueY = badgeY + 23;
		const valueSize = '9';

		const minValue = document.createElementNS('http://www.w3.org/2000/svg', 'text');
		minValue.setAttribute('x', col1X);
		minValue.setAttribute('y', valueY);
		minValue.setAttribute('fill', '#4ec9b0'); // VSCode teal for min (good)
		minValue.setAttribute('font-size', valueSize);
		minValue.setAttribute('font-family', "'Consolas', 'Monaco', 'Courier New', monospace");
		minValue.setAttribute('font-weight', 'bold');
		minValue.textContent = formatMicros(minDuration);

		const avgValue = document.createElementNS('http://www.w3.org/2000/svg', 'text');
		avgValue.setAttribute('x', col2X);
		avgValue.setAttribute('y', valueY);
		avgValue.setAttribute('fill', perfColor); // Color-coded based on performance
		avgValue.setAttribute('font-size', valueSize);
		avgValue.setAttribute('font-family', "'Consolas', 'Monaco', 'Courier New', monospace");
		avgValue.setAttribute('font-weight', 'bold');
		avgValue.textContent = formatMicros(avgDuration);

		const maxValue = document.createElementNS('http://www.w3.org/2000/svg', 'text');
		maxValue.setAttribute('x', col3X);
		maxValue.setAttribute('y', valueY);
		maxValue.setAttribute('fill', '#f48771'); // VSCode red for max (worst case)
		maxValue.setAttribute('font-size', valueSize);
		maxValue.setAttribute('font-family', "'Consolas', 'Monaco', 'Courier New', monospace");
		maxValue.setAttribute('font-weight', 'bold');
		maxValue.textContent = formatMicros(maxDuration);

		// Add connecting line from badge to block
		const connector = document.createElementNS('http://www.w3.org/2000/svg', 'line');
		connector.setAttribute('x1', badgeX + badgeWidth / 2);
		connector.setAttribute('y1', badgeY + badgeHeight);
		connector.setAttribute('x2', badgeX + badgeWidth / 2);
		connector.setAttribute('y2', badgeY + badgeHeight + 5);
		connector.setAttribute('stroke', perfColor);
		connector.setAttribute('stroke-width', '1');
		connector.setAttribute('opacity', '0.3');
		connector.setAttribute('stroke-dasharray', '2,2');

		// Assemble overlay
		overlay.appendChild(glowRect);
		overlay.appendChild(connector);
		overlay.appendChild(badge);
		overlay.appendChild(minLabel);
		overlay.appendChild(avgLabel);
		overlay.appendChild(maxLabel);
		overlay.appendChild(minValue);
		overlay.appendChild(avgValue);
		overlay.appendChild(maxValue);

		// Blockly blocks have this structure: path elements first, then other decorations
		// We insert the overlay after all existing children
		const lastChild = blockSvg.lastChild;
		if (lastChild && lastChild.classList && lastChild.classList.contains('blocklyEditableText')) {
			// If last child is editable text, insert after it
			blockSvg.appendChild(overlay);
		} else {
			// Otherwise just append
			blockSvg.appendChild(overlay);
		}
	};

	removeAllProfilingOverlays() {
		const allBlocks = this.workspace.getAllBlocks(false);
		
		allBlocks.forEach(block => {
			const blockSvg = block.getSvgRoot();
			if (blockSvg) {
				const overlay = blockSvg.querySelector('.profile-overlay');
				if (overlay) {
					overlay.remove();
				}
			}
		});
	};
};

customElements.define('custom-blockly', BlocklyHTMLElement);
