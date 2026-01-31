#include <Arduino.h>

#include "SC16IS752.h"
#include "SC16IS752serialadapter.h"
#include "btremote.h"
#include "commands.h"
#include "configuration.h"
#include "hubwebserver.h"
#include "imu.h"
#include "inputdevices.h"
#include "legodevice.h"
#include "logging.h"
#include "megahub.h"
#include "portstatus.h"
#include "statusmonitor.h"

#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <nvs_flash.h>

SC16IS752 *i2cuart1 = NULL;
SC16IS752 *i2cuart2 = NULL;
IMU *imu = NULL;
Megahub *megahub = NULL;
HubWebServer *webserver = NULL;
Configuration *configuration = NULL;
SerialLoggingOutput *loggingOutput = NULL;
InputDevices *inputDevices = NULL;
BTRemote *btremote = NULL;

#define GPIO_SPI_SS	  GPIO_NUM_4
#define GPIO_SPI_SCK  GPIO_NUM_18
#define GPIO_SPI_MOSI GPIO_NUM_23
#define GPIO_SPI_MISO GPIO_NUM_19

void setup() {
	Serial.begin(115200);

	// Initialize logging framework
	loggingOutput = new SerialLoggingOutput(&Serial);
	Logging::instance()->routeLoggingTo(loggingOutput);

	INFO("Setup started");
	INFO("ESP Chip model     : %s", ESP.getChipModel());
	INFO("ESP Chip revision  : %d", ESP.getChipRevision());
	INFO("Running on Arduino : %d.%d.%d", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
	INFO("Running on ESP IDF : %s", esp_get_idf_version());
	INFO("Max  HEAP  is %d", ESP.getHeapSize());
	INFO("Free HEAP  is %d", ESP.getFreeHeap());
	INFO("Max  PSRAM is %d", ESP.getPsramSize());
	INFO("Free PSRAM is %d", ESP.getFreePsram());

	// Force init at this point
	Statusmonitor::instance()->setStatus(BOOTING);
	Commands::instance();
	Portstatus::instance();

	INFO("Initializing SPI bus...");
	SPI.begin(GPIO_SPI_SCK, GPIO_SPI_MISO, GPIO_SPI_MOSI, GPIO_SPI_SS);
	INFO("Mounting SD card...");
	if (!SD.begin(GPIO_SPI_SS)) {
		WARN("Card Mount failed!");
		while (true) {
			Statusmonitor::instance()->setStatus(SD_ERROR);
			vTaskDelay(pdMS_TO_TICKS(50));
		}
	} else {
		uint8_t cardType = SD.cardType();

		if (cardType == CARD_NONE) {
			INFO("No SD card attached");
		} else {
			if (cardType == CARD_MMC) {
				INFO("SD card type MMC");
			} else if (cardType == CARD_SD) {
				INFO("SD card type SDSC");
			} else if (cardType == CARD_SDHC) {
				INFO("SD card type SDHC");
			} else {
				INFO("SD card type UNKNOWN");
			}

			uint64_t cardSize = SD.cardSize() / (1024 * 1024);
			INFO("SD Card Size: %lluMB", cardSize);
		}
	}
	INFO("Free HEAP  is %d", ESP.getFreeHeap());

	INFO("Initializing input decvices registry");
	inputDevices = new InputDevices();

	INFO("Initializing SC16IS752 1 on I2C bus...");
	i2cuart1 = new SC16IS752(SC16IS750_PROTOCOL_I2C, SC16IS750_ADDRESS_AA);
	i2cuart1->begin(2400, 2400);

	INFO("Initializing SC16IS752 2 on I2C bus...");
	i2cuart2 = new SC16IS752(SC16IS750_PROTOCOL_I2C, SC16IS750_ADDRESS_BB);
	i2cuart2->begin(2400, 2400);

	INFO("Initializing Lego Device ports ...");
	SC16IS752SerialAdapter *adapter1 = new SC16IS752SerialAdapter(i2cuart1, CHANNEL_A, 0, 1);
	LegoDevice *legodevice1 = new LegoDevice(adapter1);

	SC16IS752SerialAdapter *adapter2 = new SC16IS752SerialAdapter(i2cuart1, CHANNEL_B, 2, 3);
	LegoDevice *legodevice2 = new LegoDevice(adapter2);

	SC16IS752SerialAdapter *adapter3 = new SC16IS752SerialAdapter(i2cuart2, CHANNEL_A, 0, 1);
	LegoDevice *legodevice3 = new LegoDevice(adapter3);

	SC16IS752SerialAdapter *adapter4 = new SC16IS752SerialAdapter(i2cuart2, CHANNEL_B, 2, 3);
	LegoDevice *legodevice4 = new LegoDevice(adapter4);

	INFO("Initializing IMU");
	imu = new IMU();
	INFO("Free HEAP  is %d", ESP.getFreeHeap());

	INFO("Initializing Megahub...")
	megahub = new Megahub(inputDevices, legodevice1, legodevice2, legodevice3, legodevice4, imu);
	INFO("Free HEAP  is %d", ESP.getFreeHeap());

	INFO("Loading configuration");
	configuration = new Configuration(&SD, megahub);
	configuration->load();
	INFO("Free HEAP  is %d", ESP.getFreeHeap());

	if (configuration->isWiFiEnabled()) {
		configuration->connectToWiFiOrShowConfigPortal();
		// We only reach this point in case there is no configuration needed...
		INFO("Initializing WebServer instance")
		webserver = new HubWebServer(80, &SD, megahub, loggingOutput, configuration);
		INFO("Free HEAP  is %d", ESP.getFreeHeap());
	}

	if (configuration->isBTEnabled()) {
		INFO("Initializing NVS for Bluetooth bonding storage...");
		esp_err_t ret = nvs_flash_init();
		if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
			WARN("NVS partition needs to be erased, erasing...");
			ESP_ERROR_CHECK(nvs_flash_erase());
			ret = nvs_flash_init();
		}
		if (ret != ESP_OK) {
			ERROR("NVS initialization failed: %s", esp_err_to_name(ret));
		} else {
			INFO("NVS initialized successfully");
		}

		INFO("Initializing BT Remote interface")
		btremote = new BTRemote(&SD, inputDevices, megahub, loggingOutput, configuration);
		btremote->begin(megahub->name().c_str());
		INFO("Free HEAP  is %d", ESP.getFreeHeap());
	}

	INFO("Setup completed");
	Statusmonitor::instance()->setStatus(IDLE);

	INFO("Free HEAP  is %d", ESP.getFreeHeap());
	INFO("Free PSRAM is %d", ESP.getFreePsram());
}

void loop() {
	static unsigned long lastHeapLog = 0;
	unsigned long currentMillis = millis();

	// Log free heap memory every 10 seconds
	if (currentMillis - lastHeapLog >= 10000) {
		INFO("Free HEAP: %d bytes, Free PSRAM : %d bytes",
			ESP.getFreeHeap(), ESP.getFreePsram());

		char task_list_buffer[1024];
		vTaskList(task_list_buffer);
		printf("%s\n", task_list_buffer);

		lastHeapLog = currentMillis;
	}

	if (configuration->isBTEnabled()) {
		btremote->loop();
	}

	megahub->loop();

	if (configuration->isWiFiEnabled()) {
		if (WiFi.status() == WL_CONNECTED) {
			if (!webserver->isStarted()) {
				INFO("Starting WebServer as WiFi status == WL_CONNECTED. Local IP / URL: http://%s:80/", WiFi.localIP().toString().c_str());
				webserver->start();
			} else {
				webserver->loop();
			}
		}
	}
}
