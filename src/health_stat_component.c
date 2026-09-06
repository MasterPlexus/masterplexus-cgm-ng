#include "fonts.h"
#include "health_stat_component.h"
#include "layout.h"

#define STAT_ICON_W 24
#define STAT_ICON_H 24
#define STAT_TEXT_GAP 3
#define STAT_TEXT_MAX_W 60
#define STAT_PAD 1
#define CHIP_PAD 2

static void chip_update_proc(Layer *layer, GContext *ctx) {
  HealthStatComponent *c = *(HealthStatComponent **)layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, c->chip_color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}
uint8_t health_stat_component_width() {
  return STAT_ICON_W + STAT_TEXT_GAP + STAT_TEXT_MAX_W;
}

uint8_t health_stat_component_height() {
  return STAT_ICON_H;
}

// Returns true when Pebble Health can currently provide this metric.
static bool metric_available(HealthMetric metric, bool accumulated) {
  time_t now = time(NULL);
  time_t start = now;
  if (accumulated) {
    struct tm *tm_ptr = localtime(&now);
    if (tm_ptr != NULL) {
      tm_ptr->tm_hour = 0;
      tm_ptr->tm_min = 0;
      tm_ptr->tm_sec = 0;
      start = mktime(tm_ptr);
    }
  }
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, now);
  return (mask & HealthServiceAccessibilityMaskAvailable) != 0;
}

// Repositions icon + value so the group hugs the chosen side: on the right the
// value sits flush against the edge (no empty gap), on the left it starts at
// the edge. The icon is always placed directly in front of the value.
static void layout_component(HealthStatComponent *c) {
  const char *text = text_layer_get_text(c->text_layer);
  FontChoice font = get_font(FONT_18_BOLD);
  GFont gfont = fonts_get_system_font(font.key);

  GSize content = graphics_text_layout_get_content_size(
    text, gfont, GRect(0, 0, STAT_TEXT_MAX_W, 200), GTextOverflowModeWordWrap, GTextAlignmentLeft
  );
  int16_t text_w = content.w > STAT_TEXT_MAX_W ? STAT_TEXT_MAX_W : content.w;

  int16_t text_total = font.height + font.padding_top + font.padding_bottom;
  int16_t icon_y = c->icon_y;
  int16_t text_y = icon_y + (STAT_ICON_H - text_total) / 2;

  int16_t icon_x;
  int16_t text_x;
  if (c->align_right) {
    // Group extends leftwards from the right edge: [icon][value] right-aligned.
    int16_t group_w = STAT_ICON_W + STAT_TEXT_GAP + text_w;
    icon_x = c->host_w - STAT_PAD - group_w;
    if (icon_x < 0) {
      icon_x = 0; // very long values: keep everything on screen
    }
    text_x = icon_x + STAT_ICON_W + STAT_TEXT_GAP;
  } else {
    icon_x = STAT_PAD;
    text_x = icon_x + STAT_ICON_W + STAT_TEXT_GAP;
  }

  layer_set_frame(
    bitmap_layer_get_layer(c->icon_layer),
    GRect(icon_x, icon_y, STAT_ICON_W, STAT_ICON_H)
  );
  layer_set_frame(
    text_layer_get_layer(c->text_layer),
    GRect(text_x, text_y, text_w, text_total)
  );

  if (c->chip_layer != NULL) {
    // Cover whatever is behind (e.g. the big BG number) with an opaque chip.
    int16_t left = icon_x < text_x ? icon_x : text_x;
    int16_t right = (icon_x + STAT_ICON_W) > (text_x + text_w) ? (icon_x + STAT_ICON_W) : (text_x + text_w);
    int16_t top = icon_y < text_y ? icon_y : text_y;
    int16_t bottom = (icon_y + STAT_ICON_H) > (text_y + text_total) ? (icon_y + STAT_ICON_H) : (text_y + text_total);
    left -= CHIP_PAD;
    top -= CHIP_PAD;
    right += CHIP_PAD;
    bottom += CHIP_PAD;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > c->host_w) right = c->host_w;
    layer_set_frame(c->chip_layer, GRect(left, top, right - left, bottom - top));
    layer_mark_dirty(c->chip_layer);
  }
}

void health_stat_component_update(HealthStatComponent *c) {
  if (c->pulse) {
    // instantaneous heart rate in BPM
    if (!metric_available(HealthMetricHeartRateBPM, false)) {
      strcpy(c->buffer, "-");
    } else {
      HealthValue v = health_service_peek_current_value(HealthMetricHeartRateBPM);
      if (v <= 0) {
        strcpy(c->buffer, "-");
      } else {
        snprintf(c->buffer, sizeof(c->buffer), "%d", (int)v);
      }
    }
  } else {
    // steps since the start of today
    if (!metric_available(HealthMetricStepCount, true)) {
      strcpy(c->buffer, "-");
    } else {
      HealthValue v = health_service_sum_today(HealthMetricStepCount);
      if (v < 0) {
        v = 0;
      }
      snprintf(c->buffer, sizeof(c->buffer), "%d", (int)v);
    }
  }

  text_layer_set_text(c->text_layer, c->buffer);
  layout_component(c);
  layer_mark_dirty(text_layer_get_layer(c->text_layer));
}

void health_stat_component_set_colors(HealthStatComponent *c, GColor fg, GCompOp op) {
  if (c == NULL) {
    return;
  }
  text_layer_set_text_color(c->text_layer, fg);
  bitmap_layer_set_compositing_mode(c->icon_layer, op);
}

void health_stat_component_set_chip_color(HealthStatComponent *c, GColor color) {
  if (c == NULL || c->chip_layer == NULL) {
    return;
  }
  c->chip_color = color;
  layer_mark_dirty(c->chip_layer);
}

HealthStatComponent* health_stat_component_create(Layer *parent, int16_t y, bool pulse, bool align_right, bool chip) {
  HealthStatComponent *c = malloc(sizeof(HealthStatComponent));
  c->pulse = pulse;
  c->align_right = align_right;
  c->chip = chip;
  c->host_w = element_get_bounds(parent).size.w;
  c->icon_y = y;
  c->chip_layer = NULL;

  if (chip) {
    c->chip_layer = layer_create_with_data(GRect(0, y, 10, 10), sizeof(HealthStatComponent *));
    *(HealthStatComponent **)layer_get_data(c->chip_layer) = c;
    layer_set_update_proc(c->chip_layer, chip_update_proc);
    layer_add_child(parent, c->chip_layer);
    c->chip_color = element_bg(parent);
  }

  uint32_t res_id = pulse ? RESOURCE_ID_ICON_HEART : RESOURCE_ID_ICON_SHOE;
  c->icon_layer = bitmap_layer_create(GRect(0, y, STAT_ICON_W, STAT_ICON_H));
  bitmap_layer_set_compositing_mode(c->icon_layer, element_comp_op(parent));
  layer_add_child(parent, bitmap_layer_get_layer(c->icon_layer));
  c->icon_bitmap = gbitmap_create_with_resource(res_id);
  bitmap_layer_set_bitmap(c->icon_layer, c->icon_bitmap);

  FontChoice font = get_font(FONT_18_BOLD);
  c->text_layer = add_text_layer(
    parent,
    GRect(0, y, 10, 10), // frame is set by layout_component()
    fonts_get_system_font(font.key),
    element_fg(parent),
    GTextAlignmentLeft
  );

  c->buffer[0] = 0;
  health_stat_component_update(c);
  return c;
}

void health_stat_component_destroy(HealthStatComponent *c) {
  if (c->icon_bitmap != NULL) {
    gbitmap_destroy(c->icon_bitmap);
  }
  if (c->chip_layer != NULL) {
    layer_destroy(c->chip_layer);
  }
  bitmap_layer_destroy(c->icon_layer);
  text_layer_destroy(c->text_layer);
  free(c);
}
