#include <BluetoothA2DPSink.h>
#include <ESP32CAN.h>
#include <CAN_config.h>

BluetoothA2DPSink a2dp_sink;
CAN_device_t CAN_cfg;
uint8_t myBLDevice = 0xACD6181CED4C;
bool OnCoolDown = false;

void setup() {
  // can setup
  CAN_cfg.speed=CAN_SPEED_100KBPS; // https://bimmerguide.de/bmw-bus-systeme/ K-CAN
  CAN_cfg.tx_pin_id = GPIO_NUM_5; // TODO 
  CAN_cfg.rx_pin_id = GPIO_NUM_4; // TODO 
  CAN_cfg.rx_queue = xQueueCreate(10, sizeof(CAN_frame_t));
  ESP32Can.CANInit();
  
  // a2dp setup
  i2s_pin_config_t my_pin_config = {
        .bck_io_num = 26, // TODO 
        .ws_io_num = 25, // TODO 
        .data_out_num = 22, // TODO
        .data_in_num = I2S_PIN_NO_CHANGE
    };
  a2dp_sink.set_pin_config(my_pin_config);
  a2dp_sink.start("E87");

  if (a2dp_sink.connect_to(&myBLDevice)) {
    a2dp_sink.set_volume(255); // Volume to max
  }
}

void loop() {
  CAN_frame_t rx_frame;
  
  if(xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {

    if (!a2dp_sink.is_connected())
      return;

    // TODO: Add cooldown
    if (rx_frame.FIR.B.FF == CAN_frame_std && rx_frame.FIR.B.RTR != CAN_RTR) {
      if (rx_frame.MsgID == 0x1D6) { // https://www.loopybunny.co.uk/CarPC/can/1D6.html & http://www.loopybunny.co.uk/CarPC/k_can.html
        if (rx_frame.data.u8[0] == 0xC0 and rx_frame.data.u8[1] == 0x0C) {
          // Continues ping; do nothing
        } else if (rx_frame.data.u8[0] == 0xE0 and rx_frame.data.u8[1] == 0x0C) { // Up Button
          a2dp_sink.next();
        } else if (rx_frame.data.u8[0] == 0xD0 and rx_frame.data.u8[1] == 0x0C) { // Down Button
          a2dp_sink.previous();
        } else if (rx_frame.data.u8[0] == 0xC0 and rx_frame.data.u8[1] == 0x0D) { // Voice button
          a2dp_sink.play();
        } else if (rx_frame.data.u8[0] == 0xC1 and rx_frame.data.u8[1] == 0x0C) { // Telephone Button
          a2dp_sink.pause();
        }
      }
    }
  }
}
