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

const commandGenerator = luaGenerator;

const customBlocks = {
    "mh_main_control_loop": {
        category: 'Control flow',
        colour: 120,
        blockdefinition: {
            "type": "mh_main_control_loop",
            "message0": "Main control loop do %1",
            "args0": [
                {
                    "type": "input_statement",
                    "name": "DO"
                }
            ],
            "colour": 120,
            "tooltip": "Main control loop",
            "helpUrl": ""
        },
        generator: (block, generator) => {

            const doCode = generator.statementToCode(block, 'DO');

            return "hub.main_control_loop(function()\n" + doCode + "end)";
        }
    },
    "mh_init": {
        category: 'Control flow',
        colour: 120,
        blockdefinition: {
            "type": "mh_init",
            "message0": "Initialization do %1",
            "args0": [
                {
                    "type": "input_statement",
                    "name": "DO"
                }
            ],
            "colour": 120,
            "tooltip": "Initialization",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            var statements = generator.statementToCode(block, 'DO');

            return "hub.init(function()\n" + statements + "end)\n";
        }
    },
    "mh_wait": {
        category: 'Control flow',
        colour: 120,
        blockdefinition: {
            "type": "mh_wait",
            "message0": "Wait %1ms",
            "args0": [
                {
                    "type": "input_value",
                    "name": "VALUE"
                }
            ],
            "previousStatement": true,
            "nextStatement": true,
            "colour": 120,
            "tooltip": "Wait",
            "helpUrl": ""
        },
        generator: (block, generator) => {

            const valueCode = generator.valueToCode(block, 'VALUE', 0);

            return "wait(" + valueCode + ")\n";
        }
    },


    "controls_if": {
        category: 'Logic',
        colour: 210,
    },
    "logic_compare": {
        category: 'Logic',
        colour: 210,
    },
    "logic_operation": {
        category: 'Logic',
        colour: 210,
    },
    "logic_negate": {
        category: 'Logic',
        colour: 210,
    },
    "logic_boolean": {
        category: 'Logic',
        colour: 210,
    },
    "logic_null": {
        category: 'Logic',
        colour: 210,
    },
    "logic_ternary": {
        category: 'Logic',
        colour: 210,
    },
    "logic_null": {
        category: 'Logic',
        colour: 210,
    },


    "controls_repeat_ext": {
        category: 'Loop',
        colour: 120,
    },
    "controls_for": {
        category: 'Loop',
        colour: 120,
    },
    "controls_flow_statements": {
        category: 'Loop',
        colour: 120,
    },


    "math_number": {
        category: 'Math',
        colour: 230,
    },
    "math_arithmetic": {
        category: 'Math',
        colour: 230,
    },
    "math_single": {
        category: 'Math',
        colour: 230,
    },
    "math_trig": {
        category: 'Math',
        colour: 230,
    },
    "math_constant": {
        category: 'Math',
        colour: 230,
    },
    "math_number_property": {
        category: 'Math',
        colour: 230,
    },
    "math_round": {
        category: 'Math',
        colour: 230,
    },
    "math_on_list": {
        category: 'Math',
        colour: 230,
    },
    "math_modulo": {
        category: 'Math',
        colour: 230,
    },
    "math_constrain": {
        category: 'Math',
        colour: 230,
    },
    "math_random_int": {
        category: 'Math',
        colour: 230,
    },
    "math_random_float": {
        category: 'Math',
        colour: 230,
    },
    "math_atan2": {
        category: 'Math',
        colour: 230,
    },

    "text": {
        category: 'Text',
        colour: 160,
    },
    "text_join": {
        category: 'Text',
        colour: 160,
    },
    "text_append": {
        category: 'Text',
        colour: 160,
    },
    "text_length": {
        category: 'Text',
        colour: 160,
    },
    "text_isEmpty": {
        category: 'Text',
        colour: 160,
    },
    "text_indexOf": {
        category: 'Text',
        colour: 160,
    },
    "text_charAt": {
        category: 'Text',
        colour: 160,
    },
    "text_getSubstring": {
        category: 'Text',
        colour: 160,
    },
    "text_changeCase": {
        category: 'Text',
        colour: 160,
    },
    "text_trim": {
        category: 'Text',
        colour: 160,
    },
    "text_count": {
        category: 'Text',
        colour: 160,
    },
    "text_replace": {
        category: 'Text',
        colour: 160,
    },
    "text_print": {
        category: 'Text',
        colour: 160,
    },

    "lists_create_empty": {
        category: 'Lists',
        colour: 160,
    },
    "lists_create_with": {
        category: 'Lists',
        colour: 160,
    },
    "lists_repeat": {
        category: 'Lists',
        colour: 160,
    },
    "lists_length": {
        category: 'Lists',
        colour: 160,
    },
    "lists_isEmpty": {
        category: 'Lists',
        colour: 160,
    },
    "lists_indexOf": {
        category: 'Lists',
        colour: 160,
    },
    "lists_getIndex": {
        category: 'Lists',
        colour: 160,
    },
    "lists_setIndex": {
        category: 'Lists',
        colour: 160,
    },
    "lists_getSublist": {
        category: 'Lists',
        colour: 160,
    },
    "lists_split": {
        category: 'Lists',
        colour: 160,
    },
    "lists_sort": {
        category: 'Lists',
        colour: 160,
    },
    "lists_reverse": {
        category: 'Lists',
        colour: 160,
    },

    "mh_digitalwrite": {
        category: 'I/O',
        colour: 230,
        blockdefinition: {
            "type": "mh_digitalwrite",
            "message0": "Digital write %1 to %2",
            "args0": [
                {
                    "type": "field_dropdown",
                    "name": "PIN",
                    "options": [
                        ["GPIO13", "GPIO13"],
                        ["GPIO16", "GPIO16"],
                        ["GPIO17", "GPIO17"],
                        ["GPIO25", "GPIO25"],
                        ["GPIO26", "GPIO26"],
                        ["GPIO27", "GPIO27"],
                        ["GPIO32", "GPIO32"],
                        ["GPIO33", "GPIO33"],
                        ["UART1_GP4", "UART1_GP4"],
                        ["UART1_GP5", "UART1_GP5"],
                        ["UART1_GP6", "UART1_GP6"],
                        ["UART1_GP7", "UART1_GP7"],
                        ["UART2_GP4", "UART2_GP4"],
                        ["UART2_GP5", "UART2_GP5"],
                        ["UART2_GP6", "UART2_GP6"],
                        ["UART2_GP7", "UART2_GP7"]
                    ]
                },
                {
                    "type": "input_value",
                    "name": "VALUE"
                }
            ],
            "previousStatement": true,
            "nextStatement": true,
            "colour": 230,
            "tooltip": "Set the state of a GPIO pin",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            const pin = block.getFieldValue('PIN');

            const valueCode = generator.valueToCode(block, 'VALUE', 0);

            return "hub.digitalWrite(" + pin + "," + valueCode + ")\n";
        }
    },
    "mh_pinmode": {
        category: 'I/O',
        colour: 230,
        blockdefinition: {
            "type": "mh_pinmode",
            "message0": "Set pin mode of %1 to %2",
            "args0": [
                {
                    "type": "field_dropdown",
                    "name": "PIN",
                    "options": [
                        ["GPIO13", "GPIO13"],
                        ["GPIO16", "GPIO16"],
                        ["GPIO17", "GPIO17"],
                        ["GPIO25", "GPIO25"],
                        ["GPIO26", "GPIO26"],
                        ["GPIO27", "GPIO27"],
                        ["GPIO32", "GPIO32"],
                        ["GPIO33", "GPIO33"],
                        ["UART1_GP4", "UART1_GP4"],
                        ["UART1_GP5", "UART1_GP5"],
                        ["UART1_GP6", "UART1_GP6"],
                        ["UART1_GP7", "UART1_GP7"],
                        ["UART2_GP4", "UART2_GP4"],
                        ["UART2_GP5", "UART2_GP5"],
                        ["UART2_GP6", "UART2_GP6"],
                        ["UART2_GP7", "UART2_GP7"]
                    ]
                },
                {
                    "type": "field_dropdown",
                    "name": "MODE",
                    "options": [
                        ["PINMODE_INPUT", "PINMODE_INPUT"],
                        ["PINMODE_INPUT_PULLUP", "PINMODE_INPUT_PULLUP"],
                        ["PINMODE_INPUT_PULLDOWN", "PINMODE_INPUT_PULLDOWN"],
                        ["PINMODE_OUTPUT", "PINMODE_OUTPUT"]
                    ]
                }
            ],
            "previousStatement": true,
            "nextStatement": true,
            "colour": 230,
            "tooltip": "Set the mode of a GPIO pin",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            const pin = block.getFieldValue('PIN');
            const mode = block.getFieldValue('MODE');

            return "hub.pinMode(" + pin + "," + mode + ")\n";
        }
    },
    "mh_digitalread": {
        category: 'I/O',
        colour: 230,
        blockdefinition: {
            "type": "mh_digitalread",
            "message0": "Digital Read %1",
            "args0": [
                {
                    "type": "field_dropdown",
                    "name": "PIN",
                    "options": [
                        ["GPIO13", "GPIO13"],
                        ["GPIO16", "GPIO16"],
                        ["GPIO17", "GPIO17"],
                        ["GPIO25", "GPIO25"],
                        ["GPIO26", "GPIO26"],
                        ["GPIO27", "GPIO27"],
                        ["GPIO32", "GPIO32"],
                        ["GPIO33", "GPIO33"],
                        ["GPIO34", "GPIO34"],
                        ["GPIO35", "GPIO35"],
                        ["GPIO36", "GPIO36"],
                        ["GPIO39", "GPIO39"],
                        ["UART1_GP4", "UART1_GP4"],
                        ["UART1_GP5", "UART1_GP5"],
                        ["UART1_GP6", "UART1_GP6"],
                        ["UART1_GP7", "UART1_GP7"],
                        ["UART2_GP4", "UART2_GP4"],
                        ["UART2_GP5", "UART2_GP5"],
                        ["UART2_GP6", "UART2_GP6"],
                        ["UART2_GP7", "UART2_GP7"]
                    ]
                },
            ],
            "output": null,
            "colour": 230,
            "tooltip": "Liest den Zustand eines GPIO-Pins",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            const pin = block.getFieldValue('PIN');

            const command = "hub.digitalRead(" + pin + ")";

            return [command, 0];
        }
    },
    "mh_set_motor_speed": {
        category: 'I/O',
        colour: 230,
        blockdefinition: {
            "type": "mh_set_motor_speed",
            "message0": "Set Motor %1 to %2",
            "args0": [
                {
                    "type": "field_dropdown",
                    "name": "PORT",
                    "options": [
                        ["PORT1", "PORT1"],
                        ["PORT2", "PORT2"],
                        ["PORT3", "PORT3"],
                        ["PORT4", "PORT4"]
                    ]
                },
                {
                    "type": "input_value",
                    "name": "VALUE"
                }
            ],
            "previousStatement": true,
            "nextStatement": true,
            "colour": 230,
            "tooltip": "Set the speed of a connected motor",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            const port = block.getFieldValue('PORT');

            const valueCode = generator.valueToCode(block, 'VALUE', 0);

            return "hub.setmotorspeed(" + port + "," + valueCode + ")\n";
        }
    },

    "mh_fastled_addleds": {
        category: 'FastLED',
        colour: 230,
        blockdefinition: {
            "type": "mh_fastled_addleds",
            "message0": "Initialize FastLED of type %1 on pin %2 with %3 LEDs",
            "args0": [
                {
                    "type": "field_dropdown",
                    "name": "TYPE",
                    "options": [
                        ["NEOPIXEL", "NEOPIXEL"],
                    ]
                },
                {
                    "type": "field_dropdown",
                    "name": "PIN",
                    "options": [
                        ["GPIO13", "GPIO13"],
                        ["GPIO16", "GPIO16"],
                        ["GPIO17", "GPIO17"],
                        ["GPIO25", "GPIO25"],
                        ["GPIO26", "GPIO26"],
                        ["GPIO27", "GPIO27"],
                        ["GPIO32", "GPIO32"],
                        ["GPIO33", "GPIO33"],
                    ]
                },
                {
                    "type": "input_value",
                    "name": "NUM_LEDS"
                }
            ],
            "previousStatement": true,
            "nextStatement": true,
            "colour": 230,
            "tooltip": "Initialize FastLED",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            const type = block.getFieldValue('TYPE');
            const pin = block.getFieldValue('PIN');
            const valueCode = generator.valueToCode(block, 'NUM_LEDS', 0);

            return "fastled.addleds(" + type + "," + pin + "," + valueCode + ")\n";
        }
    },
    "mh_fastled_show": {
        category: 'FastLED',
        colour: 230,
        blockdefinition: {
            "type": "mh_fastled_show",
            "message0": "FastLED show",
            "args0": [
            ],
            "previousStatement": true,
            "nextStatement": true,
            "colour": 230,
            "tooltip": "Shop current FastLED values to the LED strip",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            return "fastled.show()\n";
        }
    },
    "mh_fastled_clear": {
        category: 'FastLED',
        colour: 230,
        blockdefinition: {
            "type": "mh_fastled_clear",
            "message0": "FastLED clear",
            "args0": [
            ],
            "previousStatement": true,
            "nextStatement": true,
            "colour": 230,
            "tooltip": "Clear current FastLED values and sent them to the LED strip",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            return "fastled.clear()\n";
        }
    },
   "mh_fastled_set": {
        category: 'FastLED',
        colour: 230,
        blockdefinition: {
            "type": "mh_fastled_set",
            "message0": "Set LED #%1 to color %2",
            "args0": [
                {
                    "type": "input_value",
                    "name": "INDEX"
                },
                {
                    "type": "field_colour",
                    "name": "COLOR",
                    "colour": "#00a000"
                },
            ],
            "previousStatement": true,
            "nextStatement": true,
            "colour": 230,
            "tooltip": "Set LED color",
            "helpUrl": "",
            "inputsInline": true
        },
        generator: (block, generator) => {
            const index = generator.valueToCode(block, 'INDEX', 0);
            const color = block.getFieldValue('COLOR');

            const num = parseInt(color.replace('#', ''), 16);

            const r = (num >> 16) & 255;
            const g = (num >> 8) & 255;
            const b = num & 255
 
            return "fastled.set(" + index + " ," + r + ", " + g + "," + b + ")\n";
        }
    },
   
    "mh_imu_yaw": {
        category: 'IMU',
        colour: 120,
        blockdefinition: {
            "type": "mh_imu_yaw",
            "message0": "Get IMU Yaw",
            "args0": [
            ],
            "output": null,
            "colour": 120,
            "tooltip": "Wait",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            const command = "imu.yaw()";
            return [command, 0];
        }
    },
    "mh_imu_pitch": {
        category: 'IMU',
        colour: 120,
        blockdefinition: {
            "type": "mh_imu_pitch",
            "message0": "Get IMU Pitch",
            "args0": [
            ],
            "output": null,            
            "colour": 120,
            "tooltip": "Wait",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            const command = "imu.pitch()";
            return [command, 0];
        }
    },
    "mh_imu_roll": {
        category: 'IMU',
        colour: 120,
        blockdefinition: {
            "type": "mh_imu_roll",
            "message0": "Get IMU Roll",
            "args0": [
            ],
            "output": null,            
            "colour": 120,
            "tooltip": "Wait",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            const command = "imu.roll()";
            return [command, 0];
        }
    },
    "mh_imu_acceleration_x": {
        category: 'IMU',
        colour: 120,
        blockdefinition: {
            "type": "mh_imu_acceleration_x",
            "message0": "Get IMU Acceleration X",
            "args0": [
            ],
            "output": null,            
            "colour": 120,
            "tooltip": "Wait",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            const command = "imu.accelerationX()";
            return [command, 0];
        }
    },
    "mh_imu_acceleration_y": {
        category: 'IMU',
        colour: 120,
        blockdefinition: {
            "type": "mh_imu_acceleration_y",
            "message0": "Get IMU Acceleration Y",
            "args0": [
            ],
            "output": null,            
            "colour": 120,
            "tooltip": "Wait",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            const command = "imu.accelerationY()";
            return [command, 0];
        }
    },
    "mh_imu_acceleration_z": {
        category: 'IMU',
        colour: 120,
        blockdefinition: {
            "type": "mh_imu_acceleration_z",
            "message0": "Get IMU Acceleration Z",
            "args0": [
            ],
            "output": null,            
            "colour": 120,
            "tooltip": "Wait",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            const command = "imu.accelerationZ()";
            return [command, 0];
        }
    },

    "ui_show_value": {
        category: 'UI',
        colour: 10,
        blockdefinition: {
            "type": "ui_show_value",
            "message0": "Show value %1: %2 with style %3",
            "args0": [
                {
                    "type": "field_input",
                    "name": "LABEL",
                    "text": "Label"
                },
                {
                    "type": "input_value",
                    "name": "VALUE"
                },
                {
                    "type": "field_dropdown",
                    "name": "STYLE",
                    "options": [
                        ["FORMAT_SIMPLE", "FORMAT_SIMPLE"],
                    ]
                },
            ],
            "previousStatement": true,
            "nextStatement": true,
            "colour": 10,
            "tooltip": "Show value on the UI",
            "helpUrl": ""
        },
        generator: (block, generator) => {

            const labelCode = block.getFieldValue('LABEL');
            const valueCode = generator.valueToCode(block, 'VALUE', 0);
            const styleCode = block.getFieldValue('STYLE');

            return "ui.showvalue(\"" + labelCode + "\", " + styleCode + ", " + valueCode + ")\n";
        }
    },

    "mh_debug_free_heap": {
        category: 'Debug',
        colour: 230,
        blockdefinition: {
            "type": "mh_debug_free_heap",
            "message0": "Get free HEAP memory",
            "args0": [
            ],
            "output": null,
            "colour": 230,
            "tooltip": "Get free HEAP memory",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            const command = "deb.freeHeap()";

            return [command, 0];
        }
    },    
};

function generateToolbox(definitions) {
  const categories = {};

  // Blocks nach Kategorien gruppieren
  Object.entries(definitions).forEach(([type, def]) => {
      const categoryName = def.category || 'Sonstiges';

      if (!categories[categoryName]) {
          categories[categoryName] = {
              colour: def.colour || 0,
              blocks: []
          };
      }

      categories[categoryName].blocks.push({
          kind: 'block',
          type: type
      });
  });

  var box = {
      kind: 'categoryToolbox',
      contents: Object.entries(categories).map(([name, data]) => ({
          kind: 'category',
          name: name,
          colour: data.colour,
          contents: data.blocks
      }))
  };

  box.contents.push({
    kind: 'category',
    name: 'Variables',
    custom: 'VARIABLE',
    categorystyle: 'variable_category'
  });

  return box;
};

class BlocklyHTMLElement extends HTMLElement {

  workspace = null;

  connectedCallback() {
    this.innerHTML = template;

    registerFieldColour();

    Blockly.setLocale(En);
    Blockly.ContextMenuItems.registerCommentOptions();

    // Register Blocks
    Object.values(customBlocks).forEach(def => {
        if (def.blockdefinition) {
            Blockly.Blocks[def.blockdefinition.type] = {
                init: function () {
                    this.jsonInit(def.blockdefinition);
                }
            };
        }
    });

    // Register Generators
    Object.entries(customBlocks).forEach(([type, def]) => {
        if (def.generator) {
            commandGenerator.forBlock[type] = function (block) {
                return def.generator(block, commandGenerator);
            };
        }
    });

    // Workspace Setup
    this.workspace = Blockly.inject(this, {
        toolbox: generateToolbox(customBlocks),
        zoom: {
            controls: true,
            wheel: true,
            startScale: 1.0,
            maxScale: 3,
            minScale: 0.3,
            scaleSpeed: 1.2
        },
        trashcan: true
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
    const code = commandGenerator.workspaceToCode(this.workspace);
    return code;
  };

  clearWorkspace() {
    if (confirm('Workspace wirklich zurücksetzen?')) {
        workspace.clear();
    }
  };
};

customElements.define('custom-blockly', BlocklyHTMLElement);