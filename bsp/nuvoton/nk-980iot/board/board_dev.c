/**************************************************************************//**
*
* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date            Author       Notes
* 2020-12-12      Wayne        First version
*
******************************************************************************/

#include <rtconfig.h>
#include <rtdevice.h>

#if defined(BOARD_USING_STORAGE_SPIFLASH)
#if defined(RT_USING_SFUD)
    #include "spi_flash.h"
    #include "spi_flash_sfud.h"
#endif

#include "drv_qspi.h"

#define W25X_REG_READSTATUS    (0x05)
#define W25X_REG_READSTATUS2   (0x35)
#define W25X_REG_WRITEENABLE   (0x06)
#define W25X_REG_WRITESTATUS   (0x01)
#define W25X_REG_QUADENABLE    (0x02)

static rt_uint8_t SpiFlash_ReadStatusReg(struct rt_qspi_device *qspi_device)
{
    rt_uint8_t u8Val;
    rt_uint8_t w25x_txCMD1 = W25X_REG_READSTATUS;
    rt_qspi_send_then_recv(qspi_device, &w25x_txCMD1, 1, &u8Val, 1);

    return u8Val;
}

static rt_uint8_t SpiFlash_ReadStatusReg2(struct rt_qspi_device *qspi_device)
{
    rt_uint8_t u8Val;
    rt_uint8_t w25x_txCMD1 = W25X_REG_READSTATUS2;
    rt_qspi_send_then_recv(qspi_device, &w25x_txCMD1, 1, &u8Val, 1);

    return u8Val;
}

static void SpiFlash_WriteStatusReg(struct rt_qspi_device *qspi_device, uint8_t u8Value1, uint8_t u8Value2)
{
    rt_uint8_t w25x_txCMD1;
    rt_uint8_t u8Val[3];

    w25x_txCMD1 = W25X_REG_WRITEENABLE;
    rt_qspi_send(qspi_device, &w25x_txCMD1, 1);

    u8Val[0] = W25X_REG_WRITESTATUS;
    u8Val[1] = u8Value1;
    u8Val[2] = u8Value2;
    rt_qspi_send(qspi_device, &u8Val, 3);
}

static void SpiFlash_WaitReady(struct rt_qspi_device *qspi_device)
{
    volatile uint8_t u8ReturnValue;

    do
    {
        u8ReturnValue = SpiFlash_ReadStatusReg(qspi_device);
        u8ReturnValue = u8ReturnValue & 1;
    }
    while (u8ReturnValue != 0);   // check the BUSY bit
}

static void SpiFlash_EnterQspiMode(struct rt_qspi_device *qspi_device)
{
    uint8_t u8Status1 = SpiFlash_ReadStatusReg(qspi_device);
    uint8_t u8Status2 = SpiFlash_ReadStatusReg2(qspi_device);

    u8Status2 |= W25X_REG_QUADENABLE;
    SpiFlash_WriteStatusReg(qspi_device, u8Status1, u8Status2);
    SpiFlash_WaitReady(qspi_device);
}

static void SpiFlash_ExitQspiMode(struct rt_qspi_device *qspi_device)
{
    uint8_t u8Status1 = SpiFlash_ReadStatusReg(qspi_device);
    uint8_t u8Status2 = SpiFlash_ReadStatusReg2(qspi_device);

    u8Status2 &= ~W25X_REG_QUADENABLE;
    SpiFlash_WriteStatusReg(qspi_device, u8Status1, u8Status2);
    SpiFlash_WaitReady(qspi_device);
}

static int rt_hw_spiflash_init(void)
{
    if (nu_qspi_bus_attach_device("qspi0", "qspi01", 4, SpiFlash_EnterQspiMode, SpiFlash_ExitQspiMode) != RT_EOK)
        return -1;

#if defined(RT_USING_SFUD)
    if (rt_sfud_flash_probe(FAL_USING_NOR_FLASH_DEV_NAME, "qspi01") == RT_NULL)
    {
        return -(RT_ERROR);
    }
#endif
    return 0;
}
INIT_COMPONENT_EXPORT(rt_hw_spiflash_init);
#endif /* BOARD_USING_STORAGE_SPIFLASH */

#if defined(BOARD_USING_LCD_ILI9341) && defined(NU_PKG_USING_ILI9341_SPI)
#include <lcd_ili9341.h>
#if defined(PKG_USING_GUIENGINE)
    #include <rtgui/driver.h>
#endif
int rt_hw_ili9341_port(void)
{
    if (rt_hw_lcd_ili9341_spi_init("spi0") != RT_EOK)
        return -1;

    rt_hw_lcd_ili9341_init();

#if defined(PKG_USING_GUIENGINE)
    rt_device_t lcd_ili9341;
    lcd_ili9341 = rt_device_find("lcd");
    if (lcd_ili9341)
    {
        rtgui_graphic_set_device(lcd_ili9341);
    }
#endif
    return 0;
}
INIT_COMPONENT_EXPORT(rt_hw_ili9341_port);
#endif /* BOARD_USING_LCD_ILI9341 */


#if defined(BOARD_USING_ESP8266)
#include <at_device_esp8266.h>

#define LOG_TAG     "at.sample.esp"
#undef DBG_TAG
#undef DBG_LVL
#include <at_log.h>

static struct at_device_esp8266 esp0 =
{
    "esp0",          /* esp8266 device name */
    "uart1",         /* esp8266 serial device name, EX: uart1, uuart1 */

    "NT_ZY_BUFFALO", /* Wi-Fi SSID */
    "12345678",      /* Wi-Fi PASSWORD */
    2048             /* Receive buffer length */
};

static int rt_hw_esp8266_port(void)
{
    struct at_device_esp8266 *esp8266 = &esp0;

    return at_device_register(&(esp8266->device),
                              esp8266->device_name,
                              esp8266->client_name,
                              AT_DEVICE_CLASS_ESP8266,
                              (void *) esp8266);
}
//INIT_APP_EXPORT(rt_hw_esp8266_port);

static void at_wifi_set(int argc, char **argv)
{
    struct at_device_ssid_pwd sATDConf;
    struct at_device *at_dev = RT_NULL;

    /* If the number of arguments less than 2 */
    if (argc != 3)
    {
        rt_kprintf("\n");
        rt_kprintf("at_wifi_set <ssid> <password>\n");
        return ;
    }

    sATDConf.ssid     = argv[1]; //ssid
    sATDConf.password = argv[2]; //password

    if ((at_dev = at_device_get_first_initialized()) != RT_NULL)
        at_device_control(at_dev, AT_DEVICE_CTRL_SET_WIFI_INFO, &sATDConf);
    else
    {
        rt_kprintf("Can't find any initialized AT device.\n");
    }
}
#ifdef FINSH_USING_MSH
    MSH_CMD_EXPORT(at_wifi_set, AT device wifi set ssid / password function);
#endif
#endif /* BOARD_USING_ESP8266  */


#if defined(BOARD_USING_NAU8822) && defined(NU_PKG_USING_NAU8822)
#include <acodec_nau8822.h>
S_NU_NAU8822_CONFIG sCodecConfig =
{
    .i2c_bus_name = "i2c0",

    .i2s_bus_name = "sound0",

    .pin_phonejack_en = 0,

    .pin_phonejack_det = 0,
};

int rt_hw_nau8822_port(void)
{
    if (nu_hw_nau8822_init(&sCodecConfig) != RT_EOK)
        return -1;

    return 0;
}
INIT_COMPONENT_EXPORT(rt_hw_nau8822_port);
#endif /* BOARD_USING_NAU8822 */
