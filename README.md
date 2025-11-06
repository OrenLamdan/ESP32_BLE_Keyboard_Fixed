# ESP32_BLE_Keyboard_Fixed

This works with:
ESP board version 2.0.14
It is based on the ESP32_BLE_Keyboard library by T-ck with some changes to the Authentication. The changes are all in the BleKeyboard.cpp file and are within these lines:
BLESecurity* pSecurity = new BLESecurity();

#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
// C3/S3: some hosts dislike SC+MITM with NO_IO; bonding-only is safer
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
#else
// Original, stronger policy for classic ESP32, S2, etc.
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
#endif
The BLEKeyboardWithButton example: connects to BT and send "hello world" each time the button is pressed. Make sure to connect a button to GPIO4 (other leg to the GND)
