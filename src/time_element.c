#include "fonts.h"
#include "layout.h"
#include "preferences.h"
#include "time_element.h"

#define TESTING_TIME_DISPLAY "13:37"


static BatteryComponent *create_battery_component(Layer *parent, uint8_t battery_loc) {
  GRect bounds = element_get_bounds(parent);
  int x = -1;
  int y = -1;
  bool align_right;
  if (battery_loc == BATTERY_LOC_TIME_TOP_LEFT) {
    x = battery_component_vertical_padding();
    y = 0;
    align_right = false;
  } else if (battery_loc == BATTERY_LOC_TIME_TOP_RIGHT) {
    x = bounds.size.w - battery_component_width() - battery_component_vertical_padding();
    y = 0;
    align_right = true;
  } else if (battery_loc == BATTERY_LOC_TIME_BOTTOM_LEFT) {
    x = battery_component_vertical_padding();
    y = bounds.size.h - battery_component_height();
    align_right = false;
  } else if (battery_loc == BATTERY_LOC_TIME_BOTTOM_RIGHT) {
    x = bounds.size.w - battery_component_width() - battery_component_vertical_padding();
    y = bounds.size.h - battery_component_height();
    align_right = true;
  }
  if (bounds.size.h <= battery_component_height()) {
    y = (bounds.size.h - battery_component_height()) / 2 - 1;
  }
  if (x != -1) {
    return battery_component_create(parent, x, y, align_right);
  } else {
    return NULL;
  }
}

static RecencyComponent *create_recency_component(Layer *parent, uint8_t recency_loc) {
  GRect bounds = element_get_bounds(parent);
  int16_t y = -1;
  bool align_right;
  if (recency_loc == RECENCY_LOC_TIME_TOP_LEFT) {
    y = 0;
    align_right = false;
  } else if (recency_loc == RECENCY_LOC_TIME_TOP_RIGHT) {
    y = 0;
    align_right = true;
  } else if (recency_loc == RECENCY_LOC_TIME_BOTTOM_LEFT) {
    y = bounds.size.h - recency_component_height();
    align_right = false;
  } else if (recency_loc == RECENCY_LOC_TIME_BOTTOM_RIGHT) {
    y = bounds.size.h - recency_component_height();
    align_right = true;
  }

  if (y != -1) {
    return recency_component_create(parent, y, align_right, NULL, NULL);
  } else {
    return NULL;
  }
}


TimeElement* time_element_create(Layer* parent) {
  GRect bounds = element_get_bounds(parent);
  Preferences *prefs = get_prefs();

  const int time_margin = 2;
  FontChoice font = choose_font_for_height(bounds.size.h, prefs->include_seconds);

  TimeElement* out = malloc(sizeof(TimeElement));

  TextLayer* time_text = add_text_layer(
    parent,
    GRect(time_margin, (bounds.size.h - font.height) / 2 - font.padding_top, bounds.size.w - 2 * time_margin, font.height + font.padding_top + font.padding_bottom),
    get_g_font(font),
    element_fg(parent),
    prefs->time_align == ALIGN_LEFT ? GTextAlignmentLeft : (prefs->time_align == ALIGN_CENTER ? GTextAlignmentCenter : GTextAlignmentRight)
  );

  out->time_text = time_text;
  out->battery = create_battery_component(parent, prefs->battery_loc);
  out->recency = create_recency_component(parent, prefs->recency_loc);
  return out;
}

void time_element_destroy(TimeElement* el) {
  text_layer_destroy(el->time_text);
  if (el->battery != NULL) {
    battery_component_destroy(el->battery);
  }
  if (el->recency != NULL) {
    recency_component_destroy(el->recency);
  }
  free(el);
}

void time_element_update(TimeElement *el, DataMessage *data) {
  time_element_tick(el);
}

void time_element_second_tick(TimeElement *el, struct tm* tick_time) {
  static char buffer[16];

  int hr = clock_is_24h_style() ? tick_time->tm_hour : (tick_time->tm_hour > 12 ? tick_time->tm_hour - 12 : (tick_time->tm_hour == 0 ? 12 : tick_time->tm_hour));

  if (get_prefs()->include_seconds) {
    snprintf (buffer, sizeof(buffer), "%d:%02d:%02d", hr, tick_time->tm_min, tick_time->tm_sec);
    if (hr >= 10) buffer[8]= 0;
    else buffer[7] = 0;
  }
  else {
    snprintf (buffer, sizeof(buffer), "%02d:%02d", hr, tick_time->tm_min);
    buffer[5] = 0;
  }

#ifdef IS_TEST_BUILD
  strcpy(buffer, TESTING_TIME_DISPLAY);
#endif

  text_layer_set_text(el->time_text, buffer);

  if (el->recency != NULL) {
    recency_component_tick(el->recency);
  }
}

void time_element_tick(TimeElement *el) {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  time_element_second_tick(el, tick_time);
}
