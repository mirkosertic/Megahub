#ifndef IMU_H
#define IMU_H

#include "MPU6050_6Axis_MotionApps20.h"

class IMU {
private:
	long lastchecktime_;
	MPU6050 mpu_;
	uint16_t packetSize_;
	uint8_t fifoOBuffer_[64];
	Quaternion q_; // [w, x, y, z]         Quaternion container
	VectorInt16 aa_; // [x, y, z]            Accel sensor measurements
	VectorInt16 gg_; // [x, y, z]            Gyro sensor measurements
	VectorInt16 aaWorld_; // [x, y, z]            World-frame accel sensor measurements
	VectorInt16 ggWorld_; // [x, y, z]            World-frame gyro sensor measurements
	VectorFloat gravity_; // [x, y, z]            Gravity vector
	// float euler_[3]; // [psi, theta, phi]    Euler angle container
	float ypr_[3]; // [yaw, pitch, roll]   Yaw/Pitch/Roll container and gravity vector
public:
	IMU();
	virtual ~IMU();

	float getAccelerationX();
	float getAccelerationY();
	float getAccelerationZ();

	float getYaw();
	float getPitch();
	float getRoll();

	void loop();
};

#endif
