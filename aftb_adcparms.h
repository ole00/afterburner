/*
 * Parameters for new varVppMeasureVpp()
 * Select the data set that matches your hardware variant or add a suitable data set.
 * 2024-02-14 Initial release
 */
#ifndef __AFTB_ADCPARMS_H__
#define __AFTB_ADCPARMS_H__

// Select your hardware version (AREF source)
#define AREF_1V1            // Defined data sets AREF_3V3, AREF_1V1, AREF_3V3_R4
// For best results, measure the voltage on AREF pin with a good multimeter and use this value for VREF

#if defined(AREF_3V3)
// Values for ATmega328P (NANO V3, UNO R3) AREF = EXTERNAL R7 = 3k3, Ri = 32k (original Afterburner)
#define ADC_R5 (100.0)      // R5 in kOhm as float
#define ADC_R6 (20.0)       // R6 in kOhm as float
#define VREF (3.0)          // Vref in Volts as float; Vref = 3.3V * 32k / (32k + 3k3) => 2.991V
#define AREF_SOURCE EXTERNAL

#elif defined(AREF_1V1)
// Values for ATmega328P (NANO V3, UNO R3) AREF = INTERNAL 1.1V, R5 = 20k, R6 = 1k3, R7 not populated
#define ADC_R5 (20.0)       // R5 in kOhm as float
#define ADC_R6 (1.3)        // R6 in kOhm as float
#define VREF (1.1)          // Vref in Volts as float; Vref = 1.1V
#define AREF_SOURCE INTERNAL

#elif defined(AREF_3V3_R4)
// Values for Renesas RA4M1 (UNO R4 Minima/WiFi) AREF = EXTERNAL R7 = 3k3, Ri = 130k (original Afterburner)
#define ADC_R5 (100.0)      // R5 in kOhm as float
#define ADC_R6 (20.0)       // R6 in kOhm as float
#define VREF (3.2)          // Vref in Volts as float; Vref = 3.3V * 130k / (130k + 3k3) => 3.218V
#define AREF_SOURCE AR_EXTERNAL

// You can add additional variants here

#else
// Invalid or missing AREF variant
#undef ADC_R5
#undef ADC_R6
#undef VREF
#undef AREF_SOURCE
#error "Invalid or missing AREF variant\n"
#endif

#endif
