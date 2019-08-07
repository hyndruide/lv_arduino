#include <lvgl.h>
#include <Ticker.h>
#include <tft.h>
#include <SPI.h>

#define LVGL_TICK_PERIOD 20

#define TFT_CS        22    // do not use GPI032 or GPIO33 here
#define TFT_DC        21    // do not use GPI032 or GPIO33 here
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18
#define TP_IRQ         5
#define TP_CS         15


Ticker tick; /* timer for interrupt handler */
TFT tft(1); /* TFT instance */
TP tp(TP_CS, TP_IRQ);

uint16_t tp_x, tp_y;
bool released = true;

static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];

#if USE_LV_LOG != 0
/* Serial debugging */
void my_print(lv_log_level_t level, const char * file, uint32_t line, const char * dsc)
{

  Serial.printf("%s@%d->%s\r\n", file, line, dsc);
  delay(100);
}
#endif


/*
void TFT::writeColor(uint16_t color, uint32_t len){
    static uint16_t temp[TFT_MAX_PIXELS_AT_ONCE];
    size_t blen = (len > TFT_MAX_PIXELS_AT_ONCE)?TFT_MAX_PIXELS_AT_ONCE:len;
    uint16_t tlen = 0;

    for (uint32_t t=0; t<blen; t++){
        temp[t] = color;
    }

    while(len){
        tlen = (len>blen)?blen:len;
        writePixels(temp, tlen);
        len -= tlen;
    }
}
*/

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  static uint16_t temp[32];
  uint16_t c,old_c;
  int i=0;

  tft.startWrite(); /* Start new TFT transaction */
  tft.setAddrWindow(area->x1, area->y1, (area->x2 - area->x1 + 1), (area->y2 - area->y1 + 1)); /* set the working window */
  tft.writeCommand(0x22);
  for (int y = area->y1; y <= area->y2; y++) {
    for (int x = area->x1; x <= area->x2; x++) {
       temp[i] = color_p->full;
       i++;
       if (i==32){   
       tft.writePixels(temp, i);
       i = 0;
       }
      color_p++;
    }
  }
  tft.endWrite(); /* terminate TFT transaction */
  lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

/* Interrupt driven periodic handler */
static void lv_tick_handler(void) 
{

  lv_tick_inc(LVGL_TICK_PERIOD);
}


void tp_pressed(uint16_t x, uint16_t y){
    tp_x=x;  tp_y=y;
    released = false;
}
void tp_released(){
released = true;
}


bool my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data)
{
    data->point.x = tp_x; 
    data->point.y = tp_y;
    data->state = released?LV_INDEV_STATE_REL:LV_INDEV_STATE_PR;
    return false; /*No buffering now so no more data read*/
}



void btn_event_cb(lv_obj_t * btn, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED) {
        printf("Clicked\n");
    }
}

void setup() {

  Serial.begin(115200); /* prepare for possible serial debug */

  lv_init();

#if USE_LV_LOG != 0
  lv_log_register_print(my_print); /* register print function for debugging */
#endif
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  tft.begin(); /* TFT init */
  tft.begin(TFT_CS, TFT_DC, SPI_MOSI, SPI_MISO, SPI_SCK);
  tft.setRotation(1); /* Landscape orientation */
  tp.setRotation(1);
  lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);

  /*Initialize the display*/
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 320;
  disp_drv.ver_res = 240;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);


  /*Initialize the touch pad*/
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_input_read;
  lv_indev_drv_register(&indev_drv);

  /*Initialize the graphics library's tick*/
  tick.attach_ms(LVGL_TICK_PERIOD, lv_tick_handler);

  /* Create simple label */
  lv_obj_t *label = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(label, "Hello ESP32! (V6.0)");
  lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t * btn = lv_btn_create(lv_scr_act(), NULL);     /*Add a button the current screen*/
  lv_obj_set_pos(btn, 10, 10);                            /*Set its position*/
  lv_obj_set_size(btn, 100, 50);                          /*Set its size*/
  lv_obj_set_event_cb(btn, btn_event_cb);                 /*Assign a callback to the button*/
  
  lv_obj_t * labelb = lv_label_create(btn, NULL);          /*Add a label to the button*/
  lv_label_set_text(labelb, "Button");                     /*Set the labels text*/
}


void loop() {
  tp.loop();
  lv_task_handler(); /* let the GUI do its work */
 delay(5);
}
