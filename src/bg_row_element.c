#include "bg_row_element.h"
#include "fonts.h"
#include "layout.h"
#include "preferences.h"
#include "text_updates.h"

#define BG_TREND_PADDING 8
#define TREND_DELTA_PADDING 5
#define MISSING_TREND_BG_DELTA_PADDING 10

static bool bg_loc_is_bg(uint8_t loc) {
  return loc == STAT_LOC_BG_TOP_LEFT || loc == STAT_LOC_BG_TOP_RIGHT ||
         loc == STAT_LOC_BG_BOTTOM_LEFT || loc == STAT_LOC_BG_BOTTOM_RIGHT;
}

static bool bg_loc_is_top(uint8_t loc) {
  return loc == STAT_LOC_BG_TOP_LEFT || loc == STAT_LOC_BG_TOP_RIGHT;
}

static HealthStatComponent *create_bg_stat(Layer *parent, uint8_t loc, bool pulse, bool stack) {
  if (!bg_loc_is_bg(loc)) {
    return NULL;
  }
  GRect bounds = element_get_bounds(parent);
  bool top = bg_loc_is_top(loc);
  bool right = (loc == STAT_LOC_BG_TOP_RIGHT || loc == STAT_LOC_BG_BOTTOM_RIGHT);
  int16_t h = health_stat_component_height();
  int16_t y = top ? 0 : bounds.size.h - h;
  if (stack) {
    y = top ? (y + h + 1) : (y - h - 1);
    if (y < 0) {
      y = 0;
    }
  }
  return health_stat_component_create(parent, y, pulse, right, true);
}

static void bg_fill_update_proc(Layer *layer, GContext *ctx) {
  BGRowElement *el = *(BGRowElement **)layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, el->bg_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

// Largest available narrow digit font that fits the row height, so the BG
// value can be shown really big on tall BG rows.
static FontChoice bg_value_font(int16_t height) {
  uint8_t sizes[] = {FONT_52_NARROW, FONT_42_NARROW, FONT_36_NARROW, FONT_24_NARROW};
  for (uint8_t i = 0; i < ARRAY_LENGTH(sizes); i++) {
    FontChoice font = get_font(sizes[i]);
    if ((int16_t)(font.height + font.padding_top + font.padding_bottom) <= height) {
      return font;
    }
  }
  return get_font(FONT_24_NARROW);
}

#ifdef PBL_COLOR
// Whether the given color is light enough for black content on top.
static bool color_is_light(GColor color) {
  // argb is packed as (a << 6) | (r << 4) | (g << 2) | b, 2 bits per channel.
  int sum = ((color.argb >> 4) & 3) + ((color.argb >> 2) & 3) + (color.argb & 3);
  return sum >= 5;
}

// Picks the fill color and the matching foreground/compositing for the big BG
// row: the high/low colors chosen in the graph settings color the whole row's
// background whenever the current BG is outside the target range.
static void bg_row_element_refresh(BGRowElement *el, DataMessage *data) {
  Preferences *prefs = get_prefs();
  int16_t bg = data->last_sgv;
  GColor bg_color;

  if (bg > 0 && bg > prefs->top_of_range) {
    // too high -> "when above range"
    bg_color = prefs->colors[COLOR_KEY_POINT_HIGH];
  } else if (bg > 0 && bg < prefs->bottom_of_range) {
    // too low -> "when below range"
    bg_color = prefs->colors[COLOR_KEY_POINT_LOW];
  } else {
    // in the normal range -> "when within range"
    bg_color = prefs->colors[COLOR_KEY_POINT_WITHIN];
  }

  el->bg_color = bg_color;
  bool light = color_is_light(bg_color);
  GColor fg;
  GCompOp op;
  if (prefs->bg_text_black) {
    // Force black text/icons on the BG row (e.g. on a light background).
    fg = GColorBlack;
    op = GCompOpAnd;
  } else {
    fg = light ? GColorBlack : GColorWhite;
    op = light ? GCompOpAnd : GCompOpSet;
  }
  text_layer_set_text_color(el->bg_text, fg);
  text_layer_set_text_color(el->delta_text, fg);
  trend_arrow_component_set_compositing(el->trend, op);
  if (el->steps != NULL) {
    health_stat_component_set_colors(el->steps, fg, op);
    health_stat_component_set_chip_color(el->steps, el->bg_color);
  }
  if (el->pulse != NULL) {
    health_stat_component_set_colors(el->pulse, fg, op);
    health_stat_component_set_chip_color(el->pulse, el->bg_color);
  }
  layer_mark_dirty(el->bg_layer);
}
#endif


static void bg_row_element_rearrange(BGRowElement *el) {
  GSize bg_size = text_layer_get_content_size(el->bg_text);
  GSize delta_size = text_layer_get_content_size(el->delta_text);
  uint8_t total_width = bg_size.w \
    + (trend_arrow_component_hidden(el->trend) ? 0 : BG_TREND_PADDING + trend_arrow_component_width()) \
    + (layer_get_hidden(text_layer_get_layer(el->delta_text)) ? 0 : TREND_DELTA_PADDING + delta_size.w);

  // Align the whole group (value + arrow + delta) to the configured side.
  int16_t bg_x;
  switch (get_prefs()->bg_align) {
    case ALIGN_LEFT:
      bg_x = 0;
      break;
    case ALIGN_RIGHT:
      bg_x = el->parent_size.w - total_width;
      break;
    case ALIGN_CENTER:
    default:
      bg_x = (el->parent_size.w - total_width) / 2;
      break;
  }
  if (bg_x < 0) {
    bg_x = 0;
  }

  GRect bg_frame = layer_get_frame(text_layer_get_layer(el->bg_text));
  layer_set_frame(text_layer_get_layer(el->bg_text), GRect(
    bg_x,
    bg_frame.origin.y,
    bg_frame.size.w,
    bg_frame.size.h
  ));

  trend_arrow_component_reposition(
    el->trend,
    bg_x + bg_size.w + BG_TREND_PADDING,
    (el->parent_size.h - trend_arrow_component_height()) / 2
  );

  GRect delta_frame = layer_get_frame(text_layer_get_layer(el->delta_text));
  int16_t delta_x = bg_x + bg_size.w \
    + (trend_arrow_component_hidden(el->trend) \
        ? MISSING_TREND_BG_DELTA_PADDING \
        : BG_TREND_PADDING + trend_arrow_component_width() + TREND_DELTA_PADDING);
  layer_set_frame(text_layer_get_layer(el->delta_text), GRect(
    delta_x,
    delta_frame.origin.y,
    delta_frame.size.w,
    delta_frame.size.h
  ));
}

BGRowElement* bg_row_element_create(Layer *parent) {
  GRect bounds = element_get_bounds(parent);

  BGRowElement *el = malloc(sizeof(BGRowElement));
  el->parent = parent;
  el->parent_size = bounds.size;
  el->bg_color = element_bg(parent);

  // Fill layer sits behind the text/arrow and paints the (possibly colored)
  // row background. It covers only the content area so element borders stay.
  el->bg_layer = layer_create_with_data(
    GRect(0, 0, bounds.size.w, bounds.size.h),
    sizeof(BGRowElement *)
  );
  *(BGRowElement **)layer_get_data(el->bg_layer) = el;
  layer_set_update_proc(el->bg_layer, bg_fill_update_proc);
  layer_add_child(parent, el->bg_layer);

//FontChoice bg_font = get_font(FONT_34_NUMBERS);
  // Scale the BG value to its row height: the taller the BG row (set in the
  // layout), the larger the number, up to the biggest available narrow font.
  FontChoice bg_font = bg_value_font(bounds.size.h);
//  FontChoice bg_font = get_font(FONT_36_NARROW);
  TextLayer *bg_text = add_text_layer(
    parent,
    GRect(
      0,
      (bounds.size.h - bg_font.height) / 2 - bg_font.padding_top,
      bounds.size.w,
      bg_font.height + bg_font.padding_top + bg_font.padding_bottom
    ),
    get_g_font(bg_font),
    element_fg(parent),
    GTextAlignmentLeft
  );

  TrendArrowComponent *trend = trend_arrow_component_create(
    parent,
    0, // set by bg_row_element_rearrange
    (bounds.size.h - trend_arrow_component_height()) / 2
  );

//FontChoice delta_font = get_font(FONT_28_BOLD);
  FontChoice delta_font = get_font(FONT_28_BOLD);
//FontChoice delta_font = choose_font_for_height(bounds.size.h - 8, false);
  TextLayer *delta_text = add_text_layer(
    parent,
    GRect(
      0, // set by bg_row_element_rearrange
      (bounds.size.h - delta_font.height) / 2 - delta_font.padding_top,
      bounds.size.w,
      delta_font.height + delta_font.padding_top + delta_font.padding_bottom
    ),
    get_g_font(delta_font),
    element_fg(parent),
    GTextAlignmentLeft
  );

  el->bg_text = bg_text;
  el->trend = trend;
  el->delta_text = delta_text;

  // Host steps/pulse inside the BG row when their location points here.
  Preferences *prefs = get_prefs();
  bool stacked = (bg_loc_is_bg(prefs->steps_loc) && prefs->steps_loc == prefs->pulse_loc);
  el->steps = create_bg_stat(parent, prefs->steps_loc, false, false);
  el->pulse = create_bg_stat(parent, prefs->pulse_loc, true, stacked);
  return el;
}

void bg_row_element_destroy(BGRowElement *el) {
  layer_destroy(el->bg_layer);
  text_layer_destroy(el->bg_text);
  trend_arrow_component_destroy(el->trend);
  text_layer_destroy(el->delta_text);
  if (el->steps != NULL) {
    health_stat_component_destroy(el->steps);
  }
  if (el->pulse != NULL) {
    health_stat_component_destroy(el->pulse);
  }
  free(el);
}

void bg_row_element_update(BGRowElement *el, DataMessage *data) {
  last_bg_text_layer_update(el->bg_text, data);
  trend_arrow_component_update(el->trend, data);
  delta_text_layer_update(el->delta_text, data);
  layer_set_hidden(
    text_layer_get_layer(el->delta_text),
    strcmp("-", text_layer_get_text(el->delta_text)) == 0
  );
  bg_row_element_rearrange(el);
#ifdef PBL_COLOR
  bg_row_element_refresh(el, data);
#endif
}

void bg_row_element_tick(BGRowElement *el) {
  // Refresh the health stats that are hosted in this row.
  if (el->steps != NULL) {
    health_stat_component_update(el->steps);
  }
  if (el->pulse != NULL) {
    health_stat_component_update(el->pulse);
  }
}
