#include <BluetoothA2DPSink.h>
#include <ESP32CAN.h>
#include <CAN_config.h>

// ESP pins https://elektro.turanis.de/html/prj135/index.html
#define I2S_BCK       GPIO_NUM_26
#define I2S_WS        GPIO_NUM_25
#define I2S_DATA_OUT  GPIO_NUM_22
#define I2S_DATA_IN   GPIO_NUM_21 // I2S_PIN_NO_CHANGE // will not be used
#define CAN_TX        GPIO_NUM_5
#define CAN_RX        GPIO_NUM_4

// a2dp
BluetoothA2DPSink a2dp_sink;
#define ENABLE_A2DP

// can
CAN_device_t CAN_cfg;
bool OnCooldown = false;
#define CAN_SPEED CAN_SPEED_100KBPS // K-CAN: https://bimmerguide.de/bmw-bus-systeme/
#define ENABLE_CAN

void setup() {
  Serial.begin(9600);

  // can setup
#ifdef ENABLE_CAN
  Serial.println("Booting CAN");
  CAN_cfg.speed = CAN_SPEED;
  CAN_cfg.tx_pin_id = CAN_TX;
  CAN_cfg.rx_pin_id = CAN_RX;
  CAN_cfg.rx_queue = xQueueCreate(10, sizeof(CAN_frame_t));
  ESP32Can.CANInit();
#endif 

  // a2dp setup
#ifdef ENABLE_A2DP
  Serial.println("Booting A2DP");
  i2s_pin_config_t my_pin_config = { // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html
    .bck_io_num = I2S_BCK, 
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_DATA_OUT,
    .data_in_num = I2S_DATA_IN
  };
  a2dp_sink.set_pin_config(my_pin_config);
  a2dp_sink.set_task_core(1);
  a2dp_sink.start("E87");
#endif

Serial.println("Boot ok");
}

void loop() {
#ifdef ENABLE_CAN
  CAN_frame_t rx_frame;
  
  Serial.print("Waiting on Frame: ");
  if(xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS)) {
    Serial.println("found!");
    
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
#ifdef ENABLE_A2DP
        a2dp_sink.next();
#endif
        OnCooldown = true;
      } else if (rx_frame.data.u8[0] == 0xD0 and rx_frame.data.u8[1] == 0x0C) { // down button
#ifdef ENABLE_A2DP
        a2dp_sink.previous();
#endif
        OnCooldown = true;
      } else if (rx_frame.data.u8[0] == 0xC0 and rx_frame.data.u8[1] == 0x0D) { // voice button
#ifdef ENABLE_A2DP
        a2dp_sink.play();
#endif
        OnCooldown = true;
      } else if (rx_frame.data.u8[0] == 0xC1 and rx_frame.data.u8[1] == 0x0C) { // telephone button
#ifdef ENABLE_A2DP
        a2dp_sink.pause();
#endif
        OnCooldown = true;
      }
    }
  } else {
    Serial.println("not found!");
  }
#endif
}
