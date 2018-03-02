#include <l3gd20.h>
#include <spi.h>

// Message formatting bits
#define BIT_READ    0x80
#define BIT_ADDRINC 0x40

// Address bits
#define ADDR_WHO_AM_I           0x0F
#define ADDR_CTRL_REG_1         0x20
#define ADDR_CTRL_REG_2         0x21
#define ADDR_CTRL_REG_3         0x22
#define ADDR_CTRL_REG_4         0x23
#define ADDR_CTRL_REG_5         0x24
#define ADDR_REFERENCE          0x25
#define ADDR_OUT_TEMP           0x26
#define ADDR_STATUS_REG         0x27
#define ADDR_OUT_X_L            0x28
#define ADDR_OUT_X_H            0x29
#define ADDR_OUT_Y_L            0x2A
#define ADDR_OUT_Y_H            0x2B
#define ADDR_OUT_Z_L            0x2C
#define ADDR_OUT_Z_H            0x2D
#define ADDR_FIFO_CTRL_REG      0x2E
#define ADDR_FIFO_SRC_REG       0x2F
#define ADDR_INT1_CFG           0x30
#define ADDR_INT1_SRC           0x31
#define ADDR_INT1_TSH_XH        0x32
#define ADDR_INT1_TSH_XL        0x33
#define ADDR_INT1_TSH_YH        0x34
#define ADDR_INT1_TSH_YL        0x35
#define ADDR_INT1_TSH_ZH        0x36
#define ADDR_INT1_TSH_ZL        0x37
#define ADDR_INT1_DURATION      0x38

// Initialize the peripherals
void L3GD20_Init(L3GD20* gyro){
    // Bail if we've already done this
    if(gyro->initialized)
        return;

    // Initialize SPI
    if(gyro->spi->initialized == false)
        SPI_Config(gyro->spi);
    SPI_SetCS(gyro->spi);

    // Record that we've initialized
    gyro->initialized = true;
}

// Quick existence check to verify that a gyro can communicate
uint32_t L3GD20_WhoAmI(L3GD20* gyro){
    // Ask the gyro to identify itself
	SPI_ResetCS(gyro->spi);
	SPI_Transfer8(gyro->spi, (BIT_READ | ADDR_WHO_AM_I));
	uint32_t readVal = SPI_Transfer8(gyro->spi, 0);
	SPI_SetCS(gyro->spi);

	// Returns true if gyroscope identifies itself properly
    return (readVal == 0xD3);
}

// Enable the gyro
void L3GD20_TurnOn(L3GD20* gyro){
	SPI_ResetCS(gyro->spi);
	SPI_Transfer8(gyro->spi, ADDR_CTRL_REG_1);
	SPI_Transfer8(gyro->spi, 0xFF);
	SPI_SetCS(gyro->spi);
}

// Read new X, Y, Z angular velocities
void L3GD20_UpdateReadings(L3GD20* gyro){
    // Update x, y, and z
    SPI_ResetCS(gyro->spi);
    SPI_Transfer8(gyro->spi, (BIT_READ | BIT_ADDRINC | ADDR_OUT_X_L));
	SPI_ReadMulti(gyro->spi, 6, 2, &(gyro->rawData));
    SPI_SetCS(gyro->spi);
}

// Turn raw readings into floats scaled to natural units
void L3GD20_ComputeDps(L3GD20* gyro){
    for(int a = 0; a < 3; a++){
        gyro->data[a] = gyro->rawData[a] - gyro->offsets[a];
        gyro->data[a] *= gyro->scaleFactors[a];
    }
}