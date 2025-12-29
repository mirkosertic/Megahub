#include "configuration.h"

#include "logging.h"
#include "statusmonitor.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

const char *TEST_LUA_LUA = "hub.init(function()\n  print('Initializing this')\n  hub.setmotorspeed(PORT1,0)\n  print('This is a very cool example of Lua in Action!')\nend)\n\nhub.main_control_loop(function()\n  print('Main control loop')\n  hub.setmotorspeed(PORT1,255)\n  wait(1000)\n  hub.setmotorspeed(PORT1,(-255))\n  wait(1000)\nend)";

Configuration::Configuration(FS *fs, Megahub *hub)
	: hub_(hub) {
	fs_ = fs;
}

void Configuration::enterWiFiConfiguration() {

	// We need to start the WifiManager portal here
	WiFiManager wm;

	Statusmonitor::instance()->setStatus(CONFIGURATION_ACTIVE);
    
    // Reset settings for testing (comment out in production)
    wm.resetSettings();

	INFO("Entering configuration portal mode");
    
    // Set custom AP name and password (optional)
    wm.setConfigPortalTimeout(180); // 3 minutes timeout
    
    // Callback when entering config mode
    wm.setAPCallback([](WiFiManager* myWiFiManager) {
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
    }
}

void Configuration::loadAndApply() {
	INFO("Loading and configuring...")

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

			String ssid = String(document["ssid"].as<String>());
			String pwd = String(document["pwd"].as<String>());

			INFO("Connecting to configured network SSID %s", ssid.c_str());

			WiFi.mode(WIFI_STA);
			WiFi.begin(ssid, pwd);

			return;
		}
	}


	enterWiFiConfiguration();

	//hub_->loadLUA(TEST_LUA_LUA);
}

Configuration::~Configuration() {
}
