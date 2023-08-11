#include "FastIMU.h"
#include <Wire.h>

#define IMU_ADDRESS 0x69    //Change to the address of the IMU
#define PERFORM_CALIBRATION //Comment out this line to skip calibration at start
MPU6500 IMU;               //Change to the name of any supported IMU! 
// Other supported IMUS: MPU9255 MPU9250 MPU6886 MPU6050 ICM20689 ICM20690 BMI055 BMX055 BMI160 LSM6DS3 LSM6DSL

const int I2C_SDA = 18;   //I2C Data pin
const int I2C_SCL = 21;   //I2c Clock pin

calData calib = { 0 };  //Calibration data
AccelData accelData;    //Sensor data
GyroData gyroData;

void setup() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000); //400khz clock
  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  int err = IMU.init(calib, IMU_ADDRESS);
  if (err != 0) {
    Serial.print("Error initializing IMU: ");
    Serial.println(err);
    while (true) {
      ;
    }
  }


#ifdef PERFORM_CALIBRATION
  Serial.println("FastIMU calibration & data example");
  delay(2500);
  Serial.println("Keep IMU level.");
  delay(5000);
  IMU.calibrateAccelGyro(&calib);
  Serial.println("Calibration done!");
  Serial.println("Accel biases X/Y/Z: ");
  Serial.print(calib.accelBias[0]);
  Serial.print(", ");
  Serial.print(calib.accelBias[1]);
  Serial.print(", ");
  Serial.println(calib.accelBias[2]);
  Serial.println("Gyro biases X/Y/Z: ");
  Serial.print(calib.gyroBias[0]);
  Serial.print(", ");
  Serial.print(calib.gyroBias[1]);
  Serial.print(", ");
  Serial.println(calib.gyroBias[2]);
  delay(5000);
  IMU.init(calib, IMU_ADDRESS);
#endif

  //err = IMU.setGyroRange(500);      //USE THESE TO SET THE RANGE, IF AN INVALID RANGE IS SET IT WILL RETURN -1
  //err = IMU.setAccelRange(2);       //THESE TWO SET THE GYRO RANGE TO ±500 DPS AND THE ACCELEROMETER RANGE TO ±2g
  
  if (err != 0) {
    Serial.print("Error Setting range: ");
    Serial.println(err);
    while (true) {
      ;
    }
  }
}

void loop() {
  IMU.update();
  IMU.getAccel(&accelData);
  Serial.print("Accel - x:");
  Serial.print(accelData.accelX);
  Serial.print("\t");
  Serial.print("y:");
  Serial.print(accelData.accelY);
  Serial.print("\t");
  Serial.print("z:");
  Serial.print(accelData.accelZ);
  Serial.print("\t");
  IMU.getGyro(&gyroData);
  Serial.print("Gyro - x:");
  Serial.print(gyroData.gyroX);
  Serial.print("\t");
  Serial.print("y:");
  Serial.print(gyroData.gyroY);
  Serial.print("\t");
  Serial.print("z:");
  Serial.print(gyroData.gyroZ);
  if (IMU.hasTemperature()) {
	  Serial.print("\t");
    Serial.print("Temp:");
	  Serial.println(IMU.getTemp());
  }
  delay(150);
}
