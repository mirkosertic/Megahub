#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "megahub.h"

#include <FS.h>
#include <memory>
#include <vector>

typedef std::function<void(char *buffer, int size)> Configuration_StreamTarget;

class Configuration {
private:
	std::unique_ptr<Megahub> hub_;
	FS *fs_;

	bool wifiEnabled_;
	bool btEnabled_;
	String ssid_;
	String pwd_;

	bool deleteDirectory(String directory);

public:
	Configuration(FS *fs, Megahub *hub);
	virtual ~Configuration();

	bool isWiFiEnabled();
	bool isBTEnabled();

	void connectToWiFiOrShowConfigPortal();

	void enterWiFiConfiguration();
	void load();

	String getAutostartProject();
	bool setAutostartProject(String projectId);

	void deleteProject(String projectId);
	std::vector<String> getProjects();
	bool writeFileChunkToProject(String projectId, String fileName, uint64_t position, uint8_t *data, size_t length);
	void streamProjectFileTo(String projectId, String fileName, const Configuration_StreamTarget &streamTarget);

	String getProjectFileContentAsString(String projectId, String fileName);
	bool writeProjectFileContent(String project, String filename, String &content);
};

#endif // CONFIGURATION_H
