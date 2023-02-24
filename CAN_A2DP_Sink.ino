#include <BluetoothA2DPSink.h>
#include <ESP32CAN.h>
#include <CAN_config.h>

// ESP pins https://elektro.turanis.de/html/prj135/index.html
#define I2S_BCK       GPIO_NUM_26
#define I2S_WS        GPIO_NUM_25
#define I2S_DATA_OUT  GPIO_NUM_22
#define I2S_DATA_IN   I2S_PIN_NO_CHANGE
#define CAN_TX        GPIO_NUM_5
#define CAN_RX        GPIO_NUM_4

// a2dp
BluetoothA2DPSink a2dp_sink;
const uint8_t myBLDevice = 0xACD6181CED4C;

// can
CAN_device_t CAN_cfg;
bool OnCooldown = false;

// temp
TaskHandle_t T_HandleTemprature;
extern uint8_t temprature_sens_read();
#define TEMP (temprature_sens_read() - 32) / 1.8 // convert to °C
#define MAXTEMP 100

void setup() {
  // temp
  // xTaskCreate(HandleTemprature, "Handle Temprature", 1024, NULL, 0, &T_HandleTemprature); // TODO

  // can setup
  CAN_cfg.speed = CAN_SPEED_100KBPS; // https://bimmerguide.de/bmw-bus-systeme/ K-CAN
  CAN_cfg.tx_pin_id = CAN_TX;
  CAN_cfg.rx_pin_id = CAN_RX;
  CAN_cfg.rx_queue = xQueueCreate(10, sizeof(CAN_frame_t));
  ESP32Can.CANInit();
  
  // a2dp setup
  i2s_pin_config_t my_pin_config = {
    .bck_io_num = I2S_BCK, 
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_DATA_OUT,
    .data_in_num = I2S_DATA_IN
  };
  a2dp_sink.set_pin_config(my_pin_config);
  a2dp_sink.set_task_core(2);
  a2dp_sink.start("E87");
  
  if (a2dp_sink.connect_to((uint8_t*) myBLDevice)) {
    a2dp_sink.set_volume(255); // volume to max
  }
}

void loop() {
  CAN_frame_t rx_frame;
  
  if(xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS)) {

    if (!a2dp_sink.is_connected())
      return;

    if (OnCooldown)
      return;

    if (rx_frame.FIR.B.FF != CAN_frame_std)
      return;
      
    if (rx_frame.FIR.B.RTR != CAN_RTR)
      return;
    
    if (rx_frame.MsgID == 0x1D6) { // https://www.loopybunny.co.uk/CarPC/can/1D6.html & http://www.loopybunny.co.uk/CarPC/k_can.html
      if (rx_frame.data.u8[0] == 0xC0 and rx_frame.data.u8[1] == 0x0C) { // continues Ping; nothing is pressed
        OnCooldown = false;
      } else if (rx_frame.data.u8[0] == 0xE0 and rx_frame.data.u8[1] == 0x0C) { // up button
        a2dp_sink.next();
        OnCooldown = true;
      } else if (rx_frame.data.u8[0] == 0xD0 and rx_frame.data.u8[1] == 0x0C) { // down button
        a2dp_sink.previous();
        OnCooldown = true;
      } else if (rx_frame.data.u8[0] == 0xC0 and rx_frame.data.u8[1] == 0x0D) { // voice button
        a2dp_sink.play();
        OnCooldown = true;
      } else if (rx_frame.data.u8[0] == 0xC1 and rx_frame.data.u8[1] == 0x0C) { // telephone button
        a2dp_sink.pause();
        OnCooldown = true;
      }
    }
  }
}

void HandleTemprature() {
  if (TEMP >= MAXTEMP) {
    a2dp_sink.disconnect();
    esp_deep_sleep_start();
  }
}
