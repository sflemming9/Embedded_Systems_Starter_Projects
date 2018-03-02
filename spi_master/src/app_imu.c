#include "app_imu.h"
#include "dev_imu.h"
#include <spi.h>
#include <stdio.h>
#include <stddef.h> /*NULL*/
#include <FreeRTOS.h>
#include <task.h>

extern Spi imu_spi;
LSM6DS3 imu;

void init_SPI1();
uint8_t SPI1_send(uint8_t data);

void app_imu_imuTask(void* pvParameters){
  imu.spi = &imu_spi;
  lsm6ds3_Init(&imu);
  if(lsm6ds3_WhoAmI(&imu) == false){
    printf("imu responded incorrectly or did not respond\n");
    /*NULL value indicates we are deleting the current task*/
    vTaskDelete(NULL);
  }
  lsm6ds3_TurnOn(&imu);

  while(1){  
    int16_t x_accel = lsm6ds3_read_x_accel(&imu);
    int16_t y_accel = lsm6ds3_read_y_accel(&imu);
    int16_t z_accel = lsm6ds3_read_z_accel(&imu);
    printf("x_accel:%d y_accel:%d z_accel:%d\n", x_accel, y_accel, z_accel);
    
    int16_t pitch = lsm6ds3_read_pitch_gyro(&imu);
    int16_t roll = lsm6ds3_read_roll_gyro(&imu);
    int16_t yaw = lsm6ds3_read_yaw_gyro(&imu);
    printf("pitch:%d roll:%d yaw:%d\n", pitch, roll, yaw);
    
    int16_t temp = lsm6ds3_read_temp(&imu);
    printf("temp:%d\n", temp);
    
    vTaskDelay(1000);
  }
}