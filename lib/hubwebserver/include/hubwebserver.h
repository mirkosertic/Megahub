#ifndef HUBWEBSERVER_H
#define HUBWEBSERVER_H

#include "megahub.h"

#include <FS.h>
#include <PsychicHttp.h>
#include <WiFiUdp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class HubWebServer {
private:
	PsychicHttpServer *server_;
	PsychicEventSource eventSource_;
	SerialLoggingOutput *loggingOutput_;
	TaskHandle_t logforwarderTaskHandle_;

	WiFiUDP *udp_;
	long lastSSDPNotify_;
	int wsport_;
	bool started_;
	FS *fs_;
	Megahub *hub_;

	void ssdpNotify();

	void announceMDNS();
	void announceSSDP();

	String getSSDPDescription();

public:
	HubWebServer(int wsport, FS *fs, Megahub *hub, SerialLoggingOutput *loggingOutput);
	virtual ~HubWebServer();

	bool isStarted();

	void start();
	void loop();

	void publishLogMessages();
	void publishCommands();
};

#endif // HUBWEBSERVER_H
