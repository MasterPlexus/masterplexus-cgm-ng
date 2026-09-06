#pragma once

#include <pebble.h>

typedef struct HealthStatComponent {
  Layer *chip_layer;
  GColor chip_color;
  BitmapLayer *icon_layer;
  GBitmap *icon_bitmap;
  TextLayer *text_layer;
  bool pulse; // false = steps, true = heart rate / pulse
  bool align_right; // hug the right edge of the parent element
  bool chip; // draw an opaque background so the value stays readable on busy rows
  int16_t host_w;   // width of the parent element
  int16_t icon_y;   // vertical position of the icon
  char buffer[12]; // own text buffer so steps & pulse never share memory
} HealthStatComponent;

uint8_t health_stat_component_width();
uint8_t health_stat_component_height();
HealthStatComponent* health_stat_component_create(Layer *parent, int16_t y, bool pulse, bool align_right, bool chip);
void health_stat_component_update(HealthStatComponent *c);
void health_stat_component_set_colors(HealthStatComponent *c, GColor fg, GCompOp op);
void health_stat_component_set_chip_color(HealthStatComponent *c, GColor color);
void health_stat_component_destroy(HealthStatComponent *c);
