#include "lcd_i2c.h"

extern I2C_HandleTypeDef hi2c1;

/* 실제 주소는 스캔해서 확인 후 수정 */
#define LCD_ADDR        (0x3F << 1)

#define LCD_BACKLIGHT   0x08
#define LCD_ENABLE      0x04
#define LCD_RS          0x01

#define LCD_TIMEOUT_MS  20

static HAL_StatusTypeDef LCD_Write4Bits(uint8_t data);
static HAL_StatusTypeDef LCD_SendInternal(uint8_t value, uint8_t mode);

HAL_StatusTypeDef LCD_IsPresent(void)
{
    return HAL_I2C_IsDeviceReady(&hi2c1, LCD_ADDR, 2, LCD_TIMEOUT_MS);
}

static HAL_StatusTypeDef LCD_Write4Bits(uint8_t data)
{
    uint8_t buf[2];
    buf[0] = data | LCD_ENABLE;
    buf[1] = data & ~LCD_ENABLE;
    return HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, buf, 2, LCD_TIMEOUT_MS);
}

static HAL_StatusTypeDef LCD_SendInternal(uint8_t value, uint8_t mode)
{
    HAL_StatusTypeDef st;
    uint8_t high = (value & 0xF0) | LCD_BACKLIGHT | mode;
    uint8_t low  = ((value << 4) & 0xF0) | LCD_BACKLIGHT | mode;

    st = LCD_Write4Bits(high);
    if (st != HAL_OK) return st;

    st = LCD_Write4Bits(low);
    if (st != HAL_OK) return st;

    HAL_Delay(1);
    return HAL_OK;
}

HAL_StatusTypeDef LCD_Clear(void)
{
    HAL_StatusTypeDef st = LCD_SendInternal(0x01, 0);
    HAL_Delay(2);
    return st;
}

HAL_StatusTypeDef LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    return LCD_SendInternal(0x80 | (col + row_offsets[row]), 0);
}

HAL_StatusTypeDef LCD_Print(const char *str)
{
    HAL_StatusTypeDef st;
    while (*str)
    {
        st = LCD_SendInternal((uint8_t)(*str), LCD_RS);
        if (st != HAL_OK) return st;
        str++;
    }
    return HAL_OK;
}

HAL_StatusTypeDef LCD_PrintAt(uint8_t row, uint8_t col, const char *str)
{
    HAL_StatusTypeDef st = LCD_SetCursor(row, col);
    if (st != HAL_OK) return st;
    return LCD_Print(str);
}

HAL_StatusTypeDef LCD_Init(void)
{
    HAL_StatusTypeDef st;

    if (LCD_IsPresent() != HAL_OK)
        return HAL_ERROR;

    HAL_Delay(50);

    st = LCD_Write4Bits(0x30 | LCD_BACKLIGHT);
    if (st != HAL_OK) return st;
    HAL_Delay(5);

    st = LCD_Write4Bits(0x30 | LCD_BACKLIGHT);
    if (st != HAL_OK) return st;
    HAL_Delay(1);

    st = LCD_Write4Bits(0x30 | LCD_BACKLIGHT);
    if (st != HAL_OK) return st;
    HAL_Delay(1);

    st = LCD_Write4Bits(0x20 | LCD_BACKLIGHT);
    if (st != HAL_OK) return st;

    st = LCD_SendInternal(0x28, 0);
    if (st != HAL_OK) return st;

    st = LCD_SendInternal(0x0C, 0);
    if (st != HAL_OK) return st;

    st = LCD_SendInternal(0x06, 0);
    if (st != HAL_OK) return st;

    return LCD_Clear();
}