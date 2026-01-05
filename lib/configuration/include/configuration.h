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

	bool deleteDirectory(String directory);

public:
	Configuration(FS *fs, Megahub *hub);
	virtual ~Configuration();

	void enterWiFiConfiguration();
	void loadAndApply();

	String getAutostartProject();
	bool setAutostartProject(String projectId);

	void deleteProject(String projectId);
	std::vector<String> getProjects();
	bool writeFileChunkToProject(String projectId, String fileName, uint64_t position, uint8_t *data, size_t length);
	void streamProjectFileTo(String projectId, String fileName, const Configuration_StreamTarget &streamTarget);
};

#endif // CONFIGURATION_H
