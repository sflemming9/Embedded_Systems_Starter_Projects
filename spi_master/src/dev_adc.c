#include "adc.h"

#include <FreeRTOS.h>
#include <task.h>


void ADC_Setup() {
    GPIO_InitTypeDef        GPIO_InitStructure;
    ADC_InitTypeDef         ADC_InitStructure;
    ADC_CommonInitTypeDef   ADC_CommonInitStructure;

    // enable clocks for GPIO and ADC
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    // configure PA0, PA1 as alternate function inputs
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // ADC1 configuration
    ADC_CommonStructInit(&ADC_CommonInitStructure);
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    ADC_StructInit(&ADC_InitStructure);
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);
}

uint16_t ADC_GetChannel(uint8_t channel) {
    ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_84Cycles);
    // Start the conversion
    ADC_SoftwareStartConv(ADC1);
    // Wait until conversion completion
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    // Get the conversion value
    return ADC_GetConversionValue(ADC1);
}

#define CUTOFF_FREQUENCY 10 // Hz

// TODO: Lift correction/internal resistance correction?
uint16_t filterVoltage(uint16_t oldVal, uint16_t newSample) {
    // Filter coefficients
    static float rc = 1.0 / (2.0 * 3.1415926 * CUTOFF_FREQUENCY);
    static float dt = 1.0 / ADC_RATE;
    float alpha = dt / (rc + dt);
    return (uint16_t) (oldVal + (alpha * (newSample - oldVal)));
}

// low-pass filter works using precomputed alpha = 0.5
uint16_t filterVoltageFast(uint16_t oldVal, uint16_t newSample) {
    return oldVal + ((newSample - oldVal) >> 1);
}


void ADC_Poll(volatile ADC_Value *adc) {

    adc->brake[1] = ADC_GetChannel(0);
    adc->throttle[1] = ADC_GetChannel(1);

    adc->brake[1] = filterVoltageFast(adc->brake[1], ADC_GetChannel(0));
    adc->brake[0] = adc->brake[1];

    adc->throttle[1] = filterVoltageFast(adc->throttle[1], ADC_GetChannel(1));
    adc->throttle[0] = adc->throttle[1];
}

void ADC_InitVal(volatile ADC_Value *adc) {

    for (uint8_t i=0; i<ADC_BUFF_SIZE; ++i) {
        adc->brake[i] = 0;
        adc->throttle[i] = 0;
    }
}
/*
void adcTask(void* pvParameters){
  ADC_Setup();
  ADC_InitVal(&adc_val);
  while(1){
    ADC_Poll(&adc_val);
    printf("%d\n", adc_val.throttle[ADC_BUFF_SIZE - 1]);
    vTaskDelay(1000);
  }
}
*/