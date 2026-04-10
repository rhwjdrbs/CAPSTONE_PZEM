#ifndef INC_LCD_I2C_H_
#define INC_LCD_I2C_H_

#include "main.h"
#include <stdint.h>

HAL_StatusTypeDef LCD_Init(void);
HAL_StatusTypeDef LCD_Clear(void);
HAL_StatusTypeDef LCD_SetCursor(uint8_t row, uint8_t col);
HAL_StatusTypeDef LCD_Print(const char *str);
HAL_StatusTypeDef LCD_PrintAt(uint8_t row, uint8_t col, const char *str);
HAL_StatusTypeDef LCD_IsPresent(void);

#endif /* INC_LCD_I2C_H_ */