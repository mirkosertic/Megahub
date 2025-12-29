#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "megahub.h"

#include <FS.h>
#include <memory>

class Configuration {
private:
	std::unique_ptr<Megahub> hub_;
	FS *fs_;
public:
	Configuration(FS *fs, Megahub *hub);
	virtual ~Configuration();

	void enterWiFiConfiguration();
	void loadAndApply();
};

#endif // CONFIGURATION_H
