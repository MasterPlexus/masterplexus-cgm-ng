#pragma once

#include <pebble.h>

// https://forums.getpebble.com/discussion/7147/text-layer-padding
typedef struct FontChoice {
  const char *key;
  int8_t res_id;
  uint8_t height;
  unsigned int padding_top:4;
  unsigned int padding_bottom:4;
  bool custom;
  bool narrow;
} FontChoice;

//enum {
//  FONT_18_BOLD,
//  FONT_24_BOLD,
//  FONT_28_BOLD,
//  FONT_34_NUMBERS,
//  FONT_42_BOLD,
//};

enum {
  FONT_18_BOLD,
  FONT_24_BOLD,
  FONT_24_NARROW,
  FONT_28_BOLD,
  FONT_34_NUMBERS,
  FONT_36_NARROW,
  FONT_42_NARROW,
  FONT_52_NARROW,
  FONT_42_BOLD,
};

static inline GFont get_g_font(FontChoice font) {
  return (font.custom ? fonts_load_custom_font(resource_get_handle(font.res_id)) : fonts_get_system_font(font.key));
};

FontChoice get_font(uint8_t font_size);
FontChoice choose_font_for_height(uint8_t height, bool narrow);