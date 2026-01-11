#ifndef BTCONTROLLER_H
#define BTCONTROLLER_H

#include <Arduino.h>

// #include <BLEGamepadClient.h>

class BluetoothController {
private:
	//	XboxController *controller_;

public:
	BluetoothController();
	virtual ~BluetoothController();

	void init();
	void loop();
};

#endif // BTCONTROLLER_H
