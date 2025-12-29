#include <Arduino.h>

#include "hubwebserver.h"

#include "commands.h"
#include "embedded_files.h"
#include "logging.h"

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WiFi.h>

void logForwarderTask(void *param) {
	HubWebServer *server = (HubWebServer *) param;
	while (true) {
		server->publishLogMessages();
		server->publishCommands();
	}
}

HubWebServer::HubWebServer(int wsport, FS *fs, Megahub *hub, SerialLoggingOutput *loggingOutput) {
	server_ = new PsychicHttpServer();
	loggingOutput_ = loggingOutput;
	lastSSDPNotify_ = millis();
	wsport_ = wsport;
	started_ = false;
	fs_ = fs;
	hub_ = hub;
	udp_ = nullptr;
}

HubWebServer::~HubWebServer() {
	delete server_;
	if (udp_) {
		delete udp_;
	}
}

void HubWebServer::announceMDNS() {
	String technicalName = hub_->deviceUid().c_str();
	if (MDNS.begin(technicalName)) {
		INFO("Registered as mDNS-Name %s", technicalName.c_str());
	} else {
		WARN("Registered as mDNS-Name %s failed", technicalName.c_str());
	}
}

void HubWebServer::announceSSDP() {
	INFO("Initializing SSDP...");

	const IPAddress SSDP_MULTICAST_ADDR(239, 255, 255, 250);
	const uint16_t SSDP_PORT = 1900;

	udp_ = new WiFiUDP();

	// Join multicast group for SSDP
	if (udp_->beginMulticast(SSDP_MULTICAST_ADDR, SSDP_PORT)) {
		INFO("SSDP multicast joined successfully");
	} else {
		WARN("Failed to join SSDP multicast group");
	}
}

String HubWebServer::getSSDPDescription() {
	String deviceuid = hub_->deviceUid();
	String name = hub_->name();
	;
	String manufacturer = hub_->manufacturer();
	String devicetype = hub_->deviceType();
	String version = hub_->version();
	String serialnumber = hub_->serialnumber();
	String configurationurl = "http://" + String(deviceuid.c_str()) + ".local:" + wsport_ + "/project/lala/";

	String xml = "<?xml version='1.0'?>";
	xml += "<root xmlns='urn:schemas-upnp-org:device-1-0'>";
	xml += "<specVersion><major>1</major><minor>0</minor></specVersion>";
	xml += "<device>";
	xml += "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>";
	xml += "<friendlyName>" + name + "</friendlyName>";
	xml += "<manufacturer>" + manufacturer + "</manufacturer>";
	xml += "<modelName>" + devicetype + "</modelName>";
	xml += "<modelNumber>" + version + "</modelNumber>";
	xml += "<serialNumber>" + serialnumber + "</serialNumber>";
	xml += "<UDN>uuid:" + deviceuid + "</UDN>";
	xml += "<presentationURL>" + configurationurl + "</presentationURL>";
	xml += "</device>";
	xml += "</root>";
	return xml;
}

void HubWebServer::start() {

	announceMDNS();
	announceSSDP();

	server_->config.max_uri_handlers = 40;
	server_->setPort(80);
	server_->start();

	server_->on("/", HTTP_GET, [this](PsychicRequest *request, PsychicResponse *resp) {

      INFO("webserver() - Rendering / page");

      // Chunked response to optimize RAM usage
      size_t content_length = index_html_gz_len;

      INFO("webserver() - Size of response is %d", content_length);
      PsychicStreamResponse response(resp, "text/html");

      response.addHeader("Cache-Control", "no-cache, must-revalidate");
	  response.addHeader("Content-Encoding", "gzip");
      response.setContentLength(content_length);

      response.beginSend();
	  response.write(index_html_gz, index_html_gz_len);
      return response.endSend(); });

	server_->on("/main.js", HTTP_GET, [this](PsychicRequest *request, PsychicResponse *resp) {

      INFO("webserver() - Rendering /main.js resource");

	  // Chunked response to optimize RAM usage
      size_t content_length = main_js_gz_len;

      INFO("webserver() - Size of response is %d", content_length);
      PsychicStreamResponse response(resp, "text/javascript");

      response.addHeader("Cache-Control", "no-cache, must-revalidate");
	  response.addHeader("Content-Encoding", "gzip");
      response.setContentLength(content_length);

      response.beginSend();
	  response.write(main_js_gz, main_js_gz_len);
      return response.endSend(); });

	server_->on("/style.css", HTTP_GET, [this](PsychicRequest *request, PsychicResponse *resp) {

      INFO("webserver() - Rendering /style.css resource");

	  // Chunked response to optimize RAM usage
      size_t content_length = style_css_gz_len;

      INFO("webserver() - Size of response is %d", content_length);
      PsychicStreamResponse response(resp, "text/css");

      response.addHeader("Cache-Control", "no-cache, must-revalidate");
	  response.addHeader("Content-Encoding", "gzip");
      response.setContentLength(content_length);

      response.beginSend();
	  response.write(style_css_gz, style_css_gz_len);
      return response.endSend(); });

	server_->on("/description.xml", HTTP_GET, [this](PsychicRequest *request, PsychicResponse *resp) {
                    INFO("webserver() - /description.xml received");

                    String data = this->getSSDPDescription();

                    PsychicStreamResponse response(resp, "application/xml");
                    response.addHeader("Cache-Control", "no-cache, must-revalidate");
                    response.setContentLength(data.length());

                    response.beginSend();
                    response.print(data);
                    return response.endSend(); });

	server_->on("/project/*", HTTP_GET, [this](PsychicRequest *request, PsychicResponse *resp) {

					String uri = request->uri().substring(9);

					if (uri.endsWith("/model.xml")) {

						int p = uri.indexOf('/');
						String projectId = uri.substring(0, p);

						String path = "/project/" + projectId + "/model.xml";

						File content = fs_->open(path, FILE_READ);
						
						PsychicStreamResponse response(resp, "application/octet-stream");
						INFO("webserver() - content %s requested for download, file size is %d", path.c_str(), content.size());
						response.setCode(200);
						response.addHeader("Cache-Control","no-cache, must-revalidate");      
						response.beginSend();

						INFO("webserver() - start sending data to client");
						char buffer[512];
						size_t read = content.readBytes(&buffer[0], sizeof(buffer));
						while (read > 0) {
							INFO("Sending chunk of %d bytes", read);
							response.write((uint8_t *)&buffer[0], read);
							read = content.readBytes(&buffer[0], sizeof(buffer));
						}
						INFO("webserver() - transfer finished");

						content.close();                    

						return response.endSend(); 						

					} else {

						INFO("webserver() - Index page %s requested", uri.c_str());

						// Chunked response to optimize RAM usage
						size_t content_length = index_html_gz_len;

						INFO("webserver() - Size of response is %d", content_length);
						PsychicStreamResponse response(resp, "text/html");

						response.addHeader("Cache-Control", "no-cache, must-revalidate");
						response.setContentLength(content_length);
    					response.addHeader("Content-Encoding", "gzip");
						response.beginSend();
						response.write(index_html_gz, index_html_gz_len);
						return response.endSend(); 
					} });

	PsychicUploadHandler *projectPutHandler = new PsychicUploadHandler();
	projectPutHandler->onUpload([this](PsychicRequest *request, const String &filename, uint64_t position, uint8_t *data, size_t length, bool final) {

		INFO("webserver() - got data chunk with position %llu and length %u", position, length);

		String uri = request->uri().substring(9);
		int p = uri.indexOf('/');
		String projectId = uri.substring(0, p);
		String fname = uri.substring(p + 1);

		fs_->mkdir("/project");
		fs_->mkdir("/project/" + projectId);

		String file = "/project/" + projectId + "/" + fname;
		File content;

		if (position == 0) {
			INFO("webserver() - creating file %s", file.c_str());
			content = fs_->open(file, FILE_WRITE, true);
		} else {
			INFO("webserver() - opening file %s for append", file.c_str());        
			content = fs_->open(file, FILE_APPEND);
		}

		if (!content) {
			WARN("webserver() - failed to access file");
			return ESP_FAIL;
		} 

		if (!content.write(data, length)) {
			WARN("webserver() - Failed wo write data");
			return ESP_FAIL;
		}

		content.close();

		return ESP_OK; });

	projectPutHandler->onRequest([this](PsychicRequest *request, PsychicResponse *resp) {
		PsychicStreamResponse response(resp, "application/xml");                    

		String uri = request->uri().substring(9);
		int p = uri.indexOf('/');
		String projectId = uri.substring(0, p);
		String fname = uri.substring(p + 1);

		INFO("webserver() - /project/%s/%s PUT received", projectId.c_str(), fname.c_str());

		fs_->mkdir("/project");
		fs_->mkdir("/project/" + projectId);

		response.setCode(201);
		response.addHeader("Cache-Control","no-cache, must-revalidate");      
		response.setContentLength(0);

		return response.send(); });
	server_->on("/project/*", HTTP_PUT, projectPutHandler);

	PsychicUploadHandler *projectSyntaxCheckHandler = new PsychicUploadHandler();
	projectSyntaxCheckHandler->onUpload([this](PsychicRequest *request, const String &filename, uint64_t position, uint8_t *data, size_t length, bool final) {

		INFO("webserver() - got data chunk with position %llu and length %u", position, length);

		String file = "/~syntaxcheck.lua";
		File content;

		if (position == 0) {
			INFO("webserver() - creating file %s", file.c_str());
			content = fs_->open(file, FILE_WRITE, true);
		} else {
			INFO("webserver() - opening file %s for append", file.c_str());        
			content = fs_->open(file, FILE_APPEND);
		}

		if (!content) {
			WARN("webserver() - failed to access file");
			return ESP_FAIL;
		} 

		if (!content.write(data, length)) {
			WARN("webserver() - Failed wo write data");
			return ESP_FAIL;
		}

		content.close();

		return ESP_OK; });

	projectSyntaxCheckHandler->onRequest([this](PsychicRequest *request, PsychicResponse *resp) {
		INFO("webserver() - /syntaxcheck PUT received");

		PsychicStreamResponse response(resp, "application/json");

		JsonDocument root;

		String file = "/~syntaxcheck.lua";
		File content = fs_->open(file, FILE_READ);
		if (content) {
			String code = content.readString();
			content.close();

			LuaCheckResult result = hub_->checkLUACode(code);
			root["success"] = result.success;
			root["parseTime"] = result.parseTime;
			root["errorMessage"] = String(result.errorMessage.c_str());

		} else {
			WARN("webserver() - failed to access file");
			root["success"] = false;
			root["errorMessage"] = "Could not access uploaded code file";
		}

		String strContent;
		serializeJson(root, strContent);
		
		response.setCode(200);
		response.setContentType("application/json");
		response.setContentLength(strContent.length());
		response.addHeader("Cache-Control", "no-cache, must-revalidate");
		response.beginSend();
		response.print(strContent);
		return response.endSend(); });
	server_->on("/syntaxcheck", HTTP_PUT, projectSyntaxCheckHandler);

	PsychicUploadHandler *executeHandler = new PsychicUploadHandler();
	executeHandler->onUpload([this](PsychicRequest *request, const String &filename, uint64_t position, uint8_t *data, size_t length, bool final) {

		INFO("webserver() - got data chunk with position %llu and length %u", position, length);

		String file = "/~execute.lua";
		File content;

		if (position == 0) {
			INFO("webserver() - creating file %s", file.c_str());
			content = fs_->open(file, FILE_WRITE, true);
		} else {
			INFO("webserver() - opening file %s for append", file.c_str());        
			content = fs_->open(file, FILE_APPEND);
		}

		if (!content) {
			WARN("webserver() - failed to access file");
			return ESP_FAIL;
		} 

		if (!content.write(data, length)) {
			WARN("webserver() - Failed wo write data");
			return ESP_FAIL;
		}

		content.close();

		return ESP_OK; });

	executeHandler->onRequest([this](PsychicRequest *request, PsychicResponse *resp) {
		INFO("webserver() - /execute PUT received");

		PsychicStreamResponse response(resp, "application/json");

		JsonDocument root;

		String file = "/~execute.lua";
		File content = fs_->open(file, FILE_READ);
		if (content) {
			String code = content.readString();
			content.close();

			hub_->executeLUACode(code);

			root["success"] = true;			

		} else {
			WARN("webserver() - failed to access file");
			root["success"] = false;
		}

		String strContent;
		serializeJson(root, strContent);
		
		response.setCode(200);
		response.setContentType("application/json");
		response.setContentLength(strContent.length());
		response.addHeader("Cache-Control", "no-cache, must-revalidate");
		response.beginSend();
		response.print(strContent);
		return response.endSend(); });
	server_->on("/execute", HTTP_PUT, executeHandler);

	eventSource_.onOpen([](PsychicEventSourceClient *client) {
		INFO("[eventsource] connection #%u connected from %s\n", client->socket(), client->remoteIP().toString().c_str());
	});
	eventSource_.onClose([](PsychicEventSourceClient *client) {
		INFO("[eventsource] connection #%u closed\n", client->socket());
	});
	server_->on("/events", &eventSource_);

	// Logging forwarder task
	xTaskCreate(
		logForwarderTask,
		"LogForwarderTask",
		4096,
		(void *) this,
		1,
		&logforwarderTaskHandle_ // Store task handle for cancellation
	);

	started_ = true;
}

void HubWebServer::publishLogMessages() {
	String logMessage = loggingOutput_->waitForLogMessage(pdMS_TO_TICKS(5));
	while (logMessage.length() > 0) {
		if (eventSource_.count() > 0) {
			JsonDocument root;
			root["message"] = logMessage;

			String strContent;
			serializeJson(root, strContent);

			eventSource_.send(strContent.c_str(), "log", millis());
		}

		logMessage = loggingOutput_->waitForLogMessage(pdMS_TO_TICKS(5));
	}
}

void HubWebServer::publishCommands() {
	String command = Commands::instance()->waitForCommand(pdMS_TO_TICKS(5));
	while (command.length() > 0) {
		if (eventSource_.count() > 0) {
			eventSource_.send(command.c_str(), "command", millis());
		}

		command = Commands::instance()->waitForCommand(pdMS_TO_TICKS(5));
	}
}

bool HubWebServer::isStarted() {
	return started_;
}

void HubWebServer::ssdpNotify() {
	INFO("Sent SSDP NOTIFY messages");

	const IPAddress SSDP_MULTICAST_ADDR(239, 255, 255, 250);
	const uint16_t SSDP_PORT = 1900;

	// Send initial NOTIFY messages
	char notify[1024];

	String deviceUid = hub_->deviceUid();

	// Send NOTIFY for different device types
	String deviceTypes[] = {
		"upnp:rootdevice",
		"urn:schemas-upnp-org:device:Basic:1",
		String("uuid:") + deviceUid.c_str()};

	const char *SSDP_NOTIFY_TEMPLATE = "NOTIFY * HTTP/1.1\r\n"
									   "HOST: 239.255.255.250:1900\r\n"
									   "CACHE-CONTROL: max-age=1800\r\n"
									   "LOCATION: http://%s:%d/description.xml\r\n"
									   "SERVER: SSDPServer/1.0\r\n"
									   "NT: %s\r\n"
									   "USN: uuid:%s::%s\r\n"
									   "NTS: ssdp:alive\r\n"
									   "BOOTID.UPNP.ORG: 1\r\n"
									   "CONFIGID.UPNP.ORG: 1\r\n"
									   "\r\n";

	for (int i = 0; i < 3; i++) {
		sprintf(notify, SSDP_NOTIFY_TEMPLATE,
			WiFi.localIP().toString().c_str(),
			wsport_,
			deviceTypes[i].c_str(),
			deviceUid.c_str(),
			deviceTypes[i].c_str());

		udp_->beginPacket(SSDP_MULTICAST_ADDR, SSDP_PORT);
		udp_->write((uint8_t *) notify, strlen(notify));
		udp_->endPacket();
	}
}

void HubWebServer::loop() {
	long now = millis();
	if (now - lastSSDPNotify_ > 5000) {
		ssdpNotify();
		lastSSDPNotify_ = now;
	}

	// SSDP

	// Incoming messages
	int packetSize = udp_->parsePacket();
	if (packetSize) {
		char packetBuffer[512];
		int len = udp_->read(packetBuffer, sizeof(packetBuffer) - 1);
		if (len > 0) {
			packetBuffer[len] = '\0';

			// Check if it's an M-SEARCH request
			if (strstr(packetBuffer, "M-SEARCH") && strstr(packetBuffer, "ssdp:discover")) {
				INFO("Received SSDP M-SEARCH request");

				// Extract search target
				char *stLine = strstr(packetBuffer, "ST:");
				String searchTarget = "upnp:rootdevice"; // Default

				if (stLine) {
					stLine += 3; // Skip "ST:"
					while (*stLine == ' ') {
						stLine++; // Skip spaces
					}
					char *end = strstr(stLine, "\r");
					if (end) {
						*end = '\0';
						searchTarget = String(stLine);
					}
				}

				char response[1024];
				char dateStr[64];

				// Simple date string (could be improved with real time)
				sprintf(dateStr, "Mon, 01 Jan 1970 00:00:00 GMT");

				const char *SSDP_RESPONSE_TEMPLATE = "HTTP/1.1 200 OK\r\n"
													 "CACHE-CONTROL: max-age=1800\r\n"
													 "DATE: %s\r\n"
													 "EXT:\r\n"
													 "LOCATION: http://%s:%d/description.xml\r\n"
													 "SERVER: SSDPServer/1.0\r\n"
													 "ST: %s\r\n"
													 "USN: uuid:%s::%s\r\n"
													 "BOOTID.UPNP.ORG: 1\r\n"
													 "CONFIGID.UPNP.ORG: 1\r\n"
													 "\r\n";
				String deviceUid = hub_->deviceUid();

				sprintf(response, SSDP_RESPONSE_TEMPLATE,
					dateStr,
					WiFi.localIP().toString().c_str(),
					wsport_,
					searchTarget.c_str(),
					deviceUid.c_str(),
					searchTarget.c_str());

				// Send unicast response to requester
				udp_->beginPacket(udp_->remoteIP(), udp_->remotePort());
				udp_->write((uint8_t *) response, strlen(response));
				udp_->endPacket();

				INFO("Sent SSDP response");
			}
		}
	}
	// Server runs async...
}
