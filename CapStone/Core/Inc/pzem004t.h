#ifndef INC_PZEM004T_H_
#define INC_PZEM004T_H_

#include "main.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PZEM_DEFAULT_ADDR      0xF8U

typedef enum
{
    PZEM_OK = 0,
    PZEM_ERR_UART,
    PZEM_ERR_CRC,
    PZEM_ERR_FRAME,
    PZEM_ERR_PARAM
} PZEM_Status_t;

typedef struct
{
    float voltage;
    float current;
    float power;
    float energy;
    float frequency;
    float pf;
    uint16_t alarm;
} PZEM_Data_t;

PZEM_Status_t PZEM_ReadMeasurements(UART_HandleTypeDef *huart, uint8_t addr, PZEM_Data_t *out);
uint16_t PZEM_CRC16(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* INC_PZEM004T_H_ */