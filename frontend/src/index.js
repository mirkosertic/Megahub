import './components/files/component.js';
import './components/logger/component.js';
import './components/sidebar-toggle/component.js';
import './components/ui/component.js';
import './components/portstatus/component.js';
import './components/blockly/component.js';
import './components/luapreview/component.js';
import './components/btdevicelist/component.js';

const mode = import.meta.env.VITE_MODE;

import {
	APP_EVENT_TYPE_COMMAND,
	APP_EVENT_TYPE_LOG,
	APP_EVENT_TYPE_PORTSTATUS,
	APP_REQUEST_TYPE_GET_AUTOSTART,
	APP_REQUEST_TYPE_GET_PROJECTS,
	APP_REQUEST_TYPE_GET_PROJECT_FILE,
	APP_REQUEST_TYPE_READY_FOR_EVENTS,
	APP_REQUEST_TYPE_STOP_PROGRAM,
	APP_REQUEST_TYPE_PUT_AUTOSTART,
	APP_REQUEST_TYPE_SYNTAX_CHECK,
	APP_REQUEST_TYPE_RUN_PROGRAM,
	APP_REQUEST_TYPE_PUT_PROJECT_FILE,
	APP_REQUEST_TYPE_DELETE_PROJECT,
	APP_EVENT_TYPE_BTCLASSICDEVICES,
	BLEClient
} from './bleclient.js'

const bleClient = new BLEClient();

var blocklyEditor = null;
var luaPreview = null;
var uiComponents = null;
var portstatus = null
var btdevicelist = null

// Functions

function generateCode() {
	const code = blocklyEditor.generateLUAPreview();

	luaPreview.highlightCode(code);
	return code;
};

function syntaxCheck() {
	const luaCode = generateCode();

	window.Application.syntaxCheck(luaCode);
};

function executeCode() {
	const luaCode = generateCode();

	window.Application.executeCode(luaCode);
};

function stopCode() {
	window.Application.stop();
}

const STRORAGE_KEY = 'blockly_robot_workspace';

// Save workspace as XML
async function saveWorkspace() {
	try {
		const xmlText = blocklyEditor.generateXML();
		await window.Application.saveProjectFile("model.xml", "application/xml; charset=UTF-8", xmlText);
		const luaCode = generateCode();
		await window.Application.saveProjectFile("program.lua", "text/x-lua; charset=UTF-8", luaCode);
		console.log("Workspace saved!");
		return true;
	} catch (error) {
		console.error('Error while saving project:', error);
		return false;
	}
};

window.Application = {

	activeProject : null,

	setInitState : function() {
		Application.setMode('btconnect');
	},

	setMode : function(mode) {
		const items = document.querySelectorAll(".dynamicvisibility");
		for (const item of items) {
			item.style.display = 'none';
			item.style.visibility = 'hidden';
			if (item.classList.contains('visible-' + mode)) {
				item.style.display = 'block';
				item.style.visibility = "visible";
			}
		}
	},

	jumpToFilesView : function() {
		window.Application.setMode('management');
		window.Application.requestProjectsAndAutostartConfig((projectList, autostartProject) => {
			document.getElementById("files").initialize(projectList, autostartProject);
		});
	},

	createProject : function(projectId) {
		console.log("Creating new empty project")
		window.Application.setMode('editor');

		window.Application.activeProject = projectId;
	},

	openProject : function(projectId) {
		console.log("Opening project " + projectId);
		window.Application.setMode('editor');

		window.Application.activeProject = projectId;

		window.Application.requestProjectFile(projectId, 'model.xml', function(projectId, filename, content) {
			if (!blocklyEditor.loadXML(content)) {
				console.log("Error loading workspace! Blockly failed to parse?");
			}
		});
	},

	deleteProject : async function(project) {
		// clang-format off
		if (mode === 'bt') {
			console.log("Deleting project " + project);
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_DELETE_PROJECT, JSON.stringify({ "project": project }));
			
			window.Application.jumpToFilesView();
		} else if (mode === 'web') {
			fetch("/project/" + encodeURIComponent(project), {
				method : "DELETE"
			})
			.then((response) => {
				console.log("Got response from backend : " + JSON.stringify(response));

				window.Application.jumpToFilesView();
			})
			.catch((error) => {
				console.error('Error deleting project:', error);
			});
		}
		// clang-format on
	},

	setAutostartProject : async function(project) {
		// clang-format off
		if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_PUT_AUTOSTART, JSON.stringify({"project" : project}));
		} else if (mode === 'web') {
			fetch("/autostart", {
				method : "PUT",
				body : JSON.stringify({"project" : project}),
				headers : {
							"Content-type" : "application/json; charset=UTF-8",
						  },
			}).then((response) => {
				console.log("Got response from backend : " + JSON.stringify(response));
			})
			.catch((error) => {
				console.error('Error deleting project:', error);
			});
		}
		// clang-format off
	},

	requestProjectFile : async function(projectId, filename, callback) {
		console.log("Requesting file " + filename + " of project " + projectId);

		if (mode === 'dev') {
			if (filename === 'model.xml') {

				const xmlText = localStorage.getItem(STRORAGE_KEY);
				if (!xmlText) {
					console.log('No data found in localStorage');
					return false;
				}

				callback(projectId, filename, xmlText);
				return;
			}

			callback(projectId, filename, '');
		} else if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_GET_PROJECT_FILE, JSON.stringify({"project" : projectId, "filename" : filename}));
			const contentResponse = new TextDecoder().decode(response);

			callback(projectId, filename, contentResponse);
		} else if (mode === 'web') {
			const response = await fetch("/project/" + encodeURIComponent(projectId) + "/" + filename);
			const responseText = await response.text();
			callback(projectId, filename, responseText);
		}
	},

	saveProjectFile: async function (filename, contentType, content) {
		var projectId = window.Application.activeProject;
		// clang-format off
		if (mode === 'dev') {
			// Dev-specific logic
			if (filename === 'model.xml') {
				localStorage.setItem(STRORAGE_KEY, content);
			}
		} else if (mode === 'bt') {
			console.log("Saving file " + filename + " to project " + projectId);
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_PUT_PROJECT_FILE, JSON.stringify({"project" : projectId, "filename" : filename, "content": content}));
		} else if (mode === 'web') {
			fetch("/project/" + encodeURIComponent(projectId) + "/" + filename, {
				method : "PUT",
				body : content,
				headers : {
						   "Content-type" : contentType,
						   },
			})
			.then((response) => {
				console.log("Got response from backend : " + JSON.stringify(response));
			});
		}
		// clang-format on
	},

	syntaxCheck : async function(luaCode) {
		// clang-format off
		if (mode === 'dev') {
			// Dev-specific logic
		} else if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_SYNTAX_CHECK, JSON.stringify({ "luaScript": luaCode }));
			const contentResponse = new TextDecoder().decode(response);
			const responsejson = JSON.parse(contentResponse);
			alert("Syntax Check Result:\nSuccess: " + responsejson.result + "\nParse Time: " + responsejson.parseTime + "ms\nError Message: " + responsejson.errorMessage);
		} else if (mode === 'web') {
			fetch("/syntaxcheck", {
				method : "PUT",
				body : luaCode,
				headers : {
						   "Content-type" : "text/x-lua; charset=UTF-8",
						   },
			})
			.then((response) => response.json())
			.then((response) => {
				alert("Syntax Check Result:\nSuccess: " + response.success + "\nParse Time: " + response.parseTime + "ms\nError Message: " + response.errorMessage);
			});
		}
		// clang-format on
	},

	executeCode : async function(luaCode) {
		// clang-format off
		if (mode === 'dev') {
			// Dev-specific logic
		} else if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_RUN_PROGRAM, JSON.stringify({ "luaScript": luaCode }));
			const contentResponse = new TextDecoder().decode(response);
			const responsejson = JSON.parse(contentResponse);
			uiComponents.clear();
			alert("Syntax Check Result:\nSuccess: " + responsejson.result);
		} else if (mode === 'web') {
			fetch("/execute", {
				method : "PUT",
				body : luaCode,
				headers : {
						   "Content-type" : "text/x-lua; charset=UTF-8",
						   },
			})
			.then((response) => response.json())
			.then((response) => {
				uiComponents.clear();
			});
		}
		// clang-format on
	},

	stop : async function() {
		// clang-format off
		if (mode === 'dev') {
			// Dev-specific logic
		} else if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_STOP_PROGRAM, JSON.stringify({}));
		} else if (mode === 'web') {
			fetch("/stop", {
				method : "PUT",
				body : '',
				headers : {
						   "Content-type" : "text/x-lua; charset=UTF-8",
						   },
			})
			.then((response) => response.json())
			.then((response) => {
				console.log("Stop command sent, response: " + JSON.stringify(response));
			});
		}
		// clang-format on
	},

	requestProjectsAndAutostartConfig : async function(callback) {
		var projects = [
			{name : 'dummy'},
			{name : 'testproject'},
		];

		if (mode === 'bt') {
			console.log("Requesting Projects...");
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_GET_PROJECTS, JSON.stringify({}));
			const responseText = new TextDecoder().decode(response);
			console.log("Got Response : (" + responseText + ")");
			const responseJson = JSON.parse(responseText);
			console.log("JSON Response was " + JSON.stringify(responseJson));
			projects = responseJson.projects;
		} else if (mode === 'web') {
			const response = await fetch("/projects");
			const responseJson = await response.json();
			projects = responseJson.projects;
		}

		var autostart = "testproject";

		if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_GET_AUTOSTART, JSON.stringify({}));
			const responseJson = JSON.parse(new TextDecoder().decode(response));
			autostart = responseJson.project;
		} else if (mode === 'web') {
			const response = await fetch("/autostart");
			const responseJson = await response.json();
			autostart = responseJson.project;
		}

		callback(projects, autostart);
	},
}

async function initBLEConnection() {
	try {
		await bleClient.connect();
		console.log('Connected!');

		const logger = document.getElementById('logger');

		portstatus.initialize();
		btdevicelist.initialize();

		bleClient.addEventListener(APP_EVENT_TYPE_LOG, (data) => {
			const logEvent = JSON.parse(new TextDecoder().decode(data));
			logger.addToLog(logEvent.message);
		});

		bleClient.addEventListener(APP_EVENT_TYPE_PORTSTATUS, (data) => {
			const status = JSON.parse(new TextDecoder().decode(data));
			portstatus.updateStatus(status);
		});

		bleClient.addEventListener(APP_EVENT_TYPE_COMMAND, (data) => {
			const command = JSON.parse(new TextDecoder().decode(data));
			uicomponents.processUIEvent(command);
		});

		bleClient.addEventListener(APP_EVENT_TYPE_BTCLASSICDEVICES, (data) => {
			const devices = JSON.parse(new TextDecoder().decode(data));
			btdevicelist.updateDevices(devices);
		});

		// Wildcard listener
		// bleClient.addEventListener('*', (appEventType, data) => {
		//	console.log(`Event Type ${appEventType} received:`, new TextDecoder().decode(data));
		//});

		// Activate Eventing
		console.log("Notifying, I am ready!");
		await bleClient.sendRequest(APP_REQUEST_TYPE_READY_FOR_EVENTS, JSON.stringify({}));

		console.log("Now I should get log messages");

		bleClient.onDisconnect(() => {
			console.log('Connection terminated!');
			window.Application.setInitState();
		});

		// We are ready to do something, we start with the files view
		window.Application.jumpToFilesView();

	} catch (error) {
		console.error('Error:', error);
	}
};

document.addEventListener('DOMContentLoaded', () => {
	blocklyEditor = document.getElementById('blockly');
	blocklyEditor.addChangeListener(() => {
		generateCode();
	});
	luaPreview = document.getElementById('luapreview');
	uiComponents = document.getElementById('uicomponents');
	portstatus = document.getElementById("portstatus");
	btdevicelist = document.getElementById("btdevicelist");

	if (mode === 'bt') {
		window.Application.setInitState();

		document.getElementById("btconnect").addEventListener("click", (ev) => {
			initBLEConnection();
		});

	} else {
		window.Application.jumpToFilesView();
	}

	if (mode == "web") {
		const eventSource = new EventSource('/events');

		eventSource.onerror = (event) => {
			console.error("EventSource failed:", event);
		};
		eventSource.addEventListener("log", (event) => {
			logger.addToLog(JSON.parse(event.data).message);
		});
		eventSource.addEventListener("command", (event) => {
			uiComponents.processUIEvent(JSON.parse(event.data));
		});
		eventSource.addEventListener("portstatus", (event) => {
			portstatus.updateStatus(JSON.parse(event.data));
		});
	}

	if (mode == "dev") {
		// Initialize BT Device List with dummy data
		btdevicelist.updateDevices({
			"devices": [
				{
					"mac": "AA:BB:CC:11:22:33",
					"name": "8BitDo M30 gamepad",
					"type": 0x09, // GAMEPAD
					"paired": true,
					"rssi": -45,
					"cod": 0x002508
				},
				{
					"mac": "DD:EE:FF:44:55:66",
					"name": "Logitech MX Master 3",
					"type": 0x0B, // MOUSE
					"paired": false,
					"rssi": -67,
					"cod": 0x002580
				},
				{
					"mac": "11:22:33:AA:BB:CC",
					"name": "Apple Magic Keyboard",
					"type": 0x0A, // KEYBOARD
					"paired": true,
					"rssi": -52,
					"cod": 0x002540
				},
				{
					"mac": "77:88:99:DD:EE:FF",
					"name": "Unknown Device",
					"type": 0x00, // UNKNOWN
					"paired": false,
					"rssi": -78,
					"cod": 0x000000
				},
				{
					"mac": "44:55:66:11:22:33",
					"name": "Sony WH-1000XM4",
					"type": 0x03, // AUDIO
					"paired": true,
					"rssi": -58,
					"cod": 0x240404
				}
			],
			"count": 5,
			"discoveryActive": true
		});

		portstatus.updateStatus({
			"ports" : [
				{
					 "id" : 1,
					 "connected" : false
				},
				{
					 "id" : 2,
					 "connected" : false
				},
				{
					 "id" : 3,
					 "connected" : true,
					 "device" : {
						"type" : "BOOST Color and Distance Sensor",
						"icon" : "⚙️",
						"modes" : [
							{
								"id" : "0",
								"name" : "COLOR"
							}
						]
					}
				},
				{
					 "id" : 4,
					 "connected" : false
				}
			]
		});
	}

	document.getElementById("reset").addEventListener("click", () => {
		blocklyEditor.clearWorkspace();
	});

	document.getElementById("syntaxcheck").addEventListener("click", () => {
		syntaxCheck();
	});

	document.getElementById("execute").addEventListener("click", () => {
		executeCode();
	});

	document.getElementById("stop").addEventListener("click", () => {
		stopCode();
	});

	// Enable auto save every 10 seconds
	setInterval(() => {
		if (window.Application.activeProject) {
			saveWorkspace();
		}
	}, 10000);
});
