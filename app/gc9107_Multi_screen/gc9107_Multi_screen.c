/**
  ******************************************************************************
  * @file    gc9107_Multi_screen.c
  * @author Sifli software development team
  * @brief   This file includes the LCD driver for GC9107 LCD (Triple Screen).
  * @attention
  ******************************************************************************
*/
/**
 * @attention
 * Copyright (c) 2019 - 2022,  Sifli Technology
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Sifli integrated circuit
 *    in a product or a software update for such product, must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Sifli nor the names of its contributors may be used to endorse
 *    or promote products derived from this software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Sifli integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY SIFLI TECHNOLOGY "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SIFLI TECHNOLOGY OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <rtthread.h>
#include "string.h"
#include "board.h"
#include "drv_io.h"
#include "drv_lcd.h"

#include "log.h"

#define ROW_OFFSET  (0)

/**
  * @brief GC9107 chip IDs
  */
#define THE_LCD_ID                  0x1190a7

/**
  * @brief  GC9107 Size
  */
#define  THE_LCD_PIXEL_WIDTH    128
#define  THE_LCD_PIXEL_HEIGHT   128

/**
  * @brief  GC9107 Registers
  */
#define REG_LCD_ID             0x04
#define REG_SLEEP_IN           0x10
#define REG_SLEEP_OUT          0x11
#define REG_PARTIAL_DISPLAY    0x12
#define REG_DISPLAY_INVERSION  0x21
#define REG_DISPLAY_OFF        0x28
#define REG_DISPLAY_ON         0x29
#define REG_WRITE_RAM          0x2C
#define REG_READ_RAM           0x2E
#define REG_CASET              0x2A
#define REG_RASET              0x2B

#define REG_TEARING_EFFECT     0x35
#define REG_NORMAL_DISPLAY     0x36
#define REG_IDLE_MODE_OFF      0x38
#define REG_IDLE_MODE_ON       0x39
#define REG_COLOR_MODE         0x3A
#define REG_WBRIGHT            0x51 /* Write brightness*/

#define REG_PORCH_CTRL         0xB2
#define REG_FRAME_CTRL         0xB3
#define REG_GATE_CTRL          0xB7
#define REG_VCOM_SET           0xBB
#define REG_LCM_CTRL           0xC0
#define REG_VDV_VRH_EN         0xC2
#define REG_VDV_SET            0xC4

#define REG_FR_CTRL            0xC6
#define REG_POWER_CTRL         0xD0
#define REG_PV_GAMMA_CTRL      0xE0
#define REG_NV_GAMMA_CTRL      0xE1

#ifdef DEBUG
    #define DEBUG_PRINTF(...)   LOG_I(__VA_ARGS__)
#else
    #define DEBUG_PRINTF(...)
#endif

/* 定义液晶分辨率 */
#define USE_HORIZONTAL 0 // 设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏

// 定义MAX/MIN宏（如果尚未定义）
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

// 三连屏引脚定义 - 更新为新的引脚配置
#define LCD_CS_PIN_1  03  // PA01
#define LCD_CS_PIN_2  02  // PA02
#define LCD_CS_PIN_3  01  // PA03
#define LCD_RST_PIN   00  // PA00 复位引脚

// 屏幕配置结构体
typedef struct {
    uint8_t cs_pin;
    uint8_t col;
    uint8_t row;
} ScreenConfig;

#define LCD_SCREEN_NUM 3   // 屏幕数量 - 从6个改为3个

// 屏幕CS引脚数组 - 更新为新的引脚
static uint16_t lcd_cs_pins[LCD_SCREEN_NUM] = {LCD_CS_PIN_1, LCD_CS_PIN_2, LCD_CS_PIN_3};

// 屏幕CS引脚对应的GPIO和PAD配置 - 更新为新的引脚
static const struct {
    uint8_t pad;
    uint8_t gpio;
} lcd_cs_pad_gpio[LCD_SCREEN_NUM] = {
    {PAD_PA01, GPIO_A1},   // 更新：PA01
    {PAD_PA02, GPIO_A2},   // 更新：PA02
    {PAD_PA03, GPIO_A3},   // 更新：PA03
};

// 屏幕映射配置 - 更新为单排三连屏布局
static const ScreenConfig screen_map[LCD_SCREEN_NUM] = {
    {LCD_CS_PIN_1, 0, 0},  // 第1块屏：第0行，第0列
    {LCD_CS_PIN_2, 1, 0},  // 第2块屏：第0行，第1列
    {LCD_CS_PIN_3, 2, 0},  // 第3块屏：第0行，第2列
    // 删除：第二排的三个屏幕配置
};

static void LCD_WriteReg(LCDC_HandleTypeDef *hlcdc, uint16_t LCD_Reg, uint8_t *Parameters, uint32_t NbParameters);
static uint32_t LCD_ReadData(LCDC_HandleTypeDef *hlcdc, uint16_t RegValue, uint8_t ReadSize);
static void LCD_WriteReg_More(LCDC_HandleTypeDef *hlcdc, uint16_t LCD_Reg, uint8_t *Parameters, uint32_t NbParameters);
static uint16_t current_lcd_cs_pin;
static uint16_t Region_Xpos0, Region_Ypos0, Region_Xpos1, Region_Ypos1;

static LCDC_InitTypeDef lcdc_int_cfg =
{
    .lcd_itf = LCDC_INTF_SPI_DCX_1DATA,
    .freq = 48000000,
    .color_mode = LCDC_PIXEL_FORMAT_RGB565,
    .cfg = {
        .spi = {
            .dummy_clock = 0,
            .syn_mode = HAL_LCDC_SYNC_DISABLE,
            .vsyn_polarity = 0,
            .vsyn_delay_us = 0,
            .hsyn_num = 0,
        },
    },
};

/**
  * @brief  spi read/write mode                                                 
  * @param  enable: false - write spi mode |  true - read spi mode
  * @retval None
  */
static void LCD_ReadMode(LCDC_HandleTypeDef *hlcdc, bool enable)
{
    if (HAL_LCDC_IS_SPI_IF(lcdc_int_cfg.lcd_itf))
    {
        if (enable)
        {
            HAL_LCDC_SetFreq(hlcdc, 2000000); //read mode min cycle 300ns
        }
        else
        {
            HAL_LCDC_SetFreq(hlcdc, lcdc_int_cfg.freq); //Restore normal frequency
        }
    }
}

/**
  * @brief  Power on the LCD.
  * @param  None
  * @retval None
  */
static void LCD_Drv_Init(LCDC_HandleTypeDef *hlcdc)
{
    uint8_t   parameter[32];
    
    /* 初始化RST引脚  */
    HAL_PIN_Set(PAD_PA00, GPIO_A0, PIN_NOPULL, 1);
    rt_pin_mode(LCD_RST_PIN, PIN_MODE_OUTPUT);
    
    /* Reset LCD - 使用专用RST引脚 */
    rt_pin_write(LCD_RST_PIN, PIN_LOW);
    HAL_Delay_us(20);
    rt_pin_write(LCD_RST_PIN, PIN_HIGH);
    
    LCD_WriteReg_More(hlcdc, 0x11, parameter, 1);
    LCD_DRIVER_DELAY_MS(120);

    LCD_WriteReg_More(hlcdc, 0xFE, parameter, 0); // internal reg enable
    LCD_WriteReg_More(hlcdc, 0xEF, parameter, 0); // internal reg enable

    parameter[0] = 0xC0;
    LCD_WriteReg_More(hlcdc, 0xB0, parameter, 1);

    parameter[0] = 0x80;
    LCD_WriteReg_More(hlcdc, 0xB1, parameter, 1);

    parameter[0] = 0x27;
    LCD_WriteReg_More(hlcdc, 0xB2, parameter, 1);

    parameter[0] = 0x13;
    LCD_WriteReg_More(hlcdc, 0xB3, parameter, 1);

    parameter[0] = 0x19;
    LCD_WriteReg_More(hlcdc, 0xB6, parameter, 1);

    parameter[0] = 0x05;
    LCD_WriteReg_More(hlcdc, 0xB7, parameter, 1);

    parameter[0] = 0xC8;
    LCD_WriteReg_More(hlcdc, 0xAC, parameter, 1);

    parameter[0] = 0x0F;
    LCD_WriteReg_More(hlcdc, 0xAB, parameter, 1);

    parameter[0] = 0x05;
    LCD_WriteReg_More(hlcdc, 0x3A, parameter, 1);

    parameter[0] = 0x04;
    LCD_WriteReg_More(hlcdc, 0xB4, parameter, 1);

    parameter[0] = 0x05;
    LCD_WriteReg_More(hlcdc, 0x3A, parameter, 1);

    parameter[0] = 0x08;
    LCD_WriteReg_More(hlcdc, 0xA8, parameter, 1);

    parameter[0] = 0x08;
    LCD_WriteReg_More(hlcdc, 0xB8, parameter, 1);

    parameter[0] = 0x02;
    LCD_WriteReg_More(hlcdc, 0xEA, parameter, 1);

    parameter[0] = 0x2A;
    LCD_WriteReg_More(hlcdc, 0xE8, parameter, 1);

    parameter[0] = 0x47;
    LCD_WriteReg_More(hlcdc, 0xE9, parameter, 1);

    parameter[0] = 0x5F;
    LCD_WriteReg_More(hlcdc, 0xE7, parameter, 1);

    parameter[0] = 0x21;
    LCD_WriteReg_More(hlcdc, 0xC6, parameter, 1);

    parameter[0] = 0x15;
    LCD_WriteReg_More(hlcdc, 0xC7, parameter, 1);

    parameter[0] = 0x1D;
    parameter[1] = 0x38;
    parameter[2] = 0x09;
    parameter[3] = 0x4D;
    parameter[4] = 0x92;
    parameter[5] = 0x2F;
    parameter[6] = 0x35;
    parameter[7] = 0x52;
    parameter[8] = 0x1E;
    parameter[9] = 0x0C;
    parameter[10] = 0x04;
    parameter[11] = 0x12;
    parameter[12] = 0x14;
    parameter[13] = 0x1F;
    LCD_WriteReg_More(hlcdc, 0xF0, parameter, 14);

    parameter[0] = 0x16;
    parameter[1] = 0x40;
    parameter[2] = 0x1C;
    parameter[3] = 0x54;
    parameter[4] = 0xA9;
    parameter[5] = 0x2D;
    parameter[6] = 0x2E;
    parameter[7] = 0x56;
    parameter[8] = 0x10;
    parameter[9] = 0x0D;
    parameter[10] = 0x0C;
    parameter[11] = 0x1A;
    parameter[12] = 0x14;
    parameter[13] = 0x1E;
    LCD_WriteReg_More(hlcdc, 0xF1, parameter, 14);

    parameter[0] = 0x00;
    parameter[1] = 0x00;
    parameter[2] = 0xFF;
    LCD_WriteReg_More(hlcdc, 0xF4, parameter, 3);

    parameter[0] = 0xFF;
    parameter[1] = 0xFF;
    LCD_WriteReg_More(hlcdc, 0xBA, parameter, 2);

    if(USE_HORIZONTAL==0)parameter[0] = 0x00;
    else if(USE_HORIZONTAL==1)parameter[0] = (0xC0);
    else if(USE_HORIZONTAL==2)parameter[0] = (0x60);
    else parameter[0] = (0xA0); 
    LCD_WriteReg_More(hlcdc, 0x36, parameter, 1);

    LCD_WriteReg_More(hlcdc, 0x11, parameter, 0); 
}

static void LCD_Init(LCDC_HandleTypeDef *hlcdc)
{
    // 初始化CS引脚 - 只初始化3个引脚
    for (int i = 0; i < LCD_SCREEN_NUM; i++) {
        HAL_PIN_Set(lcd_cs_pad_gpio[i].pad, lcd_cs_pad_gpio[i].gpio, PIN_NOPULL, 1);
        rt_pin_mode(lcd_cs_pins[i], PIN_MODE_OUTPUT);
        rt_pin_write(lcd_cs_pins[i], PIN_HIGH);
    }

    // 设置引脚的驱动能力 PAD_PA04为SCL和PAD_PA05为SDA
    HAL_PIN_Set_DS0(PAD_PA04, 1, 1);
    HAL_PIN_Set_DS0(PAD_PA05, 1, 1);
    HAL_PIN_Set_DS1(PAD_PA04, 1, 1);
    HAL_PIN_Set_DS1(PAD_PA05, 1, 1);

    memcpy(&hlcdc->Init, &lcdc_int_cfg, sizeof(LCDC_InitTypeDef));
    HAL_LCDC_Init(hlcdc);

    LCD_Drv_Init(hlcdc);
}

/**
  * @brief  Read LCD ID
  * @param  None
  * @retval LCD Register Value.
  */
static uint32_t LCD_ReadID(LCDC_HandleTypeDef *hlcdc)
{
    uint32_t lcd_data[LCD_SCREEN_NUM];

    // 遍历三个屏幕读取ID - 从6个改为3个
    for (int i = 0; i < LCD_SCREEN_NUM; i++) {
        current_lcd_cs_pin = lcd_cs_pins[i];
        lcd_data[i] = LCD_ReadData(hlcdc, REG_LCD_ID, 4);
        lcd_data[i] = ((lcd_data[i] << 1) >> 8) & 0xFFFFFF;
        rt_kprintf("\nLCD%d ReadID 0x%x \n", (i + 1), lcd_data[i]);        
    }
    return THE_LCD_ID;
}

/**
  * @brief  Enables the Display.
  * @param  None
  * @retval None
  */
static void LCD_DisplayOn(LCDC_HandleTypeDef *hlcdc)
{
    /* Display On */
    LCD_WriteReg_More(hlcdc, REG_DISPLAY_ON, (uint8_t *)NULL, 0);
}

/**
  * @brief  Disables the Display.
  * @param  None
  * @retval None
  */
static void LCD_DisplayOff(LCDC_HandleTypeDef *hlcdc)
{
    /* Display Off */
    LCD_WriteReg_More(hlcdc, REG_DISPLAY_OFF, (uint8_t *)NULL, 0);
}

/**
 * @brief Set LCD & LCDC clip area
 * @param hlcdc LCD controller handle
 * @param Xpos0 - Clip area left coordinate, base on LCD top-left, same as below.
 * @param Ypos0 - Clip area top coordinate
 * @param Xpos1 - Clip area right coordinate
 * @param Ypos1 - Clip area bottom coordinate
 */
static void LCD_SetRegion(LCDC_HandleTypeDef *hlcdc, uint16_t Xpos0, uint16_t Ypos0, uint16_t Xpos1, uint16_t Ypos1)
{
    Region_Xpos0 = Xpos0;
    Region_Ypos0 = Ypos0;
    Region_Xpos1 = Xpos1;
    Region_Ypos1 = Ypos1;
}

static bool LCDC_SetROIArea(LCDC_HandleTypeDef *hlcdc)
{
    uint8_t parameter[4];
    uint8_t col = 0, row = 0;
    bool found = false;
    
    // 查找当前CS引脚对应的屏幕位置
    for (size_t i = 0; i < sizeof(screen_map)/sizeof(screen_map[0]); i++) {
        if (screen_map[i].cs_pin == current_lcd_cs_pin) {
            col = screen_map[i].col;
            row = screen_map[i].row;
            found = true;
            break;
        }
    }
    if (!found) {
        rt_kprintf("Error: Invalid CS pin %d\n", current_lcd_cs_pin);
        return RT_FALSE;
    }

    // 计算当前屏幕的物理边界
    const uint16_t screen_x0 = col * THE_LCD_PIXEL_WIDTH;
    const uint16_t screen_y0 = row * THE_LCD_PIXEL_HEIGHT;
    const uint16_t screen_x1 = screen_x0 + THE_LCD_PIXEL_WIDTH - 1;
    const uint16_t screen_y1 = screen_y0 + THE_LCD_PIXEL_HEIGHT - 1;

    // 检查ROI是否与当前屏幕有交集
    if (Region_Xpos0 > screen_x1 || Region_Xpos1 < screen_x0 ||
        Region_Ypos0 > screen_y1 || Region_Ypos1 < screen_y0) 
    {
        return RT_FALSE;
    }

    // 计算ROI在当前屏幕内的局部坐标（使用MAX/MIN处理边界）
    const uint16_t local_x0 = MAX(Region_Xpos0, screen_x0) - screen_x0;
    const uint16_t local_y0 = MAX(Region_Ypos0, screen_y0) - screen_y0;
    const uint16_t local_x1 = MIN(Region_Xpos1, screen_x1) - screen_x0;
    const uint16_t local_y1 = MIN(Region_Ypos1, screen_y1) - screen_y0;

    // 验证局部坐标有效性
    if (local_x0 >= THE_LCD_PIXEL_WIDTH || local_y0 >= THE_LCD_PIXEL_HEIGHT ||
        local_x1 >= THE_LCD_PIXEL_WIDTH || local_y1 >= THE_LCD_PIXEL_HEIGHT ||
        local_x0 > local_x1 || local_y0 > local_y1) 
    {
        return RT_FALSE;
    }

    // 设置列地址 (CASET)
    parameter[0] = local_x0 >> 8;
    parameter[1] = local_x0 & 0xFF;
    parameter[2] = local_x1 >> 8;
    parameter[3] = local_x1 & 0xFF;
    LCD_WriteReg(hlcdc, REG_CASET, parameter, 4);

    // 设置行地址 (RASET)
    parameter[0] = local_y0 >> 8;
    parameter[1] = local_y0 & 0xFF;
    parameter[2] = local_y1 >> 8;
    parameter[3] = local_y1 & 0xFF;
    LCD_WriteReg(hlcdc, REG_RASET, parameter, 4);

    // 设置硬件ROI（使用全局坐标）
    HAL_LCDC_SetROIArea(hlcdc, 
        screen_x0 + local_x0, screen_y0 + local_y0,
        screen_x0 + local_x1, screen_y0 + local_y1
    );
              
    return RT_TRUE;
}

/**
  * @brief  Writes pixel.
  * @param  Xpos: specifies the X position.
  * @param  Ypos: specifies the Y position.
  * @param  RGBCode: the RGB pixel color
  * @retval None
  */
static void LCD_WritePixel(LCDC_HandleTypeDef *hlcdc, uint16_t Xpos, uint16_t Ypos, const uint8_t *RGBCode)
{
    uint8_t data = 0;
    /* Set Cursor */
    LCD_SetRegion(hlcdc, Xpos, Ypos, Xpos, Ypos);
    LCD_WriteReg(hlcdc, REG_WRITE_RAM, (uint8_t *)RGBCode, 2);
}

static void (* Ori_XferCpltCallback)(struct __LCDC_HandleTypeDef *lcdc);

static void HAL_GPIO_Set(uint16_t pin, int value)
{
    rt_pin_write(pin, (value != 0) ? PIN_HIGH : PIN_LOW);
}

static int current_screen_index = 0;

static void LCD_SendLayerDataCpltCbk(LCDC_HandleTypeDef *hlcdc)
{
    HAL_GPIO_Set(current_lcd_cs_pin, 1);
    current_screen_index++;
    while (current_screen_index < LCD_SCREEN_NUM) // 使用3个屏幕而不是6个
    {
        current_lcd_cs_pin = lcd_cs_pins[current_screen_index];
        if (LCDC_SetROIArea(hlcdc))
        {
            HAL_GPIO_Set(current_lcd_cs_pin, 0);
            hlcdc->XferCpltCallback = LCD_SendLayerDataCpltCbk;
            HAL_LCDC_SendLayerData2Reg_IT(hlcdc, REG_WRITE_RAM, 1);
            return;
        }
        current_screen_index++;
    }
    // 全部完成，恢复原始回调
    current_screen_index = 0;
    hlcdc->XferCpltCallback = Ori_XferCpltCallback;
    if (Ori_XferCpltCallback)
        Ori_XferCpltCallback(hlcdc);
}

static void LCD_WriteMultiplePixels(LCDC_HandleTypeDef *hlcdc, const uint8_t *RGBCode, uint16_t Xpos0, uint16_t Ypos0, uint16_t Xpos1, uint16_t Ypos1)
{
    HAL_LCDC_LayerSetData(hlcdc, HAL_LCDC_LAYER_DEFAULT, (uint8_t *)RGBCode, Xpos0, Ypos0, Xpos1, Ypos1);
    Ori_XferCpltCallback = hlcdc->XferCpltCallback;
    current_screen_index = 0;
    while (current_screen_index < LCD_SCREEN_NUM) // 使用3个屏幕而不是6个
    {
        current_lcd_cs_pin = lcd_cs_pins[current_screen_index];
        if (LCDC_SetROIArea(hlcdc))
        {
            HAL_GPIO_Set(current_lcd_cs_pin, 0);
            hlcdc->XferCpltCallback = LCD_SendLayerDataCpltCbk;
            rt_base_t level = rt_hw_interrupt_disable(); //禁用中断
            HAL_LCDC_SendLayerData2Reg_IT(hlcdc, REG_WRITE_RAM, 1);
            rt_hw_interrupt_enable(level); //恢复中断
            return;
        }
        current_screen_index++;
    }
    // 全部失败
    if (Ori_XferCpltCallback)
        Ori_XferCpltCallback(hlcdc);
}

/**
  * @brief  Writes  to the selected LCD register.
  * @param  LCD_Reg: address of the selected register.
  * @retval None
  */
static void LCD_WriteReg(LCDC_HandleTypeDef *hlcdc, uint16_t LCD_Reg, uint8_t *Parameters, uint32_t NbParameters)
{
    HAL_GPIO_Set(current_lcd_cs_pin, 0);
    HAL_LCDC_WriteU8Reg(hlcdc, LCD_Reg, Parameters, NbParameters);
    HAL_GPIO_Set(current_lcd_cs_pin, 1);
}

static void LCD_WriteReg_More(LCDC_HandleTypeDef *hlcdc, uint16_t LCD_Reg, uint8_t *Parameters, uint32_t NbParameters)
{
    // 全部CS拉低 - 只操作3个引脚
    for (int i = 0; i < LCD_SCREEN_NUM; i++) {
        HAL_GPIO_Set(lcd_cs_pins[i], 0);
    }
    HAL_LCDC_WriteU8Reg(hlcdc, LCD_Reg, Parameters, NbParameters);
    // 全部CS拉高 - 只操作3个引脚
    for (int i = 0; i < LCD_SCREEN_NUM; i++) {
        HAL_GPIO_Set(lcd_cs_pins[i], 1);
    }
}

/**
  * @brief  Reads the selected LCD Register.
  * @param  RegValue: Address of the register to read
  * @param  ReadSize: Number of bytes to read
  * @retval LCD Register Value.
  */
static uint32_t LCD_ReadData(LCDC_HandleTypeDef *hlcdc, uint16_t RegValue, uint8_t ReadSize)
{
    uint32_t rd_data = 0;
    HAL_GPIO_Set(current_lcd_cs_pin, 0);
    LCD_ReadMode(hlcdc, true);

    HAL_LCDC_ReadU8Reg(hlcdc, RegValue, (uint8_t *)&rd_data, ReadSize);

    LCD_ReadMode(hlcdc, false);
    HAL_GPIO_Set(current_lcd_cs_pin, 1);
    
    return rd_data;
}

static uint32_t LCD_ReadPixel(LCDC_HandleTypeDef *hlcdc, uint16_t Xpos, uint16_t Ypos)
{
    uint8_t   parameter[2];
    uint32_t c;
    uint32_t ret_v;

    parameter[0] = 0x55;
    LCD_WriteReg(hlcdc, REG_COLOR_MODE, parameter, 1);

    LCD_SetRegion(hlcdc, Xpos, Ypos, Xpos, Ypos);
    c =  LCD_ReadData(hlcdc, REG_READ_RAM, 4);
    c >>= lcdc_int_cfg.cfg.spi.dummy_clock; //revert fixed dummy cycle

    switch (lcdc_int_cfg.color_mode)
    {
    case LCDC_PIXEL_FORMAT_RGB565:
        parameter[0] = 0x55;
        ret_v = (uint32_t)(((c >> 8) & 0xF800) | ((c >> 5) & 0x7E0) | ((c >> 3) & 0X1F));
        break;

    case LCDC_PIXEL_FORMAT_RGB666:
        parameter[0] = 0x66;
        ret_v = (uint32_t)(((c >> 6) & 0x3F000) | ((c >> 4) & 0xFC0) | ((c >> 2) & 0X3F));
        break;

    case LCDC_PIXEL_FORMAT_RGB888:
        parameter[0] = 0x66;
        ret_v = (uint32_t)((c & 0xFCFCFC) | ((c >> 6) & 0x030303));
        break;

    default:
        RT_ASSERT(0);
        break;
    }
    LCD_WriteReg(hlcdc, REG_COLOR_MODE, parameter, 1);

    return ret_v;
}

static void LCD_SetColorMode(LCDC_HandleTypeDef *hlcdc, uint16_t color_mode)
{
    uint8_t   parameter[2];

    switch (color_mode)
    {
    case RTGRAPHIC_PIXEL_FORMAT_RGB565:
        /* Color mode 16bits/pixel */
        parameter[0] = 0x55;
        lcdc_int_cfg.color_mode = LCDC_PIXEL_FORMAT_RGB565;
        break;

    case RTGRAPHIC_PIXEL_FORMAT_RGB888:
        /* Color mode 18bits/pixel */
        parameter[0] = 0x66;
        lcdc_int_cfg.color_mode = LCDC_PIXEL_FORMAT_RGB888;
        break;

    default:
        RT_ASSERT(0);
        return; //unsupport
        break;
    }

    LCD_WriteReg_More(hlcdc, REG_COLOR_MODE, parameter, 1);
    HAL_LCDC_SetOutFormat(hlcdc, lcdc_int_cfg.color_mode);
}

static void LCD_SetBrightness(LCDC_HandleTypeDef *hlcdc, uint8_t br)
{
    uint8_t bright = (uint8_t)((int)UINT8_MAX * br / 100);
    LCD_WriteReg_More(hlcdc, REG_WBRIGHT, &br, 1);
}

static const LCD_DrvOpsDef GC9107_drv =
{
    LCD_Init,
    LCD_ReadID,
    LCD_DisplayOn,
    LCD_DisplayOff,
    LCD_SetRegion,
    LCD_WritePixel,
    LCD_WriteMultiplePixels,
    LCD_ReadPixel,
    LCD_SetColorMode,
    LCD_SetBrightness,
    NULL,
    NULL
};

LCD_DRIVER_EXPORT2(GC9107, THE_LCD_ID, &lcdc_int_cfg,
                   &GC9107_drv, 1);

/************************ (C) COPYRIGHT Sifli Technology *******END OF FILE****/