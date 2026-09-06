#include "fonts.h"
#include "format.h"
#include "layout.h"
#include "preferences.h"
#include "staleness.h"
#include "status_bar_element.h"

#define SM_TEXT_MARGIN (2)

// Fonts used for the status text. Ordered largest-first. Only system fonts with
// full letter/punctuation support are candidates so dates, units, etc. always
// render correctly.
static const uint8_t STATUS_FONT_SIZES[] = {FONT_28_BOLD, FONT_24_BOLD, FONT_18_BOLD};

// Pick the largest status font that fits the row: the font must fit on the
// screen at one line AND, once wrapped, the whole text must fit the height.
// This way a short line like a date grows with the status bar height, while a
// long multi-line status falls back to a smaller font and still fits.
static FontChoice font_for_status(GRect bounds, const char *text) {
  int16_t available_width = bounds.size.w - SM_TEXT_MARGIN;
  for (uint8_t i = 0; i < ARRAY_LENGTH(STATUS_FONT_SIZES); i++) {
    FontChoice font = get_font(STATUS_FONT_SIZES[i]);
    if ((int16_t)(font.height + font.padding_top + font.padding_bottom) > bounds.size.h) {
      // even a single line of this font would not fit the row
      continue;
    }
    GSize content = graphics_text_layout_get_content_size(
      text,
      fonts_get_system_font(font.key),
      GRect(0, 0, available_width, bounds.size.h),
      GTextOverflowModeWordWrap,
      GTextAlignmentLeft
    );
    if (content.h <= bounds.size.h) {
      return font;
    }
  }
  return get_font(FONT_18_BOLD);
}

// Positions the status text (with the given font) and keeps any battery and
// recency components aligned with the text. Called whenever the text or the
// available space may have changed.
static void layout_status_bar(StatusBarElement *el, FontChoice font, const char *text) {
  GRect bounds = el->bounds;

  text_layer_set_font(el->text, fonts_get_system_font(font.key));
  text_layer_set_text(el->text, text);

  int16_t text_y, height;
  if (bounds.size.h <= font.height * 2 + font.padding_top + font.padding_bottom) {
    // vertically center text if there is only room for one line
    text_y = (bounds.size.h - font.height) / 2 - font.padding_top;
    height = font.height + font.padding_top + font.padding_bottom;
  } else {
    // otherwise take up all the space, with half the default padding
    text_y = -1 * font.padding_top / 2;
    height = bounds.size.h - text_y;
  }
  layer_set_frame(text_layer_get_layer(el->text), GRect(SM_TEXT_MARGIN, text_y, bounds.size.w - SM_TEXT_MARGIN, height));

  if (el->battery != NULL) {
    // align the battery to the middle of the lowest line of text
    int8_t lines = (bounds.size.h - text_y) / (font.height + font.padding_top);
    int8_t battery_y = text_y + (font.height + font.padding_top) * (lines - 1) + font.padding_top + font.height / 2 - battery_component_height() / 2;
    // ...unless that places it too close to the bottom
    if (battery_y + battery_component_height() - battery_component_vertical_padding() > bounds.size.h - SM_TEXT_MARGIN) {
      battery_y = bounds.size.h - battery_component_height() + battery_component_vertical_padding() - SM_TEXT_MARGIN;
    }
    battery_component_reposition(el->battery, bounds.size.w - battery_component_width() - SM_TEXT_MARGIN, battery_y);
  }

  if (el->recency != NULL) {
    int8_t lines;
    if (get_prefs()->recency_loc == RECENCY_LOC_STATUS_TOP_RIGHT) {
      lines = 1;
    } else {
      lines = (bounds.size.h - text_y) / (font.height + font.padding_top);
    }
    // vertically align with the center of the first/last line of text
    int16_t recency_y = text_y + (font.height + font.padding_top) * (lines - 1) + font.padding_top + font.height / 2 - recency_component_height() / 2;
    // keep it within the bounds
    if (recency_y + recency_component_padding() < 0) {
      recency_y = -recency_component_padding();
    } else if (recency_y + recency_component_height() > bounds.size.h) {
      recency_y = bounds.size.h - recency_component_height() + recency_component_padding();
    }
    recency_component_reposition(el->recency, recency_y);
  }
}

// Re-formats the status text from the latest data message and applies the
// adaptive font/layout.
static void refresh_status_bar(StatusBarElement *el) {
  if (last_data_message() == NULL) {
    return;
  }
  static char buffer[STATUS_BAR_MAX_LENGTH + 16];
  format_status_bar_text(buffer, sizeof(buffer), last_data_message());

  FontChoice font = font_for_status(el->bounds, buffer);
  layout_status_bar(el, font, buffer);

  if (el->recency != NULL) {
    recency_component_tick(el->recency);
  }
}

StatusBarElement* status_bar_element_create(Layer *parent) {
  GRect bounds = element_get_bounds(parent);

  StatusBarElement *el = malloc(sizeof(StatusBarElement));
  el->bounds = bounds;
  el->battery = NULL;
  el->recency = NULL;

  // Provisional layer; its frame/font/text are set by layout_status_bar() below.
  el->text = add_text_layer(
    parent,
    GRect(0, 0, 10, 10),
    fonts_get_system_font(get_font(FONT_18_BOLD).key),
    element_fg(parent),
    GTextAlignmentLeft
  );
  text_layer_set_overflow_mode(el->text, GTextOverflowModeWordWrap);

  if (get_prefs()->battery_loc == BATTERY_LOC_STATUS_RIGHT) {
    el->battery = battery_component_create(parent, 0, 0, true);
  }

  if (get_prefs()->recency_loc == RECENCY_LOC_STATUS_TOP_RIGHT || get_prefs()->recency_loc == RECENCY_LOC_STATUS_BOTTOM_RIGHT) {
    el->recency = recency_component_create(parent, 0, true, NULL, NULL);
  }

  // Start with the default small font layout; refresh_status_bar() re-lays-out
  // adaptively as soon as data is available (e.g. right after a config change).
  layout_status_bar(el, get_font(FONT_18_BOLD), "");
  refresh_status_bar(el);

  return el;
}

void status_bar_element_destroy(StatusBarElement *el) {
  text_layer_destroy(el->text);
  if (el->battery != NULL) {
    battery_component_destroy(el->battery);
  }
  if (el->recency != NULL) {
    recency_component_destroy(el->recency);
  }
  free(el);
}

void status_bar_element_update(StatusBarElement *el, DataMessage *data) {
  status_bar_element_tick(el);
}

void status_bar_element_tick(StatusBarElement *el) {
  refresh_status_bar(el);
}
