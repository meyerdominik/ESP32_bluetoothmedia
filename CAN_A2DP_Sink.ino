#include <ACAN_ESP32.h>
#include <ACAN_ESP32_AcceptanceFilters.h>
#include <ACAN_ESP32_Buffer16.h>
#include <ACAN_ESP32_CANRegisters.h>
#include <ACAN_ESP32_Settings.h>
#include <CANMessage.h>
#include <BluetoothA2DPSink.h>

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
bool OnCooldown = false;
#define CAN_SPEED 100000 // K-CAN: https://bimmerguide.de/bmw-bus-systeme/
#define ENABLE_CAN
#ifdef ENABLE_CAN
// #define DEBUG_CAN
#endif

void setup() {
  Serial.begin(9600);

  // can setup
#ifdef ENABLE_CAN
  Serial.println("Booting CAN");

  ACAN_ESP32_Settings settings (CAN_SPEED);
  settings.mRequestedCANMode = ACAN_ESP32_Settings::ListenOnlyMode;
  settings.mRxPin = CAN_RX;
  settings.mTxPin = CAN_TX;
  
  const uint32_t errorCode = ACAN_ESP32::can.begin(settings);

#ifdef DEBUG_CAN
  if (errorCode == 0) {
    Serial.println ("CAN ok") ;
  } else {
    Serial.print ("Error Can: 0x") ;
    Serial.println (errorCode, HEX) ;
  }
#endif
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
  CANMessage frame;
  
  if(ACAN_ESP32::can.receive(frame)) {

#ifdef DEBUG_CAN
    // Print frame
    Serial.print("New frame with id: 0x");
    Serial.print(frame.id, HEX);
    Serial.print(", len: ");
    Serial.print(frame.len);
    Serial.print(", data: 0x");
    for(int i = 0; i < frame.len; i++) {
      Serial.print(frame.data[i], HEX);
    }
    Serial.println("");
#endif
    
#ifdef ENABLE_A2DP
    if (!a2dp_sink.is_connected())
      return;
#endif

    if (frame.ext)
      return;
      
    if (frame.rtr)
      return;  
    
    if (frame.id == 0x1D6) { // https://www.loopybunny.co.uk/CarPC/can/1D6.html & http://www.loopybunny.co.uk/CarPC/k_can.html
      if (frame.len < 2)
        return;

      if (frame.data[0] == 0xC0 and frame.data[1] == 0x0C) { // continues Ping; nothing is pressed
        OnCooldown = false;
      } else if (!OnCooldown && frame.data[0] == 0xE0 and frame.data[1] == 0x0C) { // up button
#ifdef ENABLE_A2DP
        a2dp_sink.next();
#endif
        OnCooldown = true;
      } else if (!OnCooldown && frame.data[0] == 0xD0 and frame.data[1] == 0x0C) { // down button
#ifdef ENABLE_A2DP
        a2dp_sink.previous();
#endif
        OnCooldown = true;
      } else if (!OnCooldown && frame.data[0] == 0xC0 and frame.data[1] == 0x0D) { // voice button
#ifdef ENABLE_A2DP
        a2dp_sink.play();
#endif
        OnCooldown = true;
      } else if (!OnCooldown && frame.data[0] == 0xC1 and frame.data[1] == 0x0C) { // telephone button
#ifdef ENABLE_A2DP
        a2dp_sink.pause();
#endif
        OnCooldown = true;
      }
    }
  }
#endif
}
