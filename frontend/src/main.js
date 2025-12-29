// Import Blockly core.
import * as Blockly from 'blockly/core';
// Import the default blocks.
import * as libraryBlocks from 'blockly/blocks';
// Import a generator.
import {luaGenerator} from 'blockly/lua';
import {registerFieldColour} from '@blockly/field-colour';
// Import a message file.
import * as En from 'blockly/msg/en';

import Prism from 'prismjs'
import 'prismjs/components/prism-lua'
import 'prismjs/plugins/line-numbers/prism-line-numbers'
import 'prismjs/themes/prism-tomorrow.css'
import 'prismjs/plugins/line-numbers/prism-line-numbers.css'

registerFieldColour();

Blockly.setLocale(En);
Blockly.ContextMenuItems.registerCommentOptions()

const commandGenerator = luaGenerator;
commandGenerator.finish = function (code) {
    var initCode = '';
    if (commandGenerator.init_ && commandGenerator.init_.length > 0) {
        initCode = commandGenerator.init_.join('\n') + '\n\n';
    }

    delete commandGenerator.init_;

    return initCode + code;
};

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

            if (!generator.init_) {
                generator.init_ = [];
            }
            generator.init_.push("hub.init(function()\n" + statements + "end)");

            return '';
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
            "tooltip": "Setzt einen GPIO-Pin auf HIGH oder LOW",
            "helpUrl": ""
        },
        generator: (block, generator) => {
            const pin = block.getFieldValue('PIN');

            const valueCode = generator.valueToCode(block, 'VALUE', 0);

            return "hub.digitalWrite(" + pin + "," + valueCode + ")\n";
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

};

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
}


// Workspace Setup
const workspace = Blockly.inject('blocklyDiv', {
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

// Functions

function generateCode() {
    const code = commandGenerator.workspaceToCode(workspace);

    document.getElementById('luaeditor').innerHTML = `<pre class="line-numbers"><code class="language-lua">${Prism.highlight(code, Prism.languages.lua, 'lua')}</code></pre>`;

    // Highlight all code blocks
    Prism.highlightAll();
    return code;
}

function syntaxCheck() {
    const luaCode = generateCode();

    // Perform syntax check
    // Write code to backend
    fetch("/syntaxcheck", {
        method: "PUT",
        body: luaCode,
        headers: {
            "Content-type": "text/x-lua; charset=UTF-8",
        },
    })
    .then((response) => response.json())
    .then((response) => {
        alert("Syntax Check Result:\nSuccess: " + response.success + "\nParse Time: " + response.parseTime + "ms\nError Message: " + response.errorMessage);
    });
}

function executeCode() {
    const luaCode = generateCode();

    fetch("/execute", {
        method: "PUT",
        body: luaCode,
        headers: {
            "Content-type": "text/x-lua; charset=UTF-8",
        },
    })
    .then((response) => response.json())
    .then((response) => {
        alert("Syntax Check Result:\nSuccess: " + response.success + "\nParse Time: " + response.parseTime + "ms\nError Message: " + response.errorMessage);
    });
}

document.getElementById("reset").addEventListener("click", () => {
    clearWorkspace();
});

document.getElementById("syntaxcheck").addEventListener("click", () => {
    syntaxCheck();
});

document.getElementById("execute").addEventListener("click", () => {
    executeCode();
});

const STRORAGE_KEY = 'blockly_robot_workspace';

// Workspace als XML speichern
function saveToLocalStorage(key) {
    try {
        const xml = Blockly.Xml.workspaceToDom(workspace);
        const xmlText = Blockly.Xml.domToText(xml);
        localStorage.setItem(key, xmlText);
        console.log('Workspace saved to localStorage, sending backup to backend');

        if (!import.meta.env.DEV) {
            // Write model to backend
            fetch("model.xml", {
                method: "PUT",
                body: xmlText,
                headers: {
                    "Content-type": "application/xml; charset=UTF-8",
                },
            })
            .then((response) => {
                console.log("Got response from backend : " + response);
            });

            const luaCode = generateCode();
            // Write code to backend
            fetch("program.lua", {
                method: "PUT",
                body: luaCode,
                headers: {
                    "Content-type": "text/x-lua; charset=UTF-8",
                },
            })
            .then((response) => {
                console.log("Got response from backend : " + response);
            });
        }

        return true;
    } catch (error) {
        console.error('Error while saving project:', error);
        return false;
    }
}

function loadXML(xmlText) {
    try {
        workspace.clear();
        const xml = Blockly.utils.xml.textToDom(xmlText);
        Blockly.Xml.domToWorkspace(xml, workspace);
        console.log('Workspace loaded!');
        return true;

    } catch (error) {
        console.error('Error while parsing workspace:', error);
        return false;
    }
}

// Workspace aus LocalStorage laden
function loadFromLocalStorage(key) {
    try {
        console.info("Trying to load from localStorage");
        const xmlText = localStorage.getItem(key);
        if (!xmlText) {
            console.log('No data found in localStorage');
            return false;
        }

        loadXML(xmlText);

        return true;
    } catch (error) {
        console.error('Error while loading workspace:', error);
        return false;
    }
}

function clearWorkspace() {
    if (confirm('Workspace wirklich zurücksetzen?')) {
        workspace.clear();
    }
}

// Auto-generate on change
workspace.addChangeListener(() => {
    generateCode();
});

// Enable autp save every 10 seconds
setInterval(() => {
    saveToLocalStorage(STRORAGE_KEY);
}, 10000);

const MAX_LOG_ENTRIES = 50;
let logCount = 0;

function addLog(message) {
    const container = document.getElementById('log-container');
    const logEntry = document.createElement('div');
    logEntry.className = 'log-entry';
   
    logEntry.textContent = `${message}`;
    
    // Add new entry at the top
    container.insertBefore(logEntry, container.firstChild);
    logCount++;
    
    // Remove oldest entry if limit exceeded
    if (logCount > MAX_LOG_ENTRIES) {
        container.removeChild(container.lastChild);
        logCount--;
    }
}

function processUIEvent(event) {
    if (event.type === "show_value") {
        const label = event.label || "Value";
        const format = event.format || "FORMAT_SIMPLE";
        const value = event.value || 0;

        var element = document.querySelector('[data-label="' + label + '"]');
        if (element === null) {
            // Create new element
            element = document.createElement('div');
            element.className = 'ui-value-entry';
            element.setAttribute('data-label', label);
            element.innerHTML = `${value}`;

            document.getElementById('uicomponents').appendChild(element);
        } else {
            element.innerHTML = `${value}`;
        }
    }
    console.log("UI Event: ", event);
}

// Try to load a program
if (!import.meta.env.DEV) {
    fetch("model.xml")
        .then((response) => response.text())
        .then((text) => {
            if (!loadXML(text)) {
                // Load last known workspace version
                loadFromLocalStorage(STRORAGE_KEY);
            }
        })
        .catch((error) => {
                console.error('Error loading model.xml:', error);
        });

    var eventSource = new EventSource('/events');
    eventSource.onerror = (event) => {
        console.error("EventSource failed:", event);
    };
    eventSource.addEventListener("log", (event) => {
        addLog(JSON.parse(event.data).message);
    });
    eventSource.addEventListener("command", (event) => {
        processUIEvent(JSON.parse(event.data));
    });
}