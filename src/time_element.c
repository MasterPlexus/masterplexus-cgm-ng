#include "fonts.h"
#include "layout.h"
#include "preferences.h"
#include "time_element.h"

#define TESTING_TIME_DISPLAY "13:37"

static HealthStatComponent *create_health_stat_component(Layer *parent, uint8_t loc, bool pulse, bool stack_after_sibling) {
  if (loc == STAT_LOC_NONE) {
    return NULL;
  }
  // Only the four corners of the time area are handled here; BG-row corners
  // are handled by the BG row element.
  bool is_time_loc = (loc == STAT_LOC_TIME_TOP_LEFT || loc == STAT_LOC_TIME_TOP_RIGHT ||
                      loc == STAT_LOC_TIME_BOTTOM_LEFT || loc == STAT_LOC_TIME_BOTTOM_RIGHT);
  if (!is_time_loc) {
    return NULL;
  }
  GRect bounds = element_get_bounds(parent);
  bool top = (loc == STAT_LOC_TIME_TOP_LEFT || loc == STAT_LOC_TIME_TOP_RIGHT);
  bool right = (loc == STAT_LOC_TIME_TOP_RIGHT || loc == STAT_LOC_TIME_BOTTOM_RIGHT);
  int16_t h = health_stat_component_height();
  int16_t y = top ? 0 : bounds.size.h - h;
  if (stack_after_sibling) {
    // If steps & pulse share a corner, stack them instead of overlapping.
    y = top ? (y + h + 1) : (y - h - 1);
    if (y < 0) {
      y = 0;
    }
  }
  return health_stat_component_create(parent, y, pulse, right, false);
}


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
  // If both steps & pulse use the same corner, stack the pulse beside the steps
  // so the two symbols never overlap.
  bool stacked = (prefs->steps_loc != STAT_LOC_NONE && prefs->steps_loc == prefs->pulse_loc);
  out->steps = create_health_stat_component(parent, prefs->steps_loc, false, false);
  out->pulse = create_health_stat_component(parent, prefs->pulse_loc, true, stacked);
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
  if (el->steps != NULL) {
    health_stat_component_destroy(el->steps);
  }
  if (el->pulse != NULL) {
    health_stat_component_destroy(el->pulse);
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

  // Refresh the health stats at most once per minute (they don't change more
  // often, and reading them every second would be wasteful).
  static int32_t last_stat_minute = -1;
  int32_t minute = tick_time->tm_hour * 60 + tick_time->tm_min;
  if (minute != last_stat_minute) {
    last_stat_minute = minute;
    if (el->steps != NULL) {
      health_stat_component_update(el->steps);
    }
    if (el->pulse != NULL) {
      health_stat_component_update(el->pulse);
    }
  }
}

void time_element_tick(TimeElement *el) {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  time_element_second_tick(el, tick_time);
}
