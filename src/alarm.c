#include "alarm.h"
#include "preferences.h"

#define ALARM_TICK_MS 1000

static AppTimer *s_timer = NULL;
static uint8_t s_repeats_left = 0;
static uint8_t s_type = VIBE_OFF;
static int16_t s_last_bg = -1;

// 1-second vibration patterns per type (on/off durations in ms).
static uint32_t s_short[] = {200, 800};
static uint32_t s_double[] = {150, 150, 150, 550};
static uint32_t s_long[] = {900, 100};
static uint32_t s_pulse[] = {80, 120, 80, 120, 80, 120, 80, 320};

static void play_pattern(uint8_t type) {
  VibePattern pattern;
  switch (type) {
    case VIBE_SHORT:
      pattern = (VibePattern){.durations = s_short, .num_segments = ARRAY_LENGTH(s_short)};
      break;
    case VIBE_DOUBLE:
      pattern = (VibePattern){.durations = s_double, .num_segments = ARRAY_LENGTH(s_double)};
      break;
    case VIBE_LONG:
      pattern = (VibePattern){.durations = s_long, .num_segments = ARRAY_LENGTH(s_long)};
      break;
    case VIBE_PULSE:
      pattern = (VibePattern){.durations = s_pulse, .num_segments = ARRAY_LENGTH(s_pulse)};
      break;
    default:
      return;
  }
  vibes_enqueue_custom_pattern(pattern);
}

static void stop_alarm(void) {
  if (s_timer != NULL) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  s_repeats_left = 0;
  s_type = VIBE_OFF;
  vibes_cancel();
}

static void alarm_timer_cb(void *data) {
  s_timer = NULL;
  if (s_repeats_left == 0) {
    s_type = VIBE_OFF;
    return;
  }
  play_pattern(s_type);
  s_repeats_left--;
  if (s_repeats_left > 0) {
    s_timer = app_timer_register(ALARM_TICK_MS, alarm_timer_cb, NULL);
  } else {
    s_type = VIBE_OFF;
  }
}

// Starts (or restarts) an alarm of the given vibration type for `seconds`.
static void start_alarm(uint8_t type, uint8_t seconds) {
  stop_alarm();
  if (type == VIBE_OFF || seconds == 0) {
    return;
  }
  s_type = type;
  s_repeats_left = seconds;
  play_pattern(s_type);
  s_repeats_left--;
  if (s_repeats_left > 0) {
    s_timer = app_timer_register(ALARM_TICK_MS, alarm_timer_cb, NULL);
  }
}

void alarm_init(void) {
  s_timer = NULL;
  s_repeats_left = 0;
  s_type = VIBE_OFF;
  s_last_bg = -1;
}

void alarm_deinit(void) {
  stop_alarm();
}

void alarm_prefs_changed(void) {
  if (!get_prefs()->alarms_active) {
    stop_alarm();
  }
  // Re-baseline on the next reading so we only alarm on a real crossing.
  s_last_bg = -1;
}

void alarm_update(int16_t bg) {
  Preferences *prefs = get_prefs();

  if (!prefs->alarms_active) {
    stop_alarm();
    s_last_bg = bg;
    return;
  }
  if (bg <= 0) {
    s_last_bg = bg;
    return;
  }

  int16_t high = (int16_t)prefs->alarm_high;
  int16_t low = (int16_t)prefs->alarm_low;

  if (s_last_bg > 0) {
    bool was_high = s_last_bg > high;
    bool was_low = s_last_bg < low;
    if (!was_high && bg > high) {
      start_alarm(prefs->alarm_type_high, prefs->alarm_high_duration);
    } else if (!was_low && bg < low) {
      start_alarm(prefs->alarm_type_low, prefs->alarm_low_duration);
    }
  }

  s_last_bg = bg;
}
