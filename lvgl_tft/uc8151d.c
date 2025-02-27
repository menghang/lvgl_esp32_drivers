/**
    @file uc8151d.c
    @brief   GoodDisplay GDEW0154M10 DES e-paper display w/ UltraChip UC8151D
    @version 1.0
    @date    2020-08-28
    @author  Jackson Ming Hu <huming2207@gmail.com>


    @section LICENSE

    MIT License

    Copyright (c) 2020 Jackson Ming Hu

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute,
    sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <driver/gpio.h>

#include "disp_spi.h"
#include "disp_driver.h"
#include "uc8151d.h"

#define PIN_DC              CONFIG_LV_DISP_PIN_DC
#define PIN_DC_BIT          ((1ULL << (uint8_t)(CONFIG_LV_DISP_PIN_DC)))

#if defined CONFIG_LV_DISP_PIN_RST
#define PIN_RST             CONFIG_LV_DISP_PIN_RST
#define PIN_RST_BIT         ((1ULL << (uint8_t)(CONFIG_LV_DISP_PIN_RST)))
#endif

#define PIN_BUSY            CONFIG_LV_DISP_PIN_BUSY
#define PIN_BUSY_BIT        ((1ULL << (uint8_t)(CONFIG_LV_DISP_PIN_BUSY)))
#define EVT_BUSY            (1UL << 0UL)

#if defined (LV_HOR_RES_MAX)
#define EPD_WIDTH           LV_HOR_RES_MAX
#else
    /* ToDo Fix, 256 is just a magic number */
#define EPD_WIDTH           256u
#endif

#if defined (LV_VER_RES_MAX)
#define EPD_HEIGHT          LV_VER_RES_MAX
#else
    /* ToDo Fix, 128 is just a magic number */
#define EPD_HEIGHT          128u
#endif

#define EPD_ROW_LEN         (EPD_HEIGHT / 8u)

#define BIT_SET(a, b)       ((a) |= (1U << (b)))
#define BIT_CLEAR(a, b)     ((a) &= ~(1U << (b)))

typedef struct
{
    uint8_t cmd;
    uint8_t data[3];
    size_t len;
} uc8151d_seq_t;

#define EPD_SEQ_LEN(x) ((sizeof(x) / sizeof(uc8151d_seq_t)))

static EventGroupHandle_t uc8151d_evts = NULL;

static void IRAM_ATTR uc8151d_busy_intr(void *arg)
{
    BaseType_t xResult;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xResult = xEventGroupSetBitsFromISR(uc8151d_evts, EVT_BUSY, &xHigherPriorityTaskWoken);
    if (xResult == pdPASS) {
        portYIELD_FROM_ISR();
    }
}

static void uc8151d_spi_send_cmd(uint8_t cmd)
{
    disp_wait_for_pending_transactions();
    gpio_set_level(PIN_DC, 0);     // DC = 0 for command
    disp_spi_send_data(&cmd, 1);
}

static void uc8151d_spi_send_data(uint8_t *data, size_t len)
{
    disp_wait_for_pending_transactions();
    gpio_set_level(PIN_DC, 1);  // DC = 1 for data
    disp_spi_send_data(data, len);
}

static void uc8151d_spi_send_data_byte(uint8_t data)
{
    disp_wait_for_pending_transactions();
    gpio_set_level(PIN_DC, 1);  // DC = 1 for data
    disp_spi_send_data(&data, 1);
}

static esp_err_t uc8151d_wait_busy(uint32_t timeout_ms)
{
    uint32_t wait_ticks = (timeout_ms == 0 ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms));
    EventBits_t bits = xEventGroupWaitBits(uc8151d_evts,
                                           EVT_BUSY, // Wait for busy bit
                                           pdTRUE, pdTRUE,       // Clear on exit, wait for all
                                           wait_ticks);         // Timeout

    return ((bits & EVT_BUSY) != 0) ? ESP_OK : ESP_ERR_TIMEOUT;
}

static void uc8151d_sleep(void)
{
    // Set VCOM to 0xf7
    uc8151d_spi_send_cmd(0x50);
    uc8151d_spi_send_data_byte(0xf7);

    // Power off
    uc8151d_spi_send_cmd(0x02);
    uc8151d_wait_busy(0);

    // Go to sleep
    uc8151d_spi_send_cmd(0x07);
    uc8151d_spi_send_data_byte(0xa5);
}

static void uc8151d_reset(void);

static void uc8151d_panel_init(void)
{
    // Hardware reset for 3 times - not sure why but it's from official demo code
    for (uint8_t cnt = 0; cnt < 3; cnt++) {
        uc8151d_reset();
    }

    // Power up
    uc8151d_spi_send_cmd(0x04);
    uc8151d_wait_busy(0);

    // Panel settings
    uc8151d_spi_send_cmd(0x00);
#if defined (CONFIG_LV_DISPLAY_ORIENTATION_PORTRAIT_INVERTED)
    uc8151d_spi_send_data_byte(0x13);
#elif defined (CONFIG_LV_DISPLAY_ORIENTATION_PORTRAIT)
    uc8151d_spi_send_data_byte(0x1f);
#endif

    // VCOM & Data intervals
    uc8151d_spi_send_cmd(0x50);
    uc8151d_spi_send_data_byte(0x97);
}

static void uc8151d_full_update(uint8_t *buf)
{
    uc8151d_panel_init();

    uint8_t *buf_ptr = buf;
    uint8_t old_data[EPD_ROW_LEN] = { 0 };

    // Fill old data
    uc8151d_spi_send_cmd(0x10);
    for (size_t h_idx = 0; h_idx < EPD_HEIGHT; h_idx++) {
        uc8151d_spi_send_data(old_data, EPD_ROW_LEN);
    }

    // Fill new data
    uc8151d_spi_send_cmd(0x13);
    for (size_t h_idx = 0; h_idx < EPD_HEIGHT; h_idx++) {
        uc8151d_spi_send_data(buf_ptr, EPD_ROW_LEN);
        buf_ptr += EPD_ROW_LEN;
    }

    // Issue refresh
    uc8151d_spi_send_cmd(0x12);
    vTaskDelay(pdMS_TO_TICKS(10));
    uc8151d_wait_busy(0);

    uc8151d_sleep();
}

void uc8151d_lv_fb_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
#if LV_USE_LOG
    size_t len = ((area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1)) / 8;
    LV_LOG_INFO("x1: 0x%x, x2: 0x%x, y1: 0x%x, y2: 0x%x", area->x1, area->x2, area->y1, area->y2);
    LV_LOG_INFO("Writing LVGL fb with len: %u", len);
#endif

    uint8_t *buf = (uint8_t *) color_map;
    uc8151d_full_update(buf);

    lv_disp_flush_ready(drv);
    LV_LOG_INFO("Ready");
}

void uc8151d_lv_set_fb_cb(lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
                           lv_color_t color, lv_opa_t opa)
{
    uint16_t byte_index = (x >> 3u) + (y * EPD_ROW_LEN);
    uint8_t bit_index = x & 0x07u;

    if (color.full) {
        BIT_SET(buf[byte_index], 7 - bit_index);
    } else {
        LV_LOG_INFO("Clear at x: %u, y: %u", x, y);
        BIT_CLEAR(buf[byte_index], 7 - bit_index);
    }
}

void uc8151d_lv_rounder_cb(lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    // Always send full framebuffer if it's not in partial mode
    area->x1 = 0;
    area->y1 = 0;
    area->x2 = EPD_WIDTH - 1;
    area->y2 = EPD_HEIGHT - 1;
}

void uc8151d_init(void)
{
    // Initialise event group
    uc8151d_evts = xEventGroupCreate();
    if (!uc8151d_evts) {
        LV_LOG_ERROR("Failed when initialising event group!");
        return;
    }

    // Setup input pin, pull-up, input
    gpio_config_t in_io_conf = {
            .intr_type = GPIO_INTR_POSEDGE,
            .mode = GPIO_MODE_INPUT,
            .pin_bit_mask = PIN_BUSY_BIT,
            .pull_down_en = 0,
            .pull_up_en = 1,
    };
    ESP_ERROR_CHECK(gpio_config(&in_io_conf));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_BUSY, uc8151d_busy_intr, (void *) PIN_BUSY);

    LV_LOG_INFO("IO init finished");
    uc8151d_panel_init();
    LV_LOG_INFO("Panel initialised");
}

static void uc8151d_reset(void)
{
#if defined CONFIG_LV_DISP_USE_RST
    gpio_set_level(PIN_RST, 0);
    // At least 10ms, leave 20ms for now just in case...
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
#endif
}
