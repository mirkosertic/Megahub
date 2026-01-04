#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "megahub.h"

#include <FS.h>
#include <memory>
#include <vector>

class Configuration {
private:
	std::unique_ptr<Megahub> hub_;
	FS *fs_;

public:
	Configuration(FS *fs, Megahub *hub);
	virtual ~Configuration();

	void enterWiFiConfiguration();
	void loadAndApply();

	String getAutostartProject();
	bool setAutostartProject(String projectName);

	std::vector<String> getProjects();
	bool writeFileChunkToProject(String projectId, String fileName, uint64_t position, uint8_t *data, size_t length);
};

#endif // CONFIGURATION_H
