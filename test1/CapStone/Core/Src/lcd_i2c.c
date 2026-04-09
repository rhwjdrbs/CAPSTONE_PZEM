#include "lcd_i2c.h"

extern I2C_HandleTypeDef hi2c1;

/*
 * LCD I2C 주소
 * - 보통 0x27 또는 0x3F
 * - I2C 스캔 결과에 맞게 수정
 */
#define LCD_ADDR        (0x27 << 1)

/* PCF8574 백팩 비트 매핑(가장 흔한 타입 기준) */
#define LCD_BACKLIGHT   0x08
#define LCD_ENABLE      0x04
#define LCD_RS          0x01

#define LCD_TIMEOUT_MS  20

static HAL_StatusTypeDef LCD_Write4Bits(uint8_t data);
static HAL_StatusTypeDef LCD_SendInternal(uint8_t value, uint8_t mode);

/* LCD 존재 확인 */
HAL_StatusTypeDef LCD_IsPresent(void)
{
    return HAL_I2C_IsDeviceReady(&hi2c1, LCD_ADDR, 2, LCD_TIMEOUT_MS);
}

/* 4비트 전송 */
static HAL_StatusTypeDef LCD_Write4Bits(uint8_t data)
{
    uint8_t buf[2];
    HAL_StatusTypeDef st;

    buf[0] = data | LCD_ENABLE;
    buf[1] = data & ~LCD_ENABLE;

    st = HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, buf, 2, LCD_TIMEOUT_MS);
    HAL_Delay(1);

    return st;
}

/* 명령/데이터 전송 */
static HAL_StatusTypeDef LCD_SendInternal(uint8_t value, uint8_t mode)
{
    HAL_StatusTypeDef st;
    uint8_t high = (value & 0xF0) | LCD_BACKLIGHT | mode;
    uint8_t low  = ((value << 4) & 0xF0) | LCD_BACKLIGHT | mode;

    st = LCD_Write4Bits(high);
    if (st != HAL_OK) return st;

    st = LCD_Write4Bits(low);
    if (st != HAL_OK) return st;

    return HAL_OK;
}

/* LCD 초기화 */
HAL_StatusTypeDef LCD_Init(void)
{
    HAL_StatusTypeDef st;

    /* 장치 응답 확인 */
    if (LCD_IsPresent() != HAL_OK)
    {
        return HAL_ERROR;
    }

    HAL_Delay(50);

    /* 초기화 시퀀스 */
    st = LCD_Write4Bits(0x30 | LCD_BACKLIGHT);
    if (st != HAL_OK) return st;
    HAL_Delay(5);

    st = LCD_Write4Bits(0x30 | LCD_BACKLIGHT);
    if (st != HAL_OK) return st;
    HAL_Delay(1);

    st = LCD_Write4Bits(0x30 | LCD_BACKLIGHT);
    if (st != HAL_OK) return st;
    HAL_Delay(1);

    st = LCD_Write4Bits(0x20 | LCD_BACKLIGHT);   // 4-bit mode
    if (st != HAL_OK) return st;
    HAL_Delay(1);

    st = LCD_SendInternal(0x28, 0);  // 4bit, 2line, 5x8
    if (st != HAL_OK) return st;

    st = LCD_SendInternal(0x0C, 0);  // display on, cursor off
    if (st != HAL_OK) return st;

    st = LCD_SendInternal(0x06, 0);  // entry mode
    if (st != HAL_OK) return st;

    st = LCD_SendInternal(0x01, 0);  // clear
    HAL_Delay(2);
    if (st != HAL_OK) return st;

    return HAL_OK;
}

/* 화면 지우기 */
HAL_StatusTypeDef LCD_Clear(void)
{
    HAL_StatusTypeDef st;

    st = LCD_SendInternal(0x01, 0);
    HAL_Delay(2);

    return st;
}

/* 커서 이동 */
HAL_StatusTypeDef LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};

    if (row > 1) row = 1;   // 16x2 기준
    if (col > 15) col = 15;

    return LCD_SendInternal(0x80 | (col + row_offsets[row]), 0);
}

/* 문자열 출력 */
HAL_StatusTypeDef LCD_Print(const char *str)
{
    HAL_StatusTypeDef st;

    while (*str)
    {
        st = LCD_SendInternal((uint8_t)(*str), LCD_RS);
        if (st != HAL_OK)
        {
            return st;
        }
        str++;
    }

    return HAL_OK;
}

/* 지정 위치 문자열 출력 */
HAL_StatusTypeDef LCD_PrintAt(uint8_t row, uint8_t col, const char *str)
{
    HAL_StatusTypeDef st;

    st = LCD_SetCursor(row, col);
    if (st != HAL_OK)
    {
        return st;
    }

    return LCD_Print(str);
}