//ESP32-S3 Seeed XIAO ESP32S3 or Supprmini clones

//ESP32 board package 3.0.5

#include "driver/touch_sensor.h"
#include "driver/touch_pad.h"
#include "soc/touch_sensor_channel.h"

#define OUTPUT_PIN  1   //output to printer, low is triggered state.
#define TOUCH_PIN   7 
#define TOUCH_CH TOUCH_PAD_GPIO7_CHANNEL

#define RGB_BRIGHTNESS 10 // Change white brightness (max 255)
#define RGB_BUILTIN 21

int touchValue;
int ctr=0;

uint64_t lastWrite=0;
uint64_t lastLatch=0;

uint32_t filt=0;
uint32_t bnch=0;

bool triggered = false;


//The threshold value is not absolute but relative to the "benchmark" value
//The benchmark value is a baseline that tracks the current filtered value
//slowly (unless triggered).  Text output prints this difference for calibration
//
touch_value_t threshold = 4000;

void setup(){
  Serial.begin(115200);

  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, HIGH);

  //Flash RGB on boot
  neopixelWrite(RGB_BUILTIN,RGB_BRIGHTNESS,RGB_BRIGHTNESS,RGB_BRIGHTNESS); // White
  delay(200);
  neopixelWrite(RGB_BUILTIN,RGB_BRIGHTNESS,0,0); // Red
  delay(200);
  neopixelWrite(RGB_BUILTIN,0,RGB_BRIGHTNESS,0); // Green
  delay(200);
  neopixelWrite(RGB_BUILTIN,0,0,RGB_BRIGHTNESS); // Blue
  delay(200);
  
  //touchRead here is arduino function to perform initialization
  touchRead(TOUCH_PIN);

  //Defaults seem to be 0x1000 and 0x1000 for ESP32
  //ESP32S3 TOUCH_PAD_SLEEP_CYCLE_DEFAULT & TOUCH_PAD_MEASURE_CYCLE_DEFAULT
  //https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-touch.h
  
  //#define TOUCH_PAD_SLEEP_CYCLE_DEFAULT   (0xf)   /*!<The number of sleep cycle in each measure process of touch channels.
  //#define TOUCH_PAD_MEASURE_CYCLE_DEFAULT (500)   /*!<The times of charge and discharge in each measure process of touch channels.
  //500, 0xf appears to be default for ESP32-S3
  //setup for quicker read with no sleep between readings, this is the arduino function, the ESP-IDF call has params swapped
  touchSetCycles(0x40,0x0);  

  //Setup hardware filter for touch trigger threshold
  //ESP-IDF calls to setup hardware filter, polling and filtering in loop was too slow 
  touch_filter_config_t tfc;
  tfc.mode=TOUCH_PAD_FILTER_IIR_256;
  tfc.debounce_cnt=3;
  tfc.noise_thr=3;
  tfc.jitter_step=0;
  tfc.smh_lvl=TOUCH_PAD_SMOOTH_IIR_8;
  touch_pad_filter_set_config(&tfc);
  touch_pad_filter_enable();  
  
  //Interrupt is used to reset output pin
  touchAttachInterrupt(TOUCH_PIN, touchTriggered, threshold);  

}

//ISR for touch detected
void touchTriggered() {
  digitalWrite(OUTPUT_PIN, LOW);
}



void loop(){
  // touchValue=touchRead(TOUCH_PIN);
  touch_pad_filter_read_smooth(TOUCH_CH, &filt);
  touch_pad_read_benchmark(TOUCH_CH,&bnch);
  ctr++;

  if(millis() - lastWrite > 10) {
    Serial.printf("0,%d,%d,10000\n",ctr, filt-bnch); //print 0 and 10000 to keep scale on plot consistent
    lastWrite=millis();
    ctr=0;  //track loop time
  }

  //set last latch when pad is triggered
  //Only update LED when state changes
  if(touchInterruptGetLastStatus(TOUCH_PIN)) {
    lastLatch=millis();
	if(!triggered) {
		digitalWrite(OUTPUT_PIN, LOW);
		neopixelWrite(RGB_BUILTIN,RGB_BRIGHTNESS,0,0); // Red
		triggered = true;
	}
  }

  //reset the output pin after 20 ms
  if((triggered && millis() - lastLatch) > 20){
    digitalWrite(OUTPUT_PIN, HIGH);  
    neopixelWrite(RGB_BUILTIN,0,RGB_BRIGHTNESS,0); // Green
	triggered = false;
  }

}








