#ifndef ADC0832_H
#define ADC0832_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void adc0832_init(void);
uint8_t adc0832_read(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif
