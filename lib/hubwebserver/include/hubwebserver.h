#ifndef HUBWEBSERVER_H
#define HUBWEBSERVER_H

#include "configuration.h"
#include "megahub.h"

#include <FS.h>
#include <PsychicHttp.h>
#include <WiFiUdp.h>
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class HubWebServer {
private:
	std::unique_ptr<PsychicHttpServer> server_;
	PsychicEventSource eventSource_;
	SerialLoggingOutput *loggingOutput_;
	TaskHandle_t logforwarderTaskHandle_;
	Configuration *configuration_;

	std::unique_ptr<WiFiUDP> udp_;
	long lastSSDPNotify_;
	int wsport_;
	bool started_;
	FS *fs_;
	Megahub *hub_;

	void ssdpNotify();

	void announceMDNS();
	void announceSSDP();

	String getSSDPDescription();

	String urlDecode(const String &text);

public:
	HubWebServer(int wsport, FS *fs, Megahub *hub, SerialLoggingOutput *loggingOutput, Configuration *configuration);
	virtual ~HubWebServer();

	bool isStarted();

	void start();
	void loop();

	void publishLogMessages();
	void publishCommands();
	void publishPortstatus();
};

#endif // HUBWEBSERVER_H
