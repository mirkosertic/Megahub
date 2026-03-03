#include "imu.h"

#include "i2csync.h"
#include "imu_config.h"
#include "logging.h"

#include <I2Cdev.h>

#define EARTH_GRAVITY_MS2 9.80665 // m/s2
#define DEG_TO_RAD        0.017453292519943295769236907684886
#define RAD_TO_DEG        57.295779513082320876798154814105

IMU::IMU() {
	lastchecktime_ = -1;
	mpu_.initialize();
	if (!mpu_.testConnection()) {
		WARN("MPU6050 connection failed");
		while (true) {
			vTaskDelay(pdMS_TO_TICKS(50));
		}
	}
	INFO("MPU6050 connection successful");

	/* Initializate and configure the DMP*/
	INFO("Initializing DMP...");
	int devStatus = mpu_.dmpInitialize();

	/* Supply your gyro offsets here, scaled for min sensitivity */
	mpu_.setXGyroOffset(0);
	mpu_.setYGyroOffset(0);
	mpu_.setZGyroOffset(0);
	mpu_.setXAccelOffset(0);
	mpu_.setYAccelOffset(0);
	mpu_.setZAccelOffset(0);

	/* Making sure it worked (returns 0 if so) */
	if (devStatus == 0) {
		mpu_.CalibrateAccel(6); // Calibration Time: generate offsets and calibrate our MPU6050
		mpu_.CalibrateGyro(6);
		INFO("These are the Active offsets: ");
		mpu_.PrintActiveOffsets();
		INFO("Enabling DMP..."); // Turning ON DMP
		mpu_.setDMPEnabled(true);

		int MPUIntStatus = mpu_.getIntStatus();

		/* Set the DMP Ready flag so the main loop() function knows it is okay to use it */
		INFO("DMP ready! Waiting for first interrupt...");
		packetSize_ = mpu_.dmpGetFIFOPacketSize(); // Get expected DMP packet size for later comparison
	}
}

IMU::~IMU() {}

// Apply imuAxisMap to remap a 3-vector [x, y, z] from physical to logical frame.
// raw[0]=X, raw[1]=Y, raw[2]=Z; result[i] = sum_j(imuAxisMap[i][j] * raw[j])
std::array<float, 3> IMU::applyAxisMapping(const std::array<float, 3>& raw) {
	std::array<float, 3> result = {};
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			result[static_cast<size_t>(i)] +=
			    imuAxisMap[static_cast<size_t>(i)][static_cast<size_t>(j)] * raw[static_cast<size_t>(j)];
		}
	}
	return result;
}

float IMU::getAccelerationX() {
	i2c_lock();
	const auto scale = static_cast<float>(mpu_.get_acce_resolution() * EARTH_GRAVITY_MS2);
	const std::array<float, 3> raw = {static_cast<float>(aaWorld_.x) * scale, static_cast<float>(aaWorld_.y) * scale,
	                                  static_cast<float>(aaWorld_.z) * scale};
	const auto corrected = applyAxisMapping(raw);
	i2c_unlock();
	return corrected[0];
}

float IMU::getAccelerationY() {
	i2c_lock();
	const auto scale = static_cast<float>(mpu_.get_acce_resolution() * EARTH_GRAVITY_MS2);
	const std::array<float, 3> raw = {static_cast<float>(aaWorld_.x) * scale, static_cast<float>(aaWorld_.y) * scale,
	                                  static_cast<float>(aaWorld_.z) * scale};
	const auto corrected = applyAxisMapping(raw);
	i2c_unlock();
	return corrected[1];
}

float IMU::getAccelerationZ() {
	i2c_lock();
	const auto scale = static_cast<float>(mpu_.get_acce_resolution() * EARTH_GRAVITY_MS2);
	const std::array<float, 3> raw = {static_cast<float>(aaWorld_.x) * scale, static_cast<float>(aaWorld_.y) * scale,
	                                  static_cast<float>(aaWorld_.z) * scale};
	const auto corrected = applyAxisMapping(raw);
	i2c_unlock();
	return corrected[2];
}

float IMU::getYaw() {
	i2c_lock();
	// ypr_[0]=yaw (Z), ypr_[1]=pitch (Y), ypr_[2]=roll (X) → map to [X, Y, Z]
	const std::array<float, 3> raw = {ypr_[2], ypr_[1], ypr_[0]};
	const auto corrected = applyAxisMapping(raw);
	i2c_unlock();
	return static_cast<float>(corrected[2] * RAD_TO_DEG);
}

float IMU::getPitch() {
	i2c_lock();
	const std::array<float, 3> raw = {ypr_[2], ypr_[1], ypr_[0]};
	const auto corrected = applyAxisMapping(raw);
	i2c_unlock();
	return static_cast<float>(corrected[1] * RAD_TO_DEG);
}

float IMU::getRoll() {
	i2c_lock();
	const std::array<float, 3> raw = {ypr_[2], ypr_[1], ypr_[0]};
	const auto corrected = applyAxisMapping(raw);
	i2c_unlock();
	return static_cast<float>(corrected[0] * RAD_TO_DEG);
}

void IMU::loop() {
	if (lastchecktime_ == -1) {
		lastchecktime_ = millis();
		return;
	}
	long currenttime = millis();

	// We check every 100ms
	if (currenttime - lastchecktime_ > 100) {
		DEBUG("Processing INU data");
		lastchecktime_ = currenttime;

		/* Read a packet from FIFO */
		if (mpu_.dmpGetCurrentFIFOPacket(fifoOBuffer_)) { // Get the Latest packet
			/*Display quaternion values in easy matrix form: w x y z */
			mpu_.dmpGetQuaternion(&q_, fifoOBuffer_);
			/*Serial.print("quat\t");
			Serial.print(q.w);
			Serial.print("\t");
			Serial.print(q.x);
			Serial.print("\t");
			Serial.print(q.y);
			Serial.print("\t");
			Serial.println(q.z);*/

			mpu_.dmpGetGravity(&gravity_, &q_);

			/* Display initial world-frame acceleration, adjusted to remove gravity
			and rotated based on known orientation from Quaternion */
			mpu_.dmpGetAccel(&aa_, fifoOBuffer_);
			mpu_.dmpConvertToWorldFrame(&aaWorld_, &aa_, &q_);
			/*Serial.print("aworld\t");
			Serial.print(aaWorld.x * mpu.get_acce_resolution() * EARTH_GRAVITY_MS2);
			Serial.print("\t");
			Serial.print(aaWorld.y * mpu.get_acce_resolution() * EARTH_GRAVITY_MS2);
			Serial.print("\t");
			Serial.println(aaWorld.z * mpu.get_acce_resolution() * EARTH_GRAVITY_MS2);*/

			/* Display initial world-frame acceleration, adjusted to remove gravity
			and rotated based on known orientation from Quaternion */
			mpu_.dmpGetGyro(&gg_, fifoOBuffer_);
			mpu_.dmpConvertToWorldFrame(&ggWorld_, &gg_, &q_);
			/* Serial.print("ggWorld\t");
			Serial.print(ggWorld.x * mpu.get_gyro_resolution() * DEG_TO_RAD);
			Serial.print("\t");
			Serial.print(ggWorld.y * mpu.get_gyro_resolution() * DEG_TO_RAD);
			Serial.print("\t");
			Serial.println(ggWorld.z * mpu.get_gyro_resolution() * DEG_TO_RAD);*/

			/* Display Euler angles in degrees */
			mpu_.dmpGetYawPitchRoll(ypr_, &q_, &gravity_);
			/*Serial.print("ypr\t");
			Serial.print(ypr[0] * RAD_TO_DEG);
			Serial.print("\t");
			Serial.print(ypr[1] * RAD_TO_DEG);
			Serial.print("\t");
			Serial.println(ypr[2] * RAD_TO_DEG);*/
		}
	}
}
