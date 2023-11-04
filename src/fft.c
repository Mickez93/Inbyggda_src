/*
 * fft.c
 *
 *  Created on: Oct 19, 2023
 *      Author: danne
 */

#include "fft.h"

arm_rfft_fast_instance_f32 S;
float32_t inputBuf_FFT[NR_SAMPLES], outputBuf_FFT[NR_SAMPLES];
uint16_t adc_q_buf[NR_SAMPLES];
bool fft_count_flag = false;
uint8_t velocity_array[FFT_MAX_COUNTS];
uint16_t fft_count = 0;
uint16_t median_velocity = 0;
float32_t peakVal = 0.0f;
uint16_t peakHz = 0;
volatile uint32_t start_tick = 0;
volatile uint8_t start_flag = 0;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc){
	HAL_GPIO_WritePin(PWR_RADAR_GPIO_Port, PWR_RADAR_Pin, ON);
	for(int i = 0 ; i < NR_SAMPLES ; i++){
		inputBuf_FFT[i] = (float32_t)adc_q_buf[i] - 32768;
	}

  arm_rfft_fast_f32(&S, inputBuf_FFT, outputBuf_FFT, 0);
  peakVal = 0.0f;
  peakHz = 0.0f;
  uint16_t freqIndex = 0;
  int velocity_saved = 0;

  for(uint16_t index = 0 ; index < NR_SAMPLES ; index+=2){
	  float curVal = sqrtf((outputBuf_FFT[index] * outputBuf_FFT[index]) + (outputBuf_FFT[index+1] * outputBuf_FFT[index+1]));

	  if(freqIndex != 0 && curVal > PEAK_THRESHOLD && !fft_count_flag){
		  fft_count_flag = true;
		  peakHz = (uint16_t)(freqIndex * SAMPLE_RATE_HZ / ((float)NR_SAMPLES));
		  velocity_saved = (int)(peakHz / (44 * cos(RADIANS)));
	  }
	  else if(curVal > PEAK_THRESHOLD){
		  peakHz = (uint16_t)(freqIndex * SAMPLE_RATE_HZ / ((float)NR_SAMPLES));
		  uint8_t vel = (int)(peakHz / (44 * cos(RADIANS)));
		  velocity_saved = vel;
	  }
	  freqIndex++;
  }

  if(fft_count_flag){
	  velocity_array[fft_count] = velocity_saved;
	  fft_count++;

	  if(fft_count >= FFT_MAX_COUNTS){
		median_velocity = calculate_median(velocity_array, FFT_MAX_COUNTS);
		fft_count_flag = false;
		fft_count = 0;
		vel_array[vel_index] = median_velocity;
		print_uart("Peak vel: %d \r\n", vel_array[vel_index]);
		vel_index++;
		median_velocity = 0;
	  }
  }
}

// 250 ms timer
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim == &htim3){
		HAL_GPIO_WritePin(PWR_RADAR_GPIO_Port, PWR_RADAR_Pin, ON);
		start_tick = HAL_GetTick();
		start_flag = 0;
	}
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef * hrtc){
	rtc_flag = true;
}

int compare_ints(const void* a, const void* b) {
    int arg1 = *(const uint8_t*)a;
    int arg2 = *(const uint8_t*)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

uint8_t calculate_median(uint8_t* array, int size) {
    // Skapa en ny array som bara innehåller värden som inte är nollor
    uint8_t non_zero_values[size];
    int non_zero_count = 0;

    for (int i = 0; i < size; i++) {
        if (array[i] != 0) {
            non_zero_values[non_zero_count] = array[i];
            non_zero_count++;
        }
    }

    // Om alla värden är nollor, returnera 0 som median
    if (non_zero_count == 0) {
        return 0;
    }

    // Sortera den nya arrayen
    qsort(non_zero_values, non_zero_count, sizeof(uint8_t), compare_ints);

    // Om antalet icke-noll-värden är udda, returnera det mittersta elementet
    if (non_zero_count % 2 == 1) {
        return non_zero_values[non_zero_count / 2];
    }
    // Om antalet icke-noll-värden är jämnt, returnera genomsnittet av de två mittersta elementen
    return (non_zero_values[non_zero_count / 2 - 1] + non_zero_values[non_zero_count / 2]) / 2;
}
