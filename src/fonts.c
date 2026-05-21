#include "fonts.h"

#define MAX_UINT8 255

FontChoice get_font(uint8_t font_size) {
  switch(font_size) {

    case FONT_18_BOLD:
      return (FontChoice) {
        .key = FONT_KEY_GOTHIC_18_BOLD,
        .height = 11,
        .padding_top = 7,
        .padding_bottom = 3,
        .custom = false,
        .narrow = true,
      };

    case FONT_24_BOLD:
      return (FontChoice) {
        .key = FONT_KEY_GOTHIC_24_BOLD,
        .height = 14,
        .padding_top = 10,
        .padding_bottom = 4,
        .custom = false,
        .narrow = true,
      };

    case FONT_24_NARROW:
      return (FontChoice) {
        .height = 14,
        .padding_top = 10,
        .padding_bottom = 4,
        .custom = true,
        .narrow = true,
        .res_id = RESOURCE_ID_FNT_NARROW_ALT_24,
      };

    case FONT_28_BOLD:
      return (FontChoice) {
        .key = FONT_KEY_GOTHIC_28_BOLD,
        .height = 18,
        .padding_top = 10,
        .padding_bottom = 4,
        .custom = false,
        .narrow = true,
      };

    case FONT_34_NUMBERS:
      return (FontChoice) {
        .key = FONT_KEY_BITHAM_34_MEDIUM_NUMBERS,
        .height = 24,
        .padding_top = 10,
        .padding_bottom = 0,
        .custom = false,
        .narrow = true,
      };

    case FONT_36_NARROW:
      return (FontChoice) {
        .height = 26,
        .padding_top = 10,
        .padding_bottom = 0,
        .custom = true,
        .narrow = true,
        .res_id = RESOURCE_ID_FNT_NARROW_ALT_36,
      };


    case FONT_42_BOLD:
      return (FontChoice) {
        .key = FONT_KEY_BITHAM_42_BOLD,
        .height = 30,
        .padding_top = 12,
        .padding_bottom = 8,
        .custom = false,
        .narrow = false,
      };

    case FONT_42_NARROW:
       return (FontChoice) {
        .height = 36,
        .padding_top = 12,
        .padding_bottom = 8,
        .custom = true,
        .narrow = true,
        .res_id = RESOURCE_ID_FNT_NARROW_ALT_42,     
       };   

    default:
      return (FontChoice) {
        .key = FONT_KEY_GOTHIC_24_BOLD,
        .height = 14,
        .padding_top = 10,
        .padding_bottom = 4,
        .custom = false,
        .narrow = false,
      };

  }
}


FontChoice choose_font_for_height(uint8_t height, bool narrow) {
// Go through system fonts ONLY if narrow fonts aren't requested
  narrow = false;
  if ( !narrow ) {
//    uint8_t choices[] = {FONT_42_BOLD, FONT_34_NUMBERS, FONT_28_BOLD, FONT_24_BOLD, FONT_18_BOLD};
    uint8_t choices[] = { FONT_34_NUMBERS, FONT_28_BOLD, FONT_24_BOLD, FONT_18_BOLD };
    for(uint8_t i = 0; i < ARRAY_LENGTH(choices); i++) {
      if (get_font(choices[i]).height < height) {
        return get_font(choices[i]);
      }
    }
  }
  else
  {
    // use specific narrow fonts if narrow fonts are included
    uint8_t choices[] = {FONT_42_NARROW, FONT_36_NARROW, FONT_24_NARROW};
    for(uint8_t i = 0; i < ARRAY_LENGTH(choices); i++) {
      if (get_font(choices[i]).height < height) {
        return get_font(choices[i]);
      }
    }
  }
  return get_font(MAX_UINT8);
}
