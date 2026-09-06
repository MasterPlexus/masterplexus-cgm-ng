#pragma once

#include <pebble.h>
#include "app_messages.h"
#include "health_stat_component.h"
#include "trend_arrow_component.h"

typedef struct BGRowElement {
  Layer *parent;
  GSize parent_size;
  Layer *bg_layer;
  GColor bg_color;
  TextLayer *bg_text;
  TrendArrowComponent *trend;
  TextLayer *delta_text;
  HealthStatComponent *steps;
  HealthStatComponent *pulse;
} BGRowElement;

BGRowElement* bg_row_element_create(Layer *parent);
void bg_row_element_destroy(BGRowElement *el);
void bg_row_element_update(BGRowElement *el, DataMessage *data);
void bg_row_element_tick(BGRowElement *el);
