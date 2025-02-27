/**
 * @file lvgl_spi_conf.h
 *
 */

#ifndef LVGL_SPI_CONF_H
#define LVGL_SPI_CONF_H

#include <sdkconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/
// DISPLAY PINS

/* Mandatory pins are MOSI and CLK */
#define DISP_SPI_MOSI CONFIG_LV_DISP_SPI_MOSI
#define DISP_SPI_CLK CONFIG_LV_DISP_SPI_CLK

/* Optional pins */
#if defined (CONFIG_LV_DISPLAY_USE_SPI_MISO)
#define DISP_SPI_MISO CONFIG_LV_DISP_SPI_MISO
#define DISP_SPI_INPUT_DELAY_NS CONFIG_LV_DISP_SPI_INPUT_DELAY_NS
#else
#define DISP_SPI_MISO (-1)
#define DISP_SPI_INPUT_DELAY_NS (0U)
#endif

#if defined(CONFIG_LV_DISP_SPI_IO2)
#define DISP_SPI_IO2 CONFIG_LV_DISP_SPI_IO2
#else
#define DISP_SPI_IO2 (-1)
#endif

#if defined(CONFIG_LV_DISP_SPI_IO3)
#define DISP_SPI_IO3 CONFIG_LV_DISP_SPI_IO3
#else
#define DISP_SPI_IO3 (-1)
#endif

#if defined (CONFIG_LV_DISPLAY_USE_SPI_CS)
    #define DISP_SPI_CS CONFIG_LV_DISP_SPI_CS
#else
    #define DISP_SPI_CS (-1)
#endif

/* Define TOUCHPAD PINS when selecting a touch controller */
#if !defined (CONFIG_LV_TOUCH_CONTROLLER_NONE)

/* Handle FT81X special case */
#if defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_FT81X) && \
    defined (CONFIG_LV_TOUCH_CONTROLLER_FT81X)

#define SHARED_SPI_BUS

#define TP_SPI_MOSI CONFIG_LV_DISP_SPI_MOSI
#define TP_SPI_MISO CONFIG_LV_DISP_SPI_MISO
#define TP_SPI_CLK  CONFIG_LV_DISP_SPI_CLK
#define TP_SPI_CS   CONFIG_LV_DISP_SPI_CS
#else
#define TP_SPI_MOSI CONFIG_LV_TOUCH_SPI_MOSI
#define TP_SPI_MISO CONFIG_LV_TOUCH_SPI_MISO
#define TP_SPI_CLK  CONFIG_LV_TOUCH_SPI_CLK
#define TP_SPI_CS   CONFIG_LV_TOUCH_SPI_CS
#endif
#endif

#define ENABLE_TOUCH_INPUT  CONFIG_LV_ENABLE_TOUCH

#if defined (CONFIG_LV_TFT_DISPLAY_PROTOCOL_SPI)
/* Display controller SPI host configuration */
#if defined (CONFIG_LV_TFT_DISPLAY_SPI2_HOST)
#define TFT_SPI_HOST SPI2_HOST
#elif defined (CONFIG_LV_TFT_DISPLAY_SPI3_HOST)
#define TFT_SPI_HOST SPI3_HOST
#else
#error SPI host not defined
#endif
#endif

/* Touch controller SPI host configuration */
#if defined (CONFIG_LV_TOUCH_CONTROLLER_SPI2_HOST)
#define TOUCH_SPI_HOST SPI2_HOST
#elif defined (CONFIG_LV_TOUCH_CONTROLLER_SPI3_HOST)
#define TOUCH_SPI_HOST SPI3_HOST
#endif

#if defined (CONFIG_LV_TFT_DISPLAY_SPI_HALF_DUPLEX)
#define DISP_SPI_HALF_DUPLEX
#else
#define DISP_SPI_FULL_DUPLEX
#endif

#if defined (CONFIG_LV_TFT_DISPLAY_SPI_TRANS_MODE_DIO)
#define DISP_SPI_TRANS_MODE_DIO
#elif defined (CONFIG_LV_TFT_DISPLAY_SPI_TRANS_MODE_QIO)
#define DISP_SPI_TRANS_MODE_QIO
#else
#define DISP_SPI_TRANS_MODE_SIO
#endif

/* Detect usage of shared SPI bus between display and indev controllers
 *
 * If the user sets the same MOSI and CLK pins for both display and indev
 * controllers then we can assume the user is using the same SPI bus
 * If so verify the user specified the same SPI bus for both */
#if !defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_FT81X)

#if defined (CONFIG_LV_TFT_DISPLAY_PROTOCOL_SPI) && \
            (CONFIG_LV_TFT_DISPLAY_PROTOCOL_SPI == 1) && \
    defined (CONFIG_LV_TOUCH_DRIVER_PROTOCOL_SPI) && \
            (TP_SPI_MOSI == DISP_SPI_MOSI) && (TP_SPI_CLK == DISP_SPI_CLK)

#if TFT_SPI_HOST != TOUCH_SPI_HOST
#error You must specify the same SPI host (SPIx_HOST) for both display and touch driver
#else
#define SHARED_SPI_BUS
#endif

#endif
#endif

/**********************
 *      TYPEDEFS
 **********************/
#if defined (CONFIG_LV_TFT_USE_CUSTOM_SPI_CLK_DIVIDER)
#define SPI_TFT_CLOCK_SPEED_HZ ((80 * 1000 * 1000) / CONFIG_LV_TFT_CUSTOM_SPI_CLK_DIVIDER)
#else
#if defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7789)
#define SPI_TFT_CLOCK_SPEED_HZ  (20*1000*1000)
#elif defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7735S)
#define SPI_TFT_CLOCK_SPEED_HZ  (40*1000*1000)
#elif defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_HX8357)
#define SPI_TFT_CLOCK_SPEED_HZ  (26*1000*1000)
#elif defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_SH1107)
#define SPI_TFT_CLOCK_SPEED_HZ  (8*1000*1000)
#elif defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9481)
#define SPI_TFT_CLOCK_SPEED_HZ  (16*1000*1000)
#elif defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9486)
#define SPI_TFT_CLOCK_SPEED_HZ  (20*1000*1000)
#elif defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9488)
#define SPI_TFT_CLOCK_SPEED_HZ  (40*1000*1000)
#elif defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9341)
#define SPI_TFT_CLOCK_SPEED_HZ  (40*1000*1000)
#elif defined(CONFIG_LV_TFT_DISPLAY_CONTROLLER_ILI9163C)
#define SPI_TFT_CLOCK_SPEED_HZ  (40*1000*1000)
#elif defined(CONFIG_LV_TFT_DISPLAY_CONTROLLER_FT81X)
#define SPI_TFT_CLOCK_SPEED_HZ  (32*1000*1000)
#elif defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_PCD8544)
#define SPI_TFT_CLOCK_SPEED_HZ  (4*1000*1000)
#else
#define SPI_TFT_CLOCK_SPEED_HZ  (40*1000*1000)
#endif

#endif /* CONFIG_LV_TFT_USE_CUSTOM_SPI_CLK_DIVIDER */

#if defined (CONFIG_LV_TFT_DISPLAY_USE_CUSTOM_SPI_MODE)
#define SPI_TFT_SPI_MODE    (CONFIG_LV_TFT_DISPLAY_CUSTOM_SPI_MODE)
#else
#if defined (CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7789)
#define SPI_TFT_SPI_MODE    (2U)
#else
#define SPI_TFT_SPI_MODE    (0U)
#endif
#endif

/* Touch driver */
#if (CONFIG_LV_TOUCH_CONTROLLER == TOUCH_CONTROLLER_STMPE610)
#define SPI_TOUCH_CLOCK_SPEED_HZ    (1*1000*1000)
#define SPI_TOUCH_SPI_MODE          (1U)
#else
#define SPI_TOUCH_CLOCK_SPEED_HZ    (2*1000*1000)
#define SPI_TOUCH_SPI_MODE          (0U)
#endif

/**********************
 * GLOBAL PROTOTYPES
 **********************/


/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LVGL_SPI_CONF_H*/
