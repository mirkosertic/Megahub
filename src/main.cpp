#include <Arduino.h>

#include "SC16IS752.h"
#include "SC16IS752serialadapter.h"
#include "configuration.h"
#include "hubwebserver.h"
#include "imu.h"
#include "legodevice.h"
#include "logging.h"
#include "megahub.h"
#include "statusmonitor.h"

#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>


#define RXD2	  16
#define TXD2	  17
#define BAUD_RATE 2400

SC16IS752 *i2cuart1 = NULL;
SC16IS752 *i2cuart2 = NULL;
IMU *imu = NULL;
Megahub *megahub = NULL;
HubWebServer *webserver = NULL;
Configuration *configuration = NULL;
SerialLoggingOutput *loggingOutput = NULL;

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
	INFO("Running on Arduino : %d.%d.%d", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
	INFO("Running on ESP IDF : %s", esp_get_idf_version());
	INFO("Max  HEAP  is %d", ESP.getHeapSize());
	INFO("Free HEAP  is %d", ESP.getFreeHeap());
	INFO("Max  PSRAM is %d", ESP.getPsramSize());
	INFO("Free PSRAM is %d", ESP.getFreePsram());

	Statusmonitor::instance()->setStatus(BOOTING);

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

	INFO("Initializing Megahub...")
	megahub = new Megahub(legodevice1, legodevice2, legodevice3, legodevice4, imu);

	INFO("Loading configuration");
	configuration = new Configuration(&SD, megahub);

	INFO("Initializing WebServer instance")
	webserver = new HubWebServer(80, &SD, megahub, loggingOutput);

	configuration->loadAndApply();

	Statusmonitor::instance()->setStatus(IDLE);
}

void loop() {
	megahub->loop();

	if (WiFi.status() == WL_CONNECTED) {
		if (!webserver->isStarted()) {
			INFO("Starting WebServer as WiFi status == WL_CONNECTED")
			webserver->start();
		} else {
			webserver->loop();
		}
	}
}
