#include <ms5803.h>
#include <math.h>
#include <csp.h>

// Enforce 100ns minimum delay between CS rising and falling again
static inline void MS5803_CsDelay(void) {
    uint64_t target = CSP_TotalClockCycles() +
        (uint64_t)(SystemCoreClock * 100e-9);
    while(CSP_TotalClockCycles() < target);
}

// Start MS5803 ADC conversion (takes <=9.04ms to complete)
void MS5803_StartConversion(Ms5803* sensor, uint32_t channel){
	// Select and convert a channel
    MS5803_CsDelay();
	SPI_ResetCS(sensor->spi);
    SPI_Transfer8(sensor->spi, channel);
    sensor->waitUntil = CSP_TotalClockCycles() +
        (uint64_t)(SystemCoreClock * 10e-3);
}

// Sample an ADC
static uint32_t MS5803_ReadADC(Ms5803* sensor){
    uint32_t result;

	// Wait for the conversion to complete
    while(CSP_TotalClockCycles() < sensor->waitUntil);
	SPI_SetCS(sensor->spi);

	// Instruct the sensor to read the converted value
    MS5803_CsDelay();
	SPI_ResetCS(sensor->spi);
    SPI_Transfer8(sensor->spi, MS5803_ADCREAD);

	// Read back the data
    SPI_ReadMulti_Big(sensor->spi, 1, 3, &result);
	result &= 0x00FFFFFF;
    SPI_SetCS(sensor->spi);

    return result;
}

// Initialize the peripherals
void MS5803_Init(Ms5803* sensor){
    // Bail if we've already done this
    if(sensor->initialized)
        return;

    // Initialize SPI
    if(sensor->spi->initialized == false){
        SPI_Config(sensor->spi);
    }
    SPI_SetCS(sensor->spi);

    // Record that we've initialized
    sensor->initialized = true;
}

// Reset the pressure sensor
void MS5803_Reset(Ms5803* sensor){
    // Transmit the reset instruction
    MS5803_CsDelay();
    SPI_ResetCS(sensor->spi);
    SPI_Transfer8(sensor->spi, MS5803_RESET);
    // We have to wait at least 2.8ms while the part resets
    sensor->waitUntil = CSP_TotalClockCycles() +
        (uint64_t)(SystemCoreClock * 3e-3);
    while(CSP_TotalClockCycles() < sensor->waitUntil);
    // End the sequence
    SPI_SetCS(sensor->spi);
}

// Retrieve pressure data
void MS5803_UpdatePressure(Ms5803* sensor){
    MS5803_StartConversion(sensor, MS5803_SELD1 | MS5803_OSR4096);
	sensor->p_raw = (int32_t)MS5803_ReadADC(sensor);
}

// Retrieve temperature data
void MS5803_UpdateTemperature(Ms5803* sensor){
    MS5803_StartConversion(sensor, MS5803_SELD2 | MS5803_OSR4096);
	sensor->t_raw = MS5803_ReadADC(sensor);
}

// Retrieve factory calibration data
void MS5803_UpdateCalibration(Ms5803* sensor){
	for(uint32_t a = 0; a < 8; a++){
        MS5803_CsDelay();
		SPI_ResetCS(sensor->spi);
        SPI_Transfer8(sensor->spi, MS5803_PROMREAD | (a << 1));
        SPI_ReadMulti_Big(sensor->spi, 1, sizeof(sensor->prom[a]), &(sensor->prom[a]));
		SPI_SetCS(sensor->spi);
	}
}

// First order computations
void MS5803_ComputeFirstOrderCorrections(Ms5803* sensor){
    // Compute temperature
  	sensor->dt = sensor->t_raw - ((int64_t)sensor->prom[5] << 8);
	sensor->temp = (int64_t)2000 + ((sensor->dt * (int64_t)sensor->prom[6]) \
					>> 23);
    sensor->temperature = sensor->temp / 100.0f;

    // Compute pressure
    sensor->off = 	((int64_t)sensor->prom[2] << 16) + \
					(((int64_t)sensor->prom[4] * sensor->dt) >> 7);
	sensor->sens = 	((int64_t)sensor->prom[1] << 15) + \
					(((int64_t)sensor->prom[3] * sensor->dt) >> 8);
	sensor->p = 	((((int64_t)sensor->p_raw * sensor->sens) >> 21) \
					- sensor->off) >> 15;
}

// Second order computations
void MS5803_ComputeSecondOrderCorrections(Ms5803* sensor){
    int64_t t2 = 0, off2 = 0, sens2 = 0;

	// This follows the logic tree for second order temperature corrections
    if(sensor->temp > 4500){
        int64_t newtemp = sensor->temp - 4500;
        sens2 = sens2 - ((newtemp * newtemp) >> 3);
    }
    else if(sensor->temp >= 2000){
        sensor->pressure = sensor->p / 1000.0f;
        return;
    }
    else if(sensor->temp < 2000){
        int64_t sqtemp = (sensor->temp - 2000) * (sensor->temp - 2000);

        t2 = (sensor->dt * sensor->dt) >> 31;
        off2 = 3 * sqtemp;
        sens2 = 7 * sqtemp >> 3;

        if(sensor->temp < -1500)
            sens2 += (2 * (sensor->temp + 1500) * (sensor->temp + 1500));
    }

    // Integrate our newly computed values in to previously acquired values
    sensor->temp -= t2;
    sensor->off -= off2;
    sensor->sens -= sens2;

    // Recompute pressure
   	sensor->p = ((((int64_t)sensor->p_raw * sensor->sens) >> 21) \
					- sensor->off) >> 15;
    sensor->pressure = sensor->p / 1000.0f;
}

// Compute altitude
float MS5803_ComputeAltitude(Ms5803* sensor){
    // This is an implementation of the hypsometric formula
    float Po = 101325.0f;
    //float To = 288.15f;
    float To = sensor->temperature;
    float g = 9.80665f;
    float L = -6.5e-3f;
    float R = 287.053f;
    float p = (float)(sensor->p);
    sensor->altitude = (To/L) * (powf((p/Po), (-L*R/g)) - 1);
    return sensor->altitude;
}
