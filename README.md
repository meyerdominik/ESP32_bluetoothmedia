# ESP32_bluetoothmedia
ESP32 BMW A2DP Sink K_CAN

Imported libs:
* https://github.com/pschatzmann/ESP32-A2DP
* [https://github.com/nhatuan84/esp32-can-protocol-demo](https://github.com/pierremolinaro/acan-esp32)

Used parts:
* ESP32: https://de.aliexpress.com/item/33004561102.html
* External DAC: https://de.aliexpress.com/item/1005004491534008.html
* CAN transceiver: https://de.aliexpress.com/item/1005002844175352.html
* AUX pinout adapter (not really necessary, since the external DAC has output pins to attatch to): https://www.amazon.de/gp/product/B009PH1IG4

Wireing:
* CAN 3.3V    -> ESP 3.3V
* CAN Ground  -> ESP Ground
* CAN RX      -> ESP pin D4 (GPIO 4)
* CAN TX      -> ESP pin D5 (GPIO 5)
* DAC 3.3V    -> ESP 3.3V
* DAC Ground  -> ESP Ground
* DAC FLT     -> ESP Ground
* DAC DMP     -> ESP Ground
* DAC SLC     -> ESP Ground
* DAC BCK     -> ESP pin D26 (GPIO 26)
* DAC DIN     -> ESP pin D22 (GPIO 22)
* DAC LCK     -> ESP pin D25 (GPIO 25)
* DAC FMT     -> ESP Ground
* DAC XMT     -> ESP 3.3V
* ESP USC     -> Power source

Remember to short the pins on the can transceiver, if you don't need the 120 Ohms resistor as an endpoint.
