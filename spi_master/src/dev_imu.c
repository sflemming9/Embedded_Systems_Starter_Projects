/*
* Driver for the lsm6ds3 iNEMO inertial module
* 
* Development tips: 
* Time (~10m) has been known to fix problems
* where previously working code stops working
* probably some interesting electrical factors at play
* 
* TODO: Calibration constants
*/
#include <dev_imu.h>
#include <spi.h>

/*SPI PINS--------------------------------------------------------------------*/
Spi imu_spi = {
    .spiPeriph = SPI1,
    .clock = RCC_APB2Periph_SPI1,
    .rccfunc = &(RCC_APB2PeriphClockCmd),
    .af = GPIO_AF_SPI1,
    .cs = {
        .port = GPIOA,
        .rccfunc = &(RCC_AHB1PeriphClockCmd),
        .pin = GPIO_Pin_4,
        .pinsource = GPIO_PinSource4,
        .clock = RCC_AHB1Periph_GPIOA,
        .af = GPIO_AF_SPI1
    },
    .sclk = {
        .port = GPIOA,
        .rccfunc = &(RCC_AHB1PeriphClockCmd),
        .pin = GPIO_Pin_5,
        .pinsource = GPIO_PinSource5,
        .clock = RCC_AHB1Periph_GPIOA,
        .af = GPIO_AF_SPI1
    },
    .miso = { /*SDO*/
        .port = GPIOA,
        .rccfunc = &(RCC_AHB1PeriphClockCmd),
        .pin = GPIO_Pin_6,
        .pinsource = GPIO_PinSource6,
        .clock = RCC_AHB1Periph_GPIOA,
        .af = GPIO_AF_SPI1
    },
    .mosi = { /*SDA*/
        .port = GPIOA,
        .rccfunc = &(RCC_AHB1PeriphClockCmd),
        .pin = GPIO_Pin_7,
        .pinsource = GPIO_PinSource7,
        .clock = RCC_AHB1Periph_GPIOA,
        .af = GPIO_AF_SPI1
    },
    .maxClockSpeed = 10000000,
    .cpol = SPI_CPOL_High,
    .cpha = SPI_CPHA_2Edge,
    .word16 = false,
    .initialized = false
};

/*IMU constants---------------------------------------------------------------*/
#define LSM6DS3_READ_MASK                       0X80
#define LSM6DS3_IDENTIFIER                       0x69

/************** Device Register  *******************/
#define LSM6DS3_TEST_PAGE  			0X00
#define LSM6DS3_RAM_ACCESS  			0X01
#define LSM6DS3_SENSOR_SYNC_TIME  		0X04
#define LSM6DS3_SENSOR_SYNC_EN  		0X05
#define LSM6DS3_FIFO_CTRL1  			0X06
#define LSM6DS3_FIFO_CTRL2  			0X07
#define LSM6DS3_FIFO_CTRL3  			0X08
#define LSM6DS3_FIFO_CTRL4  			0X09
#define LSM6DS3_FIFO_CTRL5  			0X0A
#define LSM6DS3_ORIENT_CFG_G  			0X0B
#define LSM6DS3_REFERENCE_G  			0X0C
#define LSM6DS3_INT1_CTRL  			0X0D
#define LSM6DS3_INT2_CTRL  			0X0E
#define LSM6DS3_WHO_AM_I_REG  			0X0F
#define LSM6DS3_CTRL1_XL  			0X10
#define LSM6DS3_CTRL2_G  			0X11
#define LSM6DS3_CTRL3_C  			0X12
#define LSM6DS3_CTRL4_C  			0X13
#define LSM6DS3_CTRL5_C  			0X14
#define LSM6DS3_CTRL6_G  			0X15
#define LSM6DS3_CTRL7_G  			0X16
#define LSM6DS3_CTRL8_XL  			0X17
#define LSM6DS3_CTRL9_XL  			0X18
#define LSM6DS3_CTRL10_C  			0X19
#define LSM6DS3_MASTER_CONFIG  		0X1A
#define LSM6DS3_WAKE_UP_SRC  			0X1B
#define LSM6DS3_TAP_SRC  			0X1C
#define LSM6DS3_D6D_SRC  			0X1D
#define LSM6DS3_STATUS_REG  			0X1E
#define LSM6DS3_OUT_TEMP_L  			0X20
#define LSM6DS3_OUT_TEMP_H  			0X21
#define LSM6DS3_OUTX_L_G  			0X22
#define LSM6DS3_OUTX_H_G  			0X23
#define LSM6DS3_OUTY_L_G  			0X24
#define LSM6DS3_OUTY_H_G  			0X25
#define LSM6DS3_OUTZ_L_G  			0X26
#define LSM6DS3_OUTZ_H_G  			0X27
#define LSM6DS3_OUTX_L_XL  			0X28
#define LSM6DS3_OUTX_H_XL  			0X29
#define LSM6DS3_OUTY_L_XL  			0X2A
#define LSM6DS3_OUTY_H_XL  			0X2B
#define LSM6DS3_OUTZ_L_XL  			0X2C
#define LSM6DS3_OUTZ_H_XL  			0X2D
#define LSM6DS3_SENSORHUB1_REG  		0X2E
#define LSM6DS3_SENSORHUB2_REG  		0X2F
#define LSM6DS3_SENSORHUB3_REG  		0X30
#define LSM6DS3_SENSORHUB4_REG  		0X31
#define LSM6DS3_SENSORHUB5_REG  		0X32
#define LSM6DS3_SENSORHUB6_REG  		0X33
#define LSM6DS3_SENSORHUB7_REG  		0X34
#define LSM6DS3_SENSORHUB8_REG  		0X35
#define LSM6DS3_SENSORHUB9_REG  		0X36
#define LSM6DS3_SENSORHUB10_REG  		0X37
#define LSM6DS3_SENSORHUB11_REG  		0X38
#define LSM6DS3_SENSORHUB12_REG  		0X39
#define LSM6DS3_FIFO_STATUS1  			0X3A
#define LSM6DS3_FIFO_STATUS2  			0X3B
#define LSM6DS3_FIFO_STATUS3  			0X3C
#define LSM6DS3_FIFO_STATUS4  			0X3D
#define LSM6DS3_FIFO_DATA_OUT_L  		0X3E
#define LSM6DS3_FIFO_DATA_OUT_H  		0X3F
#define LSM6DS3_TIMESTAMP0_REG  		0X40
#define LSM6DS3_TIMESTAMP1_REG  		0X41
#define LSM6DS3_TIMESTAMP2_REG  		0X42
#define LSM6DS3_STEP_COUNTER_L  		0X4B
#define LSM6DS3_STEP_COUNTER_H  		0X4C
#define LSM6DS3_FUNC_SRC  			0X53
#define LSM6DS3_TAP_CFG1  			0X58
#define LSM6DS3_TAP_THS_6D  			0X59
#define LSM6DS3_INT_DUR2  			0X5A
#define LSM6DS3_WAKE_UP_THS  			0X5B
#define LSM6DS3_WAKE_UP_DUR  			0X5C
#define LSM6DS3_FREE_FALL  			0X5D
#define LSM6DS3_MD1_CFG  			0X5E
#define LSM6DS3_MD2_CFG  			0X5F

/*******************************************************************************
* Register      : CTRL1_XL
* Address       : 0X10
* Bit Group Name: FS_XL
* Permission    : RW
*******************************************************************************/

enum LSM6DS3_FS_XL{
	LSM6DS3_2g 		        = 0x00,
	LSM6DS3_16g 		        = 0x04,
	LSM6DS3_4g 		        = 0x08,
	LSM6DS3_8g 		        = 0x0C,
} lsm6ds3_fs_xl;

/*******************************************************************************
* Register      : CTRL1_XL
* Address       : 0X10
* Bit Group Name: ODR_XL
* Permission    : RW
*******************************************************************************/

enum LSM6DS3_ACCEL_OUTPUT_DATA_RATE{
	LSM6DS3_ACCEL_LSM6DS3_OUTPUT_DATA_RATE_POWER_DOWN              = 0x00,
	LSM6DS3_ACCEL_13Hz 		        = 0x10, /*12.5hz*/
	LSM6DS3_ACCEL_26Hz 		        = 0x20,
	LSM6DS3_ACCEL_52Hz 		        = 0x30,
	LSM6DS3_ACCEL_104Hz 		        = 0x40,
	LSM6DS3_ACCEL_208Hz 		        = 0x50,
	LSM6DS3_ACCEL_416Hz 		        = 0x60,
	LSM6DS3_ACCEL_833Hz 		        = 0x70,
	LSM6DS3_ACCEL_1660Hz 		        = 0x80,
	LSM6DS3_ACCEL_3330Hz 		        = 0x90,
	LSM6DS3_ACCEL_6660Hz 		        = 0xA0,
	LSM6DS3_ACCEL_13330Hz 		= 0xB0,
} lsm6ds3_accel_output_data_rate;

/*******************************************************************************
* Register      : CTRL2_G
* Address       : 0X11
* Bit Group Name: ODR_G
* Permission    : RW
*******************************************************************************/

enum LSM6DS3_GYRO_OUTPUT_DATA_RATE{
	LSM6DS3_GYRO_POWER_DOWN 		= 0x00,
	LSM6DS3_GYRO_13Hz 		        = 0x10,
	LSM6DS3_GYRO_26Hz 		        = 0x20,
	LSM6DS3_GYRO_52Hz 		        = 0x30,
	LSM6DS3_GYRO_104Hz 		        = 0x40,
	LSM6DS3_GYRO_208Hz 		        = 0x50,
	LSM6DS3_GYRO_416Hz 		        = 0x60,
	LSM6DS3_GYRO_833Hz 		        = 0x70,
	LSM6DS3_GYRO_1660Hz 		        = 0x80,
} lsm6ds3_gyro_output_data_rate;



/*******************************************************************************
* Register      : CTRL9_XL
* Address       : 0X18
* Bit Group Name: XEN_XL
* Permission    : RW
*******************************************************************************/
enum LSM6DS3_XAXIS{
	LSM6DS3_XAXIS_DISABLED 		                = 0x00,
	LSM6DS3_XAXIS_ENABLED 		                =  0x08,
} lsm6ds3_xaxis;


/*******************************************************************************
* Register      : CTRL9_XL
* Address       : 0X18
* Bit Group Name: YEN_XL
* Permission    : RW
*******************************************************************************/
enum LSM6DS3_YAXIS{
	LSM6DS3_YAXIS_DISABLED 		                = 0x00,
	LSM6DS3_YAXIS_ENABLED 		                = 0x10,
} lsm6ds3_yaxis;

/*******************************************************************************
* Register      : CTRL9_XL
* Address       : 0X18
* Bit Group Name: ZEN_XL
* Permission    : RW
*******************************************************************************/
enum LSM6DS3_ZAXIS{
	LSM6DS3_ZAXIS_DISABLED 		                = 0x00,
	LSM6DS3_ZAXIS_ENABLED 		                = 0x20,
} lsm6ds3_zaxis;

/*READ FUNCTIONS--------------------------------------------------------------*/

/*calibration constants*/
//TODO: write these into the chip itself in case we switch boards
const float TEMP_OFFSET = 256;

/*convertoin constants*/
const float ACCEL_CONVERT = .61;
const float TEMP_CONVERT = 16;

/*this imu needs a mask*/
uint8_t lsm6ds3_read8(Spi *spi, uint8_t reg){
  return read8(spi, reg | LSM6DS3_READ_MASK);
}

void lsm6ds3_Init(LSM6DS3 *imu){
    if(imu->spi->initialized == false)
        SPI_Config(imu->spi);
    SPI_SetCS(imu->spi);
}

void lsm6ds3_TurnOn(LSM6DS3 *imu){
    lsm6ds3_xaxis = LSM6DS3_XAXIS_ENABLED;
    lsm6ds3_yaxis = LSM6DS3_YAXIS_ENABLED;
    lsm6ds3_zaxis = LSM6DS3_ZAXIS_ENABLED;
    /*accelerometer on*/
    write8(imu->spi, LSM6DS3_CTRL9_XL, lsm6ds3_xaxis | 
                                       lsm6ds3_yaxis |
                                       lsm6ds3_zaxis);
    /*gyroscope on*/
    write8(imu->spi, LSM6DS3_CTRL10_C, lsm6ds3_xaxis | 
                                        lsm6ds3_yaxis |
                                        lsm6ds3_zaxis);

    /*initialize accel rate*/
    /*g forces range up to 2gs*/
    lsm6ds3_fs_xl = LSM6DS3_2g;
    lsm6ds3_accel_output_data_rate = LSM6DS3_ACCEL_13Hz;
    write8(imu->spi, LSM6DS3_CTRL1_XL, lsm6ds3_fs_xl | lsm6ds3_accel_output_data_rate);
    
    /*initialize gyro rate*/
    lsm6ds3_gyro_output_data_rate = LSM6DS3_GYRO_13Hz;
    write8(imu->spi, LSM6DS3_CTRL2_G, lsm6ds3_gyro_output_data_rate);
}

bool lsm6ds3_WhoAmI(LSM6DS3 *imu){
    uint8_t board_id = lsm6ds3_read8(imu->spi, LSM6DS3_WHO_AM_I_REG);
    return board_id == LSM6DS3_IDENTIFIER;
}

int16_t lsm6ds3_read_x_accel(LSM6DS3 *imu){
    int16_t x_accel = lsm6ds3_read8(imu->spi, LSM6DS3_OUTX_H_XL);
    x_accel = (x_accel << 8) | (lsm6ds3_read8(imu->spi, LSM6DS3_OUTX_L_XL));
    x_accel *= ACCEL_CONVERT;
    return x_accel;
}

int16_t lsm6ds3_read_y_accel(LSM6DS3 *imu){
    int16_t y_accel = lsm6ds3_read8(imu->spi, LSM6DS3_OUTY_H_XL);
    y_accel = (y_accel << 8) | (lsm6ds3_read8(imu->spi, LSM6DS3_OUTY_L_XL));
    y_accel *= ACCEL_CONVERT;
    return y_accel;
}

int16_t lsm6ds3_read_z_accel(LSM6DS3 *imu){
    int16_t z_accel = lsm6ds3_read8(imu->spi, LSM6DS3_OUTZ_H_XL);
    z_accel = (z_accel << 8) | (lsm6ds3_read8(imu->spi, LSM6DS3_OUTZ_L_XL));
    z_accel *= ACCEL_CONVERT;
    return z_accel;
}

int16_t lsm6ds3_read_pitch_gyro(LSM6DS3 *imu){
    int16_t x_gyro = lsm6ds3_read8(imu->spi, LSM6DS3_OUTX_H_G);
    x_gyro = (x_gyro << 8) | (lsm6ds3_read8(imu->spi, LSM6DS3_OUTX_L_G));
    /*TODO: processing*/
    return x_gyro;
}

int16_t lsm6ds3_read_roll_gyro(LSM6DS3 *imu){
    int16_t y_gyro = lsm6ds3_read8(imu->spi, LSM6DS3_OUTY_H_G);
    y_gyro = (y_gyro << 8) | (lsm6ds3_read8(imu->spi, LSM6DS3_OUTY_L_G));
    /*TODO: processing*/
    return y_gyro;
}

int16_t lsm6ds3_read_yaw_gyro(LSM6DS3 *imu){
    int16_t z_gyro = lsm6ds3_read8(imu->spi, LSM6DS3_OUTZ_H_G);
    z_gyro = (z_gyro << 8) | (lsm6ds3_read8(imu->spi, LSM6DS3_OUTZ_L_G));
    /*TODO: processing*/
    return z_gyro;
}

int16_t lsm6ds3_read_temp(LSM6DS3 *imu){
    int16_t temp = lsm6ds3_read8(imu->spi, LSM6DS3_OUT_TEMP_H);
    temp = (temp << 8) | (lsm6ds3_read8(imu->spi, LSM6DS3_OUT_TEMP_L));
    temp = (temp + 256) >> 4;
    return temp;
}