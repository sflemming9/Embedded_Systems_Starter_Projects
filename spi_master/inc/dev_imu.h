#ifndef _DEV_IMU_H
#define _DEV_IMU_H

#include "spi.h"

typedef struct{
    Spi* spi;
    //bool initialized;
    //float offsets[3];
    //float scaleFactors[3];
    //int16_t rawData[3];
    //float data[3];
} LSM6DS3;

void lsm6ds3_Init(LSM6DS3 *imu);
void lsm6ds3_TurnOn(LSM6DS3 *imu);
bool lsm6ds3_WhoAmI(LSM6DS3 *imu);

int16_t lsm6ds3_read_x_accel(LSM6DS3 *imu);
int16_t lsm6ds3_read_y_accel(LSM6DS3 *imu);
int16_t lsm6ds3_read_z_accel(LSM6DS3 *imu);
int16_t lsm6ds3_read_pitch_gyro(LSM6DS3 *imu);
int16_t lsm6ds3_read_roll_gyro(LSM6DS3 *imu);
int16_t lsm6ds3_read_yaw_gyro(LSM6DS3 *imu);
int16_t lsm6ds3_read_temp(LSM6DS3 *imu);

#endif