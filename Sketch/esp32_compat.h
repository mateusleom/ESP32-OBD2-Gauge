#ifndef ESP32_COMPAT_H
#define ESP32_COMPAT_H

#include <Arduino.h>

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3

#ifndef BUZZER_PIN
#define BUZZER_PIN 26
#endif

#ifndef buzzerChannel
#define buzzerChannel 2
#endif

#ifndef backlightChannel
#define backlightChannel 0
#endif

#ifndef TFT_BL
#define TFT_BL 21
#endif

inline void compat_ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution) {
  (void)channel; (void)freq; (void)resolution;
}

inline void compat_ledcAttachPin(uint8_t pin, uint8_t channel) {
  if (pin == BUZZER_PIN || channel == buzzerChannel) {
    (ledcAttach)(BUZZER_PIN, 1500, 10);
  } else {
    (ledcAttach)(pin, 12000, 8);
  }
}

inline uint32_t compat_ledcWriteTone(uint8_t pin_or_channel, uint32_t freq) {
  if (pin_or_channel == buzzerChannel) {
    return (ledcWriteTone)(BUZZER_PIN, freq);
  }
  return (ledcWriteTone)(pin_or_channel, freq);
}

inline bool compat_ledcWrite(uint8_t pin_or_channel, uint32_t duty) {
  if (pin_or_channel == backlightChannel) {
    return (ledcWrite)(TFT_BL, duty);
  }
  return (ledcWrite)(pin_or_channel, duty);
}

#define ledcSetup compat_ledcSetup
#define ledcAttachPin compat_ledcAttachPin
#define ledcWriteTone compat_ledcWriteTone
#define ledcWrite compat_ledcWrite

#endif // ESP_ARDUINO_VERSION_MAJOR >= 3
#endif // ESP32_COMPAT_H
