#include "pzem004t.h"

#define PZEM_CMD_READ_INPUT_REGS   0x04U
#define PZEM_REG_START             0x0000U
#define PZEM_REG_COUNT             0x000AU

#define PZEM_REQ_LEN               8U
#define PZEM_RESP_LEN              25U
#define PZEM_DATA_BYTES            20U

#define PZEM_TX_TIMEOUT_MS         100U
#define PZEM_RX_TIMEOUT_MS         300U

static void PZEM_BuildReadFrame(uint8_t addr, uint8_t *frame);
static uint32_t PZEM_Parse32(const uint8_t *p);

uint16_t PZEM_CRC16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

static void PZEM_BuildReadFrame(uint8_t addr, uint8_t *frame)
{
    frame[0] = addr;
    frame[1] = PZEM_CMD_READ_INPUT_REGS;
    frame[2] = (uint8_t)(PZEM_REG_START >> 8);
    frame[3] = (uint8_t)(PZEM_REG_START & 0xFF);
    frame[4] = (uint8_t)(PZEM_REG_COUNT >> 8);
    frame[5] = (uint8_t)(PZEM_REG_COUNT & 0xFF);

    uint16_t crc = PZEM_CRC16(frame, 6);
    frame[6] = (uint8_t)(crc & 0xFF);        // CRC Low
    frame[7] = (uint8_t)((crc >> 8) & 0xFF); // CRC High
}

/*
 * PZEM v3 응답의 32-bit 값은 일반적인 big-endian 4바이트가 아니라
 * 라이브러리 구현 기준으로 다음 순서로 묶습니다.
 *
 * value = p[0]<<8 | p[1] | p[2]<<24 | p[3]<<16
 *
 * current:  p = &rx[5]
 * power:    p = &rx[9]
 * energy:   p = &rx[13]
 */
static uint32_t PZEM_Parse32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 8)  |
           ((uint32_t)p[1])       |
           ((uint32_t)p[2] << 24) |
           ((uint32_t)p[3] << 16);
}

PZEM_Status_t PZEM_ReadMeasurements(UART_HandleTypeDef *huart, uint8_t addr, PZEM_Data_t *out)
{
    if ((huart == NULL) || (out == NULL))
    {
        return PZEM_ERR_PARAM;
    }

    uint8_t tx[PZEM_REQ_LEN] = {0};
    uint8_t rx[PZEM_RESP_LEN] = {0};

    PZEM_BuildReadFrame(addr, tx);

    if (HAL_UART_Transmit(huart, tx, PZEM_REQ_LEN, PZEM_TX_TIMEOUT_MS) != HAL_OK)
    {
        return PZEM_ERR_UART;
    }

    if (HAL_UART_Receive(huart, rx, PZEM_RESP_LEN, PZEM_RX_TIMEOUT_MS) != HAL_OK)
    {
        return PZEM_ERR_UART;
    }

    /* 기본 프레임 확인 */
    if (rx[1] != PZEM_CMD_READ_INPUT_REGS)
    {
        return PZEM_ERR_FRAME;
    }

    if (rx[2] != PZEM_DATA_BYTES)
    {
        return PZEM_ERR_FRAME;
    }

    /*
     * addr = 0xF8(기본/global)일 때는 응답 address를 엄격 체크하지 않음
     * 나중에 개별 주소(0x01~0xF7)로 바꾸면 아래 체크를 켜도 됨
     */
    if (addr != PZEM_DEFAULT_ADDR)
    {
        if (rx[0] != addr)
        {
            return PZEM_ERR_FRAME;
        }
    }

    /* CRC 확인 */
    uint16_t crc_calc = PZEM_CRC16(rx, PZEM_RESP_LEN - 2);
    uint16_t crc_recv = (uint16_t)rx[23] | ((uint16_t)rx[24] << 8);

    if (crc_calc != crc_recv)
    {
        return PZEM_ERR_CRC;
    }

    /* 값 파싱 */
    out->voltage   = ((float)(((uint16_t)rx[3] << 8) | rx[4])) / 10.0f;
    out->current   = ((float)PZEM_Parse32(&rx[5]))  / 1000.0f;
    out->power     = ((float)PZEM_Parse32(&rx[9]))  / 10.0f;
    out->energy    = ((float)PZEM_Parse32(&rx[13])) / 1000.0f;
    out->frequency = ((float)(((uint16_t)rx[17] << 8) | rx[18])) / 10.0f;
    out->pf        = ((float)(((uint16_t)rx[19] << 8) | rx[20])) / 100.0f;
    out->alarm     = ((uint16_t)rx[21] << 8) | rx[22];

    return PZEM_OK;
}