/*
 * @Descripttion :  
 * @version      :  
 * @Author       : Kevincoooool
 * @Date         : 2021-09-04 16:11:59
 * @LastEditors  : Kevincoooool
 * @LastEditTime : 2021-09-04 18:05:01
 * @FilePath     : \lvgl_v8\main\main.c
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lv_examples/src/lv_demo_widgets/lv_demo_widgets.h"
#include "lv_examples/src/lv_demo_keypad_encoder/lv_demo_keypad_encoder.h"
#include "lv_examples/src/lv_demo_benchmark/lv_demo_benchmark.h"
#include "lv_examples/src/lv_demo_stress/lv_demo_stress.h"
#include "lv_examples/src/lv_demo_music/lv_demo_music.h"
#include "lvgl_helpers.h"
#include "esp_freertos_hooks.h"
static void lv_tick_task(void *arg)
{
   (void)arg;
   lv_tick_inc(portTICK_PERIOD_MS);
}
SemaphoreHandle_t xGuiSemaphore;

static void create_demo_application(void);

static void gui_task(void *arg)
{
   xGuiSemaphore = xSemaphoreCreateMutex();
   lv_init();          //lvgl内核初始化
   lvgl_driver_init(); //lvgl显示接口初始化

   /* Example for 1) */
   static lv_disp_draw_buf_t draw_buf;
   // 配置刷屏用的双缓存区大小，一般给分配的空间为 (CONFIG_LV_HOR_RES_MAX * 40~80)
   // DISP_BUF_SIZE 可通过menuconfig中的 CONFIG_CUSTOM_DISPLAY_BUFFER_SIZE 来配置。如不自定义，则默认为 (LV_HOR_RES_MAX * 40)
   lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
   lv_color_t *buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);

   lv_disp_draw_buf_init(&draw_buf, buf1, buf2, DISP_BUF_SIZE); /*Initialize the display buffer*/

   static lv_disp_drv_t disp_drv;         /*A variable to hold the drivers. Must be static or global.*/
   lv_disp_drv_init(&disp_drv);           /*Basic initialization*/
   disp_drv.draw_buf = &draw_buf;         /*Set an initialized buffer*/
   disp_drv.flush_cb = disp_driver_flush; /*Set a flush callback to draw to the display*/
   disp_drv.hor_res = CONFIG_LV_HOR_RES_MAX;                /*Set the horizontal resolution in pixels*/
   disp_drv.ver_res = CONFIG_LV_VER_RES_MAX;                /*Set the vertical resolution in pixels*/
   lv_disp_drv_register(&disp_drv);       /*Register the driver and save the created display objects*/

   /* Register an input device when enabled on the menuconfig */
#if CONFIG_LV_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
   lv_indev_drv_t indev_drv;
   lv_indev_drv_init(&indev_drv);
   indev_drv.read_cb = touch_driver_read;
   indev_drv.type = LV_INDEV_TYPE_POINTER;
   lv_indev_drv_register(&indev_drv);
#endif

   esp_register_freertos_tick_hook(lv_tick_task);

   // lv_demo_widgets();
   // lv_demo_music();
   // lv_demo_benchmark();
   /* Create the demo application */
   create_demo_application(); // 运行个人Demo时，请将这句替换

   while (1)
   {
      /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
      vTaskDelay(pdMS_TO_TICKS(10));

      /* Try to take the semaphore, call lvgl related function on success */
      if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
      {
         lv_timer_handler();
         xSemaphoreGive(xGuiSemaphore);
      }
   }
}

// 由menuconfig中的 LV_EX_CHOICE 配置项，来选择运行Demo列表中的哪个Demo。共有5个可选择。
static void create_demo_application(void)
{
   /* When using a monochrome display we only show "Hello World" centered on the
   * screen */
#if defined CONFIG_LV_TFT_DISPLAY_MONOCHROME || \
    defined CONFIG_LV_TFT_DISPLAY_CONTROLLER_ST7735S

   /* use a pretty small demo for monochrome displays */
   /* Get the current screen  */
   lv_obj_t * scr = lv_disp_get_scr_act(NULL);

   /*Create a Label on the currently active screen*/
   lv_obj_t * label1 =  lv_label_create(scr, NULL);

   /*Modify the Label's text*/
   lv_label_set_text(label1, "Hello\nworld");

   /* Align the Label to the center
   * NULL means align on parent (which is the screen now)
   * 0, 0 at the end means an x, y offset after alignment*/
   lv_obj_align(label1, NULL, LV_ALIGN_CENTER, 0, 0);
#else
   /* Otherwise we show the selected demo */

   #if defined CONFIG_LV_USE_DEMO_WIDGETS
      lv_demo_widgets();
   #elif defined CONFIG_LV_DEMO_WIDGETS_SLIDESHOW
      lv_demo_widgets();//不明自动滚动的Demo在哪儿
   #elif defined CONFIG_LV_USE_DEMO_KEYPAD_AND_ENCODER
      lv_demo_keypad_encoder();
   #elif defined CONFIG_LV_USE_DEMO_BENCHMARK
      lv_demo_benchmark();
   #elif defined CONFIG_LV_USE_DEMO_STRESS
      lv_demo_stress();
   #elif defined CONFIG_LV_USE_DEMO_MUSIC
      lv_demo_music();
   #else
      #error "No demo application selected."
   #endif
#endif
}

void app_main(void)
{

   xTaskCreatePinnedToCore(gui_task, "gui task", 1024 * 4, NULL, 1, NULL, 0);
}
