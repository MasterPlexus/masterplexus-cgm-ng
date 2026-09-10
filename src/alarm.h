#pragma once

#include <pebble.h>

// Glucose high/low vibration alarms.
void alarm_init(void);
void alarm_deinit(void);
// Called with the latest BG value (mg/dL, 0 = no data) on every data update.
void alarm_update(int16_t bg_mgdl);
// Called after new preferences were received (stops the alarm if disabled).
void alarm_prefs_changed(void);
