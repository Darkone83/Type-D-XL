#pragma once

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include "TCA9554PWR.h"

// Define EXIO pins as in vendor code
#define LCD_CS_PIN  EXIO_PIN3
#define LCD_RST_PIN EXIO_PIN1
#define LCD_SDA_PIN 1
#define LCD_SCL_PIN 2

inline void LCD_CS_L()  { Set_EXIO(LCD_CS_PIN, Low); }
inline void LCD_CS_H()  { Set_EXIO(LCD_CS_PIN, High); }
inline void LCD_RST_L() { Set_EXIO(LCD_RST_PIN, Low); }
inline void LCD_RST_H() { Set_EXIO(LCD_RST_PIN, High); }
inline void LCD_SCL_L() { digitalWrite(LCD_SCL_PIN, LOW); }
inline void LCD_SCL_H() { digitalWrite(LCD_SCL_PIN, HIGH); }
inline void LCD_SDA_L() { digitalWrite(LCD_SDA_PIN, LOW); }
inline void LCD_SDA_H() { digitalWrite(LCD_SDA_PIN, HIGH); }

// 9-bit SPI bitbang: DC bit + 8 data bits (MSB first)
inline void ST7701_Write9bit(uint8_t dc, uint8_t data)
{
  if (dc) LCD_SDA_H(); else LCD_SDA_L();
  LCD_SCL_L(); delayMicroseconds(1); LCD_SCL_H(); delayMicroseconds(1);
  for (uint8_t i = 0; i < 8; ++i) {
    if (data & 0x80) LCD_SDA_H(); else LCD_SDA_L();
    LCD_SCL_L(); delayMicroseconds(1); LCD_SCL_H(); delayMicroseconds(1);
    data <<= 1;
  }
}
inline void ST7701_WriteCommand(uint8_t cmd)
{
  LCD_CS_L();
  ST7701_Write9bit(0, cmd);
  LCD_CS_H();
}
inline void ST7701_WriteData(uint8_t data)
{
  LCD_CS_L();
  ST7701_Write9bit(1, data);
  LCD_CS_H();
}
inline void ST7701_Reset()
{
  LCD_RST_L(); delay(10);
  LCD_RST_H(); delay(50);
}

inline void vendor_ST7701_init()
{
  LCD_CS_L();
  ST7701_WriteCommand(0xFF); ST7701_WriteData(0x77); ST7701_WriteData(0x01); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x10);
  ST7701_WriteCommand(0xC0); ST7701_WriteData(0x3B); ST7701_WriteData(0x00);
  ST7701_WriteCommand(0xC1); ST7701_WriteData(0x0B); ST7701_WriteData(0x02);
  ST7701_WriteCommand(0xC2); ST7701_WriteData(0x07); ST7701_WriteData(0x02);
  ST7701_WriteCommand(0xCC); ST7701_WriteData(0x10);
  ST7701_WriteCommand(0xCD); ST7701_WriteData(0x08);
  ST7701_WriteCommand(0xB0); ST7701_WriteData(0x00); ST7701_WriteData(0x11); ST7701_WriteData(0x16); ST7701_WriteData(0x0e); ST7701_WriteData(0x11); ST7701_WriteData(0x06); ST7701_WriteData(0x05); ST7701_WriteData(0x09); ST7701_WriteData(0x08); ST7701_WriteData(0x21); ST7701_WriteData(0x06); ST7701_WriteData(0x13); ST7701_WriteData(0x10); ST7701_WriteData(0x29); ST7701_WriteData(0x31); ST7701_WriteData(0x18);
  ST7701_WriteCommand(0xB1); ST7701_WriteData(0x00); ST7701_WriteData(0x11); ST7701_WriteData(0x16); ST7701_WriteData(0x0e); ST7701_WriteData(0x11); ST7701_WriteData(0x07); ST7701_WriteData(0x05); ST7701_WriteData(0x09); ST7701_WriteData(0x09); ST7701_WriteData(0x21); ST7701_WriteData(0x05); ST7701_WriteData(0x13); ST7701_WriteData(0x11); ST7701_WriteData(0x2a); ST7701_WriteData(0x31); ST7701_WriteData(0x18);
  ST7701_WriteCommand(0xFF); ST7701_WriteData(0x77); ST7701_WriteData(0x01); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x11);
  ST7701_WriteCommand(0xB0); ST7701_WriteData(0x6d);
  ST7701_WriteCommand(0xB1); ST7701_WriteData(0x37);
  ST7701_WriteCommand(0xB2); ST7701_WriteData(0x81);
  ST7701_WriteCommand(0xB3); ST7701_WriteData(0x80);
  ST7701_WriteCommand(0xB5); ST7701_WriteData(0x43);
  ST7701_WriteCommand(0xB7); ST7701_WriteData(0x85);
  ST7701_WriteCommand(0xB8); ST7701_WriteData(0x20);
  ST7701_WriteCommand(0xC1); ST7701_WriteData(0x78);
  ST7701_WriteCommand(0xC2); ST7701_WriteData(0x78);
  ST7701_WriteCommand(0xD0); ST7701_WriteData(0x88);
  ST7701_WriteCommand(0xE0); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x02);
  ST7701_WriteCommand(0xE1); ST7701_WriteData(0x03); ST7701_WriteData(0xA0); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x04); ST7701_WriteData(0xA0); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x20); ST7701_WriteData(0x20);
  ST7701_WriteCommand(0xE2); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00);
  ST7701_WriteCommand(0xE3); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x11); ST7701_WriteData(0x00);
  ST7701_WriteCommand(0xE4); ST7701_WriteData(0x22); ST7701_WriteData(0x00);
  ST7701_WriteCommand(0xE5); ST7701_WriteData(0x05); ST7701_WriteData(0xEC); ST7701_WriteData(0xA0); ST7701_WriteData(0xA0); ST7701_WriteData(0x07); ST7701_WriteData(0xEE); ST7701_WriteData(0xA0); ST7701_WriteData(0xA0); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00);
  ST7701_WriteCommand(0xE6); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x11); ST7701_WriteData(0x00);
  ST7701_WriteCommand(0xE7); ST7701_WriteData(0x22); ST7701_WriteData(0x00);
  ST7701_WriteCommand(0xE8); ST7701_WriteData(0x06); ST7701_WriteData(0xED); ST7701_WriteData(0xA0); ST7701_WriteData(0xA0); ST7701_WriteData(0x08); ST7701_WriteData(0xEF); ST7701_WriteData(0xA0); ST7701_WriteData(0xA0); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00);
  ST7701_WriteCommand(0xEB); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x40); ST7701_WriteData(0x40); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00);
  ST7701_WriteCommand(0xED); ST7701_WriteData(0xFF); ST7701_WriteData(0xFF); ST7701_WriteData(0xFF); ST7701_WriteData(0xBA); ST7701_WriteData(0x0A); ST7701_WriteData(0xBF); ST7701_WriteData(0x45); ST7701_WriteData(0xFF); ST7701_WriteData(0xFF); ST7701_WriteData(0x54); ST7701_WriteData(0xFB); ST7701_WriteData(0xA0); ST7701_WriteData(0xAB); ST7701_WriteData(0xFF); ST7701_WriteData(0xFF); ST7701_WriteData(0xFF);
  ST7701_WriteCommand(0xEF); ST7701_WriteData(0x10); ST7701_WriteData(0x0D); ST7701_WriteData(0x04); ST7701_WriteData(0x08); ST7701_WriteData(0x3F); ST7701_WriteData(0x1F);
  ST7701_WriteCommand(0xFF); ST7701_WriteData(0x77); ST7701_WriteData(0x01); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x13);
  ST7701_WriteCommand(0xEF); ST7701_WriteData(0x08);
  ST7701_WriteCommand(0xFF); ST7701_WriteData(0x77); ST7701_WriteData(0x01); ST7701_WriteData(0x00); ST7701_WriteData(0x00); ST7701_WriteData(0x00);
  ST7701_WriteCommand(0x36); ST7701_WriteData(0x00);
  ST7701_WriteCommand(0x3A); ST7701_WriteData(0x66);
  ST7701_WriteCommand(0x11); delay(480);
  ST7701_WriteCommand(0x20); delay(120);
  ST7701_WriteCommand(0x29);
  LCD_CS_H();
}


// LGFX device for ESP32S3+RGB
class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_RGB      _panel_instance;
  lgfx::Bus_RGB        _bus_instance;
  lgfx::Light_PWM      _light_instance;
public:
  LGFX(void)
  {
    { // RGB Bus
      auto cfg = _bus_instance.config();
      cfg.panel = &_panel_instance;
      cfg.pin_d0  = 5;   cfg.pin_d1  = 45;  cfg.pin_d2  = 48;  cfg.pin_d3  = 47;  cfg.pin_d4  = 21;
      cfg.pin_d5  = 14;  cfg.pin_d6  = 13;  cfg.pin_d7  = 12;  cfg.pin_d8  = 11;  cfg.pin_d9  = 10; cfg.pin_d10 = 9;
      cfg.pin_d11 = 46;  cfg.pin_d12 = 3;   cfg.pin_d13 = 8;   cfg.pin_d14 = 18;  cfg.pin_d15 = 17;
      cfg.pin_hsync = 38; cfg.pin_vsync = 39;
      cfg.pin_henable = 40;    cfg.pin_pclk = 41;
      cfg.freq_write = 16000000;
      cfg.hsync_polarity = 1; cfg.hsync_front_porch = 50; cfg.hsync_pulse_width = 8; cfg.hsync_back_porch = 10;
      cfg.vsync_polarity = 1; cfg.vsync_front_porch = 8;  cfg.vsync_pulse_width = 3; cfg.vsync_back_porch = 8;
      cfg.pclk_active_neg = false;
      cfg.de_idle_high = false;
      cfg.pclk_idle_high = false;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    { // Panel
      auto cfg = _panel_instance.config();
      cfg.memory_width  = 480;
      cfg.memory_height = 480;
      cfg.panel_width   = 480;
      cfg.panel_height  = 480;
      cfg.offset_x      = 0;
      cfg.offset_y      = 0;
      cfg.invert        = false;
      cfg.rgb_order     = false;
      cfg.dlen_16bit    = true;
      cfg.bus_shared    = false;
      cfg.pin_cs   = -1;
      cfg.pin_rst  = -1;
      cfg.pin_busy = -1;
      _panel_instance.config(cfg);
    }
    { // Backlight PWM
      auto cfg = _light_instance.config();
      cfg.pin_bl = 6;
      cfg.invert = false;
      cfg.freq = 20000;
      cfg.pwm_channel = 1;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }
    setPanel(&_panel_instance);
  }
};

// Version
static constexpr char VERSION_TEXT[] = "v0.7.2 Beta";
