import './components/files/component.js';
import './components/logger/component.js';
import './components/sidebar-toggle/component.js';
import './components/ui/component.js';
import './components/portstatus/component.js';
import './components/blockly/component.js';
import './components/luapreview/component.js';

const mode = import.meta.env.VITE_MODE;

import {
	APP_EVENT_TYPE_COMMAND,
	APP_EVENT_TYPE_LOG,
	APP_EVENT_TYPE_PORTSTATUS,
	APP_REQUEST_TYPE_GET_AUTOSTART,
	APP_REQUEST_TYPE_GET_PROJECTS,
	APP_REQUEST_TYPE_GET_ROJECT_FILE,
	APP_REQUEST_TYPE_READY_FOR_EVENTS,
	BLEClient
} from './bleclient.js'

const bleClient = new BLEClient();

var blocklyEditor = null;
var luaPreview = null;
var uiComponents = null;
var portstatus = null

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

// Workspace als XML speichern
function saveWorkspace() {
	try {
		const xmlText = blocklyEditor.generateXML();
		window.Application.saveProjectFile("model.xml", "application/xml; charset=UTF-8", xmlText);
		const luaCode = generateCode();
		window.Application.saveProjectFile("program.lua", "text/x-lua; charset=UTF-8", luaCode);

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
			if (item.classList.contains('visible-' + mode)) {
				item.style.display = 'block';
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

		Application.activeProject = projectId;
	},

	openProject : function(projectId) {
		console.log("Opening project " + projectId);
		window.Application.setMode('editor');

		Application.activeProject = projectId;

		window.Application.requestProjectFile(projectId, 'model.xml', function(projectId, filename, content) {
			if (!blocklyEditor.loadXML(content)) {
				console.log("Error loading workspace! Blockly failed to parse?");
			}
		});
	},

	deleteProject : function(project) {
		if (mode === 'bt') {
			// TODO
		} else if (mode === 'web') {
			fetch("/project/" + encodeURIComponent(project), {
				method : "DELETE"
			})
				.then((response) => {
					console.log("Got response from backend : " + JSON.stringify(response));

					this.initialize();
				})
				.catch((error) => {
					console.error('Error deleting project:', error);
				});
		}
	},

	setAutostartProject : function(project) {
		if (mode === 'bt') {
			// TODO
		} else if (mode === 'web') {
			fetch("/autostart", {
				method : "PUT",
				body : JSON.stringify(
					{"project" : project}
						 ),
				headers : {
									   "Content-type" : "application/json; charset=UTF-8",
									   },
			})
				.then((response) => {
					console.log("Got response from backend : " + JSON.stringify(response));
				})
				.catch((error) => {
					console.error('Error deleting project:', error);
				});
		}
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
			}

			callback(projectId, filename, '');
		} else if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_GET_ROJECT_FILE, JSON.stringify({"project" : projectId, "filename" : filename}));
			const responseJson = JSON.parse(new TextDecoder().decode(response));
			if (responseJson.result) {
				var content = new TextDecoder().decode(response).content;
				callback(projectId, filename, content);
			} else {
				console.warn("Got error response from server : " + JSON.stringify(responseJson));
			}
		} else if (mode === 'web') {
			const response = await fetch("/project/" + encodeURIComponent(projectId) + "/" + filename);
			const responseText = await response.text();
			callback(projectId, filename, responseText);
		}
	},

	saveProjectFile : async function(projectId, filename, contentType, content) {
		if (mode === 'dev') {
			// Dev-specific logic
			if (filename === 'model.xml') {
				localStorage.setItem(STRORAGE_KEY, xmlText);
			}
		} else if (mode === 'bt') {
			// TODDO
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
	},

	syntaxCheck : async function(luaCode) {
		if (mode === 'dev') {
			// Dev-specific logic
		} else if (mode === 'bt') {
			// BT-specific logic
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
	},

	executeCode : async function(luaCode) {
		if (mode === 'dev') {
			// Dev-specific logic
		} else if (mode === 'bt') {
			// BT-specific logic
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
					alert("Syntax Check Result:\nSuccess: " + response.success + "\nParse Time: " + response.parseTime + "ms\nError Message: " + response.errorMessage);
				});
		}
	},

	stop : async function() {
		if (mode === 'dev') {
			// Dev-specific logic
		} else if (mode === 'bt') {
			// BT-specific logic
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
	},

	requestProjectsAndAutostartConfig : async function(callback) {
		var projects = [
			{name : 'dummy'},
			{name : 'testproject'},
		];

		if (mode === 'bt') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_GET_PROJECTS, JSON.stringify({}));
			const responseJson = JSON.parse(new TextDecoder().decode(response));
			projects = responseJson.projects;
		} else if (mode === 'web') {
			const response = await fetch("/projects");
			const responseJson = await response.json();
			projects = responseJson.projects;
		}

		var autostart = "testproject";

		if (mode === 'bt') {
			const response = await fetch("/autostart");
			const responseJson = await response.json();
			autostart = responseJson.project;
		} else if (mode === 'web') {
			const response = await bleClient.sendRequest(APP_REQUEST_TYPE_GET_AUTOSTART, JSON.stringify({}));
			const responseJson = JSON.parse(new TextDecoder().decode(response));
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

		// Event-Listener registrieren (Application Event Type 1 = z.B. Sensor-Event)
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
			uicomponents.processUIEvent(status);
		});

		// Wildcard-Listener
		// bleClient.addEventListener('*', (appEventType, data) => {
		//	console.log(`Event Type ${appEventType} empfangen:`, new TextDecoder().decode(data));
		//});

		// Activate Eventing
		await bleClient.sendRequest(APP_REQUEST_TYPE_READY_FOR_EVENTS, JSON.stringify({}));

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

	// Enable autp save every 10 seconds
	setInterval(() => {
		if (window.Application.activeProject) {
			saveWorkspace();
		}
	}, 10000);
});
