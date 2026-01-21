#include "configuration.h"

#include "logging.h"
#include "statusmonitor.h"

#include <ArduinoJson.h>
#include <WiFi.h>
// #include <IotWebConf.h>

Configuration::Configuration(FS *fs, Megahub *hub)
	: hub_(hub)
	, wifiEnabled_(false)
	, btEnabled_(true)
	, ssid_("")
	, pwd_("") {
	fs_ = fs;
}

void Configuration::enterWiFiConfiguration() {

	/*
	// We need to start the WifiManager portal here
	//WiFiManager wm;

	Statusmonitor::instance()->setStatus(CONFIGURATION_ACTIVE);

	// Reset settings for testing (comment out in production)
	resetSettings();

	INFO("Entering configuration portal mode");

	// Set custom AP name and password (optional)
	wm.setConfigPortalTimeout(180); // 3 minutes timeout

	// Callback when entering config mode
	/*wm.setAPCallback([](WiFiManager *myWiFiManager) {
		INFO("Entered config mode SSID %s IP %s", myWiFiManager->getConfigPortalSSID().c_str(), WiFi.softAPIP().toString().c_str());
	});

	// Callback when WiFi is configured
	wm.setSaveConfigCallback([]() {
		INFO("Configuration saved!");
		Statusmonitor::instance()->setStatus(IDLE);
	});

	wm.setTitle(String("Megahub Configuration"));

	// Start portal with custom name
	String apName = String("Megahub_") + hub_->deviceUid();
	if (wm.autoConnect(apName.c_str(), "12345678")) {
		INFO("Connected wo WiFi, my IP is %s", WiFi.localIP().toString().c_str());

		Statusmonitor::instance()->setStatus(IDLE);

		// Save credentials to SD card
		JsonDocument doc;
		doc["ssid"] = WiFi.SSID().c_str();
		doc["pwd"] = WiFi.psk().c_str();

		File configFile = fs_->open("/wificonfig.json", FILE_WRITE, true);
		if (!configFile) {
			WARN("Could not create config file!");
		} else {
			serializeJson(doc, configFile);
			configFile.close();
		}

	} else {
		Serial.println("Failed to configure WiFi");
		ESP.restart();
	}*/
}

void Configuration::load() {
	INFO("Loading configuration...")

	File configFile = fs_->open("/wificonfig.json", FILE_READ);
	if (!configFile) {
		WARN("No configuration file detected!");
	} else {
		JsonDocument document;
		DeserializationError error = deserializeJson(document, configFile);

		configFile.close();

		if (error) {
			WARN("Could not read configuration file!");
		} else {

			if (document["bluetoothEnabled"].is<bool>()) {
				btEnabled_ = document["bluetoothEnabled"].as<bool>();
			}
			if (document["wifiEnabled"].is<bool>()) {
				wifiEnabled_ = document["wifiEnabled"].as<bool>();
			}

			if (document["ssid"].is<String>()) {
				ssid_ = String(document["ssid"].as<String>());
			}

			if (document["pwd"].is<String>()) {
				pwd_ = String(document["pwd"].as<String>());
			}
		}
	}
}

bool Configuration::isWiFiEnabled() {
	return wifiEnabled_;
}

bool Configuration::isBTEnabled() {
	return btEnabled_;
}

void Configuration::connectToWiFiOrShowConfigPortal() {
	if (ssid_.length() > 0) {
		INFO("Connecting to configured network SSID %s", ssid_.c_str());

		WiFi.mode(WIFI_STA);
		WiFi.begin(ssid_, pwd_);
	} else {
		enterWiFiConfiguration();
	}
}

Configuration::~Configuration() {
}

String Configuration::getAutostartProject() {
	String result = "";
	File configFile = fs_->open("/autostart.json", FILE_READ);
	if (!configFile) {
		WARN("No autostart configuration file detected!");
	} else {
		JsonDocument document;
		DeserializationError error = deserializeJson(document, configFile);

		configFile.close();

		if (error) {
			WARN("Could not read autostart configuration file!");
		} else {
			if (document["project"].is<String>()) {
				INFO("Got configured autostart project")
				result = String(document["project"].as<String>());
			} else {
				WARN("project key does not exist in configuration. No autostart configured");
			}
		}
	}
	return result;
}

bool Configuration::setAutostartProject(String projectId) {
	JsonDocument newAutostartConfig;
	newAutostartConfig["project"] = projectId;

	File configFile = fs_->open("/autostart.json", FILE_WRITE, true);
	if (!configFile) {
		WARN("Could not create autostart configuration file!");
		return false;
	}
	serializeJson(newAutostartConfig, configFile);
	configFile.close();

	return true;
}

std::vector<String> Configuration::getProjects() {
	std::vector<String> result;

	File sp = fs_->open("/project");
	File entry = sp.openNextFile();
	while (entry) {
		if (entry.isDirectory()) {
			INFO("Found project directory %s", entry.name());
			// Use Copy Constructor due to how FS works...
			result.push_back(String(entry.name()));
		}
		entry.close();
		entry = sp.openNextFile();
	}
	sp.close();

	return result;
}

bool Configuration::writeFileChunkToProject(String projectId, String fileName, uint64_t position, uint8_t *data, size_t length) {
	fs_->mkdir("/project");
	fs_->mkdir("/project/" + projectId);

	String file = "/project/" + projectId + "/" + fileName;
	File content;

	if (position == 0) {
		INFO("Creating file %s", file.c_str());
		content = fs_->open(file, FILE_WRITE, true);
	} else {
		INFO("Opening file %s for append", file.c_str());
		content = fs_->open(file, FILE_APPEND);
	}

	if (!content) {
		WARN("Failed to access file");
		return false;
	}

	if (!content.write(data, length)) {
		WARN("Failed wo write data");
		content.close();
		return false;
	}

	content.close();

	return true;
}

File Configuration::getProjectFile(String projectId, String fileName) {
	String path = "/project/" + projectId + "/" + fileName;

	return fs_->open(path, FILE_READ);
}

void Configuration::streamProjectFileTo(String projectId, String fileName, const Configuration_StreamTarget &streamTarget) {
	INFO("webserver() - start sending data to client");

	String path = "/project/" + projectId + "/" + fileName;

	File content = fs_->open(path, FILE_READ);
	if (content) {
		DEBUG("Reading file %s", path.c_str());
		char buffer[512];
		size_t read = content.readBytes(&buffer[0], sizeof(buffer));
		while (read > 0) {
			DEBUG("Sending chunk of %d bytes", read);
			streamTarget(&buffer[0], read);
			read = content.readBytes(&buffer[0], sizeof(buffer));
		}
		content.close();
	} else {
		WARN("Cannot open file %s to read", path.c_str());
	}
}

bool Configuration::deleteDirectory(String directory) {
	File dir = this->fs_->open(directory);

	if (!dir) {
		WARN("Failed to open directory: %s", directory.c_str());
		return false;
	}

	if (!dir.isDirectory()) {
		WARN("Not a directory: %s", directory.c_str());
		dir.close();
		return false;
	}

	// Iterate through all files and subdirectories
	File file = dir.openNextFile();
	while (file) {
		String filePath = directory + "/" + String(file.name());

		if (file.isDirectory()) {
			// Recursively delete subdirectory
			if (!deleteDirectory(filePath.c_str())) {
				return false;
			}
		} else {
			// Delete file
			this->fs_->remove(filePath.c_str());
			INFO("Deleted file: %s", filePath.c_str());
		}

		file.close();
		file = dir.openNextFile();
	}

	dir.close();

	// Finally, remove the directory itself
	if (this->fs_->rmdir(directory)) {
		INFO("Deleted directory: %s", directory.c_str());
		return true;
	}

	WARN("Failed to delete directory: %s", directory.c_str());
	return false;
}

void Configuration::deleteProject(String projectId) {
	String path = "/project/" + projectId;

	deleteDirectory(path);
}

bool Configuration::writeProjectFileContent(String project, String filename, String &strContent) {

	String path = "/project/" + project + "/" + filename;

	File content = fs_->open(path, FILE_WRITE, true);
	if (content) {
		DEBUG("Writing file %s", path.c_str());
		content.print(strContent);
		content.close();
		return true;
	}
	WARN("Cannot open file %s to read", path.c_str());
	return false;
}
