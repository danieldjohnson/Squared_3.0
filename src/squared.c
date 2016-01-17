/*
 * Original source code by lastfuture
 * SDK 2.0beta4 port by Jnm
 * SDK 3.0 port and colorizing by hexahedria
 * adaptations for Chalk and Aplite as well as continued development by lastfuture
 */

#include <pebble.h>

Window *window;

typedef struct {
  bool large_mode;
  bool eu_date;
  bool quick_start;
  bool leading_zero;
  int background_color;
  int number_base_color;
  bool number_variation;
  int ornament_base_color;
  bool ornament_variation;
  bool invert;
  bool monochrome;
  bool center;
  bool btvibe;
  bool contrast;
  bool nightsaver;
  int ns_start;
  int ns_stop;
  bool backlight;
  
} Preferences;

Preferences curPrefs;

enum {
    KEY_LARGE_MODE,
    KEY_EU_DATE,
    KEY_QUICK_START,
    KEY_LEADING_ZERO,
    KEY_BACKGROUND_COLOR,
    KEY_NUMBER_BASE_COLOR,
    KEY_NUMBER_VARIATION,
    KEY_ORNAMENT_BASE_COLOR,
    KEY_ORNAMENT_VARIATION,
    KEY_INVERT,
    KEY_MONOCHROME,
    KEY_CENTER,
    KEY_BTVIBE,
    KEY_CONTRAST,
    KEY_NIGHTSAVER,
    KEY_NS_START,
    KEY_NS_STOP,
    KEY_BACKLIGHT,
};

#define KEY_DEBUGWATCH 50

#define PREFERENCES_KEY 0

#define US_DATE (!curPrefs.eu_date) // true == MM/DD, false == DD/MM
#define CENTER_DATE (curPrefs.center)
#define DISCONNECT_VIBRATION (curPrefs.btvibe)
#define CONTRAST_WHILE_CHARGING PBL_IF_BW_ELSE(false, (curPrefs.contrast))
#define LIGHT_WHILE_CHARGING (curPrefs.backlight)
#define DISABLE_ANIM (curPrefs.nightsaver)
#define DISABLE_ANIM_START_TIME (curPrefs.ns_start)
#define DISABLE_ANIM_END_TIME (curPrefs.ns_stop)
#define NO_ZERO (!curPrefs.leading_zero) // true == replaces leading Zero for hour, day, month with a "cycler"
#define TILE_SIZE PBL_IF_RECT_ELSE((curPrefs.large_mode ? 12 : 10), 10)
#define INVERT (curPrefs.invert)
#define GREYS (curPrefs.monochrome)
#define NUMSLOTS PBL_IF_RECT_ELSE(8, 18)
#define SPACING_X TILE_SIZE
#define SPACING_Y (curPrefs.large_mode ? TILE_SIZE - 1 : TILE_SIZE)
#define DIGIT_CHANGE_ANIM_DURATION 1500
#define STARTDELAY (curPrefs.quick_start ? 1000 : 2000)

#define NUMBER_BASE_COLOR_ARGB8   (curPrefs.number_base_color)
#define ORNAMENT_BASE_COLOR_ARGB8 (curPrefs.ornament_base_color)
#define NUMBER_ADD_VARIATION      (curPrefs.number_variation)
#define ORNAMENT_ADD_VARIATION    (curPrefs.ornament_variation)

#define BACKGROUND_COLOR    PBL_IF_BW_ELSE((INVERT ? GColorWhite : GColorBlack), ((GColor8) { .argb = curPrefs.background_color }))

#define FONT blocks

typedef struct {
	Layer *layer;
	int   prevDigit;
	int   curDigit;
	int   divider;
	AnimationProgress normTime;
  int   slotIndex;
} digitSlot;

digitSlot slot[NUMSLOTS];

AnimationImplementation animImpl;
Animation *anim;
bool splashEnded = false;
static bool debug = false;

unsigned char blocks[][5][5] =  {{
	{1,1,1,1,1},
	{1,0,0,0,1},
	{1,0,2,0,1},
	{1,0,0,0,1},
	{1,1,1,1,1}
}, {
	{2,2,0,1,1},
	{0,0,0,0,1},
	{2,2,2,0,1},
	{0,0,0,0,1},
	{2,2,2,0,1}
}, {
	{1,1,1,1,1},
	{0,0,0,0,1},
	{1,1,1,1,1},
	{1,0,0,0,0},
	{1,1,1,1,1}
}, {
	{1,1,1,1,1},
	{0,0,0,0,1},
	{0,1,1,1,1},
	{0,0,0,0,1},
	{1,1,1,1,1}
}, {
	{1,0,2,0,1},
	{1,0,0,0,1},
	{1,1,1,1,1},
	{0,0,0,0,1},
	{2,2,2,0,1}
}, {
	{1,1,1,1,1},
	{1,0,0,0,0},
	{1,1,1,1,1},
	{0,0,0,0,1},
	{1,1,1,1,1}
}, {
	{1,1,1,1,1},
	{1,0,0,0,0},
	{1,1,1,1,1},
	{1,0,0,0,1},
	{1,1,1,1,1}
}, {
	{1,1,1,1,1},
	{0,0,0,0,1},
	{2,0,2,0,1},
	{2,0,2,0,1},
	{2,0,2,0,1}
}, {
	{1,1,1,1,1},
	{1,0,0,0,1},
	{1,1,1,1,1},
	{1,0,0,0,1},
	{1,1,1,1,1}
}, {
	{1,1,1,1,1},
	{1,0,0,0,1},
	{1,1,1,1,1},
	{0,0,0,0,1},
	{1,1,1,1,1}
}, {
	{2,2,2,2,2},
	{0,0,0,0,0},
	{2,2,2,2,2},
	{0,0,0,0,0},
	{2,2,2,2,2}
}, {
	{2,0,2,0,2},
	{2,0,2,0,2},
	{2,0,2,0,2},
	{2,0,2,0,2},
	{2,0,2,0,2}
}, {
	{1,1,1,1,1},
	{0,0,0,0,0},
	{1,1,1,1,1},
	{0,0,0,0,0},
	{1,1,1,1,1}
}, {
	{1,0,1,0,1},
	{1,0,1,0,1},
	{1,0,1,0,1},
	{1,0,1,0,1},
	{1,0,1,0,1}
}};

int startDigit[18] = {
	11,12,12,11,11,12,10,13,12,11,12,11,11,12,10,13,12,10 // 2x h, 2x m, 4x date, 2x filler top, 4x filler sides, 2x filler bottom, 2x filler bottom sides
};

unsigned char variation[] = {
  0b00000000, 0b00010000, 0b00000100, 0b00010000, 0b00000000,
  0b00010001, 0b00010000, 0b00000000, 0b00000101, 0b00010000,
  0b00000001, 0b00000000, 0b00000000, 0b00010100, 0b00010001,
  0b00000000, 0b00010001, 0b00000101, 0b00010001, 0b00010100,
  0b00000000, 0b00010001, 0b00000101, 0b00000000, 0b00000001,
  0b00000100, 0b00010100, 0b00010100, 0b00000001, 0b00010100,
  0b00010001, 0b00000001, 0b00010101, 0b00010101, 0b00010000,
  0b00000000, 0b00010100, 0b00010001, 0b00010100, 0b00000100,
  0b00010100, 0b00010101, 0b00010001, 0b00010001, 0b00010101,
  0b00010000, 0b00010000, 0b00000000, 0b00010000, 0b00000001,
  0b00000001, 0b00000000, 0b00010100, 0b00000000, 0b00010100,
  0b00010100, 0b00000001, 0b00000101, 0b00010000, 0b00010000,
  0b00000001, 0b00010000, 0b00000000, 0b00000001, 0b00000000,
  0b00010100, 0b00000100, 0b00000001, 0b00010000, 0b00010000,
  0b00000001, 0b00000001, 0b00000101, 0b00010101, 0b00000000,
  0b00010000, 0b00000001, 0b00000001, 0b00010000, 0b00000000,
  0b00000100, 0b00010000, 0b00010100, 0b00010000, 0b00010100,
  0b00000100, 0b00010101, 0b00010101, 0b00010000, 0b00010101,
  0b00000101, 0b00000001, 0b00010100, 0b00010100, 0b00000000,
  0b00000000, 0b00000001, 0b00010001, 0b00000101, 0b00010100
};

static uint8_t shadowtable[] = {
  192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,
  192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,
  192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,
  192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,
  192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197,
  192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197,
  192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197,
  208,208,208,209,208,208,208,209,208,208,208,209,212,212,212,213,
  192,192,193,194,192,192,193,194,196,196,197,198,200,200,201,202,
  192,192,193,194,192,192,193,194,196,196,197,198,200,200,201,202,
  208,208,209,210,208,208,209,210,212,212,213,214,216,216,217,218,
  224,224,225,226,224,224,225,226,228,228,229,230,232,232,233,234,
  192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
  208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
  224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
  240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};
// alpha should only be 0b??111111 where ?? = 00 (full shade), 01 (much shade), 10 (some shade), 11 (none shade)
static uint8_t alpha = 0b10111111;

/*
uint8_t combine_colors(uint8_t fg_color, uint8_t bg_color) {
  return (shadowtable[((~fg_color)&0b11000000) + (bg_color&63)]&63) + shadowtable[fg_color];
}
*/

#define FONT_HEIGHT_BLOCKS (sizeof *FONT / sizeof **FONT)
#define FONT_WIDTH_BLOCKS (sizeof **FONT)
#define TOTALBLOCKS FONT_HEIGHT_BLOCKS * FONT_WIDTH_BLOCKS
#define FONT_HEIGHT FONT_HEIGHT_BLOCKS*TILE_SIZE
#define FONT_WIDTH FONT_WIDTH_BLOCKS*TILE_SIZE

#define TILES_X ( \
    FONT_WIDTH + SPACING_X + FONT_WIDTH)
#define TILES_Y ( \
    FONT_HEIGHT + SPACING_Y + FONT_HEIGHT)

#define ORIGIN_X PBL_IF_RECT_ELSE(((144 - TILES_X)/2), ((180 - TILES_X)/2))
#define ORIGIN_Y PBL_IF_RECT_ELSE((curPrefs.large_mode ? 1 : TILE_SIZE*1.5), (TILE_SIZE*2.2))

static bool contrastmode = false, previous_contrastmode = false, allow_animate = true;

static void handle_bluetooth(bool connected) {
  if (DISCONNECT_VIBRATION && !connected) {
    static uint32_t const segments[] = { 200, 200, 50, 150, 200 };
    VibePattern pat = {
    	.durations = segments,
    	.num_segments = ARRAY_LENGTH(segments),
    };
    vibes_enqueue_custom_pattern(pat);
  }
}

static GRect slotFrame(int i) {
	int x, y, w, h;
	if (i<4) { // main digits
		w = FONT_WIDTH;
		h = FONT_HEIGHT;
		if (i%2) {
			x = ORIGIN_X + FONT_WIDTH + SPACING_X; // i = 1 or 3
		} else {
			x = ORIGIN_X; // i = 0 or 2
		}
		
		if (i<2) {
			y = ORIGIN_Y;
		} else {
			y = ORIGIN_Y + FONT_HEIGHT + SPACING_Y;
		}
	} else if (i<8) { // date digits
		w = FONT_WIDTH/2;
		h = FONT_HEIGHT/2;
		x = ORIGIN_X + (FONT_WIDTH + SPACING_X) * (i - 4) / 2;
		y = ORIGIN_Y + (FONT_HEIGHT + SPACING_Y) * 2;
	} else if (i<10) { // top filler for round
    w = FONT_WIDTH;
		h = FONT_HEIGHT;
    if (i%2) {
			x = ORIGIN_X + FONT_WIDTH + SPACING_X; // i = 1 or 3
		} else {
			x = ORIGIN_X; // i = 0 or 2
		}
    y = ORIGIN_Y - FONT_HEIGHT - SPACING_Y;
  } else if (i<14) { // side filler for round
    w = FONT_WIDTH;
		h = FONT_HEIGHT;
    if (i%2) {
			x = ORIGIN_X + FONT_WIDTH + SPACING_X + FONT_WIDTH + SPACING_X;
		} else {
			x = (int) (ORIGIN_X - FONT_WIDTH - SPACING_X);
		}
		if (i<12) {
			y = ORIGIN_Y;
		} else {
			y = ORIGIN_Y + FONT_HEIGHT + SPACING_Y;
		}
  } else if (i<16) { // botom filler for round
		w = FONT_WIDTH/2;
		h = FONT_HEIGHT/2;
    x = ORIGIN_X + (FONT_WIDTH + SPACING_X) * (i - 13) / 2; // 13 = 14-1 (skipping invisible slot outside circle)
		y = ORIGIN_Y + (FONT_HEIGHT + SPACING_Y) * 2 + h + (h/6);    
  } else { // bottom side filler for round
		w = FONT_WIDTH/2;
		h = FONT_HEIGHT/2;
    if (i%2) {
      x = ORIGIN_X + FONT_WIDTH + SPACING_X + FONT_WIDTH + SPACING_X;
    } else {
      x = ORIGIN_X - w - SPACING_X/2; // todo: find correct value
    }
		y = ORIGIN_Y + (FONT_HEIGHT + SPACING_Y) * 2;
  }
	return GRect(x, y, w, h);
}

static GColor8 getSlotColor(int x, int y, int digit, int pos) {
  int argb;
  bool should_add_var = false;
  if (FONT[digit][y][x] == 0) {
    if (contrastmode) {
      return GColorBlack;
    }
    return BACKGROUND_COLOR;
  } else if (FONT[digit][y][x] == 1) {
    #if defined(PBL_COLOR)
      if (contrastmode && pos >= 8) {
        argb = 0b11000000;
      } else {
        argb = contrastmode ? 0b11111111 : NUMBER_BASE_COLOR_ARGB8;
        should_add_var = contrastmode ? false : NUMBER_ADD_VARIATION;
      }
    #elif defined(PBL_BW)
      if (!INVERT) {
        argb = 0b11111111;
      } else {
        argb = 0b11000000;
      }
    #endif
  } else {
    #if defined(PBL_COLOR)
      argb = contrastmode ? 0b11000001 : ORNAMENT_BASE_COLOR_ARGB8;
      should_add_var = contrastmode ? false : ORNAMENT_ADD_VARIATION;
    #elif defined(PBL_BW)
      if (GREYS) {
        argb = 0b11010101;
      } else {
        if (!INVERT) {
          argb = 0b11111111;
        } else {
          argb = 0b11000000;
        }
      }
    #endif
  }
  if (should_add_var) {
    argb += variation[ ( y*5 + x + digit*17 + pos*19 )%sizeof(variation) ];
  }
  if (pos >= 8) {
    int argb_temp = shadowtable[alpha & argb];
    if (argb_temp == BACKGROUND_COLOR.argb) {
      argb_temp = argb;
    }
    argb = argb_temp;
  }
  GColor8 color = { .argb = argb };
  return color;
}

static void updateSlot(Layer *layer, GContext *ctx) {
	int t, tx, ty, w, shift, total, tilesize, widthadjust;
	total = TOTALBLOCKS;
	widthadjust = 0;
	digitSlot *slot = *(digitSlot**)layer_get_data(layer);

	if (slot->divider == 2) {
		widthadjust = 1;
	}
	tilesize = TILE_SIZE/slot->divider;
	uint32_t skewedNormTime = slot->normTime*3;
	GRect r;
	
  graphics_context_set_fill_color(ctx, contrastmode ? GColorBlack : BACKGROUND_COLOR);
	r = layer_get_bounds(slot->layer);
	graphics_fill_rect(ctx, GRect(0, 0, r.size.w, r.size.h), 0, GCornerNone);
	for (t=0; t<total; t++) {
		w = 0;
		tx = t % FONT_WIDTH_BLOCKS;
		ty = t / FONT_HEIGHT_BLOCKS;
		shift = 0-(t-ty);
    
    GColor8 oldColor = getSlotColor(tx, ty, slot->prevDigit, slot->slotIndex);
    GColor8 newColor = getSlotColor(tx, ty, slot->curDigit, slot->slotIndex);
    
	  graphics_context_set_fill_color(ctx, oldColor);
    graphics_fill_rect(ctx, GRect((tx*tilesize)-(tx*widthadjust), ty*tilesize-(ty*widthadjust), tilesize-widthadjust, tilesize-widthadjust), 0, GCornerNone);
		
    if(!gcolor_equal(oldColor, newColor)) {
      w = (skewedNormTime*TILE_SIZE/ANIMATION_NORMALIZED_MAX)+shift-widthadjust;
   		if (w < 0) {
  			w = 0;
  		} else if (w > tilesize-widthadjust) {
  			w = tilesize-widthadjust;
  		}
      graphics_context_set_fill_color(ctx, newColor);
      graphics_fill_rect(ctx, GRect((tx*tilesize)-(tx*widthadjust), ty*tilesize-(ty*widthadjust), w, tilesize-widthadjust), 0, GCornerNone);
    }
	}
}

static unsigned short get_display_hour(unsigned short hour) {
    if (clock_is_24h_style()) {
        return hour;
    }
    unsigned short display_hour = hour % 12;
    return display_hour ? display_hour : 12;
}

static void setupAnimation() {
  if (debug) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Setting up anim");
  }
  anim = animation_create();
	animation_set_delay(anim, 0);
	animation_set_duration(anim, contrastmode ? 500 : DIGIT_CHANGE_ANIM_DURATION);
	animation_set_implementation(anim, &animImpl);
  animation_set_curve(anim, AnimationCurveEaseInOut);
  if (debug) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Done setting up anim %i", (int)anim);
  }
}

static void destroyAnimation() {
  if (debug) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Destroying anim %i", (int)anim);
  }
  animation_destroy(anim);
  anim = NULL;
}

void handle_tick(struct tm *t, TimeUnits units_changed) {
	int ho, mi, da, mo, i;

  if (splashEnded) {
    if (animation_is_scheduled(anim)){
      animation_unschedule(anim);
      animation_destroy(anim);
    }
    ho = get_display_hour(t->tm_hour);
    mi = t->tm_min;
    da = t->tm_mday;
    mo = t->tm_mon+1;
    if (debug) {
      //ho = 9;
    }
    
    if (debug) {
      APP_LOG(APP_LOG_LEVEL_INFO, "It is now %d:%d %d.%d.", (int)ho, (int)mi, (int)da, (int)mo);
    }
    
    allow_animate = true;
    if (DISABLE_ANIM) {
      if (DISABLE_ANIM_START_TIME == DISABLE_ANIM_END_TIME) {
        allow_animate = false;
        if (debug) {
          APP_LOG(APP_LOG_LEVEL_INFO, "No Animation because activation and deactivation times are the same");
        }
      } else if (DISABLE_ANIM_START_TIME > DISABLE_ANIM_END_TIME) {
        // across midnight
        if (ho >= DISABLE_ANIM_START_TIME || ho < DISABLE_ANIM_END_TIME) {
          allow_animate = false;
          if (debug) {
            APP_LOG(APP_LOG_LEVEL_INFO, "No Animation because time is between %d:00 and %d:00", (int)DISABLE_ANIM_START_TIME , (int)DISABLE_ANIM_END_TIME );
          }
        }
      } else {
        // prior to midnight
        if (ho >= DISABLE_ANIM_START_TIME && ho < DISABLE_ANIM_END_TIME) {
          allow_animate = false;
          if (debug) {
            APP_LOG(APP_LOG_LEVEL_INFO, "No Animation because time is between %d:00 and %d:00", (int)DISABLE_ANIM_START_TIME , (int)DISABLE_ANIM_END_TIME );
          }
        }
      }
    }

    for (i=0; i<NUMSLOTS; i++) {
      slot[i].prevDigit = slot[i].curDigit;
    }

    slot[0].curDigit = ho/10;
    slot[1].curDigit = ho%10;
    slot[2].curDigit = mi/10;
    slot[3].curDigit = mi%10;
    if (US_DATE) {
      slot[4].curDigit = mo/10;
      slot[5].curDigit = mo%10;
      if (CENTER_DATE && da < 10) {
        slot[6].curDigit = da%10;
        if (slot[7].prevDigit == 10 || slot[7].prevDigit == 12) {
          slot[7].curDigit = 11;
        } else {
          slot[7].curDigit = 10;
        }
      } else {
        slot[6].curDigit = da/10;
        slot[7].curDigit = da%10;
      }
    } else {
      slot[4].curDigit = da/10;
      slot[5].curDigit = da%10;
      if (CENTER_DATE && mo < 10) {
        slot[6].curDigit = mo%10;
        if (slot[7].prevDigit == 10 || slot[7].prevDigit == 12) {
          slot[7].curDigit = 11;
        } else {
          slot[7].curDigit = 10;
        }
      } else {
        slot[6].curDigit = mo/10;
        slot[7].curDigit = mo%10;
      }
    }

    if (NO_ZERO) {
      if (debug) {
        APP_LOG(APP_LOG_LEVEL_INFO, "Slot 0 was %d", (int) slot[0].prevDigit);
      }
      if (ho < 10 && slot[0].prevDigit >= 10) {
        if (NUMSLOTS > 8) {
          if (debug) {
            APP_LOG(APP_LOG_LEVEL_INFO, "More than 8 slots");
          }
          if (slot[10].prevDigit != 10 && slot[10].prevDigit < 12) {
            slot[0].curDigit = 11;
          } else {
            slot[0].curDigit = 10;
          }
        } else {
          if (debug) {
            APP_LOG(APP_LOG_LEVEL_INFO, "8 slots or fewer");
          }
          if (slot[0].prevDigit == 10) {
            slot[0].curDigit = 11;
          } else {
            slot[0].curDigit = 10;
          }
        }
        if (debug) {
          APP_LOG(APP_LOG_LEVEL_INFO, "Slot 0 is now %d", (int) slot[0].curDigit);
        }
      }
      if (slot[4].curDigit == 0) {
        slot[4].curDigit = 10;
        if (slot[4].prevDigit == 10) {
          slot[4].curDigit++;
        }
      }
      if (slot[6].curDigit == 0) {
        slot[6].curDigit = 10;
        if (slot[6].prevDigit == 10) {
          slot[6].curDigit++;
        }
      }
    }
    if (NUMSLOTS > 8) {
      for(int dig = 8; dig < NUMSLOTS; dig++) {
        if (slot[dig].prevDigit == 10 || slot[dig].prevDigit == 12) {
          slot[dig].curDigit = 11;
        } else {
          slot[dig].curDigit = 10;
        }
      }
    }
    setupAnimation();
    animation_schedule(anim);
  }
}

void handle_timer(void *data) {
	time_t curTime;
	
  splashEnded = true;

  curTime = time(NULL);
  handle_tick(localtime(&curTime), SECOND_UNIT|MINUTE_UNIT|HOUR_UNIT|DAY_UNIT|MONTH_UNIT|YEAR_UNIT);
}


void initSlot(int i, Layer *parent) {
	digitSlot *s = &slot[i];
	
  s->slotIndex = i;
	s->normTime = ANIMATION_NORMALIZED_MAX;
	s->prevDigit = startDigit[i];
	s->curDigit = startDigit[i];
	if ((i<4 || i>=8) && i<14) {
		s->divider = 1;
	} else {
		s->divider = 2;
	}
	s->layer = layer_create_with_data(slotFrame(i), sizeof(digitSlot *));
	*(digitSlot **)layer_get_data(s->layer) = s;

	layer_set_update_proc(s->layer, updateSlot);
	layer_add_child(parent, s->layer);
}

static void deinitSlot(int i) {
	layer_destroy(slot[i].layer);
}

static void animateDigits(struct Animation *anim, const AnimationProgress normTime) {
	int i;
	for (i=0; i<NUMSLOTS; i++) {
		if (slot[i].curDigit != slot[i].prevDigit) {
      if (allow_animate) {
        slot[i].normTime = normTime;
      } else {
        slot[i].normTime = ANIMATION_NORMALIZED_MAX;
      }
			layer_mark_dirty(slot[i].layer);
		}
	}
}

static void setupUI() {
  Layer *rootLayer;
	int i;
  
  window_set_background_color(window, contrastmode ? GColorBlack : BACKGROUND_COLOR);
	
	window_stack_push(window, true);

	rootLayer = window_get_root_layer(window);

	for (i=0; i<NUMSLOTS; i++) {
		initSlot(i, rootLayer);
	}

	animImpl.setup = NULL;
	animImpl.update = animateDigits;
	animImpl.teardown = destroyAnimation;

	setupAnimation();

	app_timer_register(contrastmode ? 0 : STARTDELAY, handle_timer, NULL);
}

static void teardownUI() {
	int i;
	for (i=0; i<NUMSLOTS; i++) {
		deinitSlot(i);
	}
	
	animation_destroy(anim);
}

static void battery_handler(BatteryChargeState charge_state) {
  #if defined(PBL_COLOR)
  if (CONTRAST_WHILE_CHARGING) {
    previous_contrastmode = contrastmode;
    if (charge_state.is_plugged) {
      contrastmode = true;
    } else {
      contrastmode = false;
    }
    if (previous_contrastmode != contrastmode) {
      teardownUI();
      setupUI();
    }
  }
  #endif
  if (LIGHT_WHILE_CHARGING) {
    if (charge_state.is_plugged) {
      light_enable(true);
    } else {
      light_enable(false);
    }
  }
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *large_mode_t = dict_find(iter, KEY_LARGE_MODE);
  Tuple *eu_date_t = dict_find(iter, KEY_EU_DATE);
  Tuple *quick_start_t = dict_find(iter, KEY_QUICK_START);
  Tuple *leading_zero_t = dict_find(iter, KEY_LEADING_ZERO);
  Tuple *background_color_t = dict_find(iter, KEY_BACKGROUND_COLOR);
  Tuple *number_base_color_t = dict_find(iter, KEY_NUMBER_BASE_COLOR);
  Tuple *number_variation_t = dict_find(iter, KEY_NUMBER_VARIATION);
  Tuple *ornament_base_color_t = dict_find(iter, KEY_ORNAMENT_BASE_COLOR);
  Tuple *ornament_variation_t = dict_find(iter, KEY_ORNAMENT_VARIATION);
  Tuple *invert_t = dict_find(iter, KEY_INVERT);
  Tuple *monochrome_t = dict_find(iter, KEY_MONOCHROME);
  Tuple *center_t = dict_find(iter, KEY_CENTER);
  Tuple *btvibe_t = dict_find(iter, KEY_BTVIBE);
  Tuple *contrast_t = dict_find(iter, KEY_CONTRAST);
  Tuple *nightsaver_t = dict_find(iter, KEY_NIGHTSAVER);
  Tuple *ns_start_t = dict_find(iter, KEY_NS_START);
  Tuple *ns_stop_t = dict_find(iter, KEY_NS_STOP);
  Tuple *backlight_t = dict_find(iter, KEY_BACKLIGHT);
  Tuple *debug_t = dict_find(iter, KEY_DEBUGWATCH);
  
  bool was_config = true;
  
  if (debug_t) {
    if (debug_t->value->int8 == 1) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Enabling debug infos for debug watch");
      debug = true;
      was_config = false;
    }
  }
  
  if (was_config) {
  
  if (large_mode_t) {
    curPrefs.large_mode =             large_mode_t->value->int8;
  }
  if (eu_date_t) {
    curPrefs.eu_date =                eu_date_t->value->int8;
  }
  if (quick_start_t) {
    curPrefs.quick_start =            quick_start_t->value->int8;
  }
  if (leading_zero_t) {
    curPrefs.leading_zero =           leading_zero_t->value->int8;
  }
  if (background_color_t) {
    curPrefs.background_color =       background_color_t->value->int32;
  }
  if (number_base_color_t) {
    curPrefs.number_base_color =      number_base_color_t->value->int32;
  }
  if (number_variation_t) {
    curPrefs.number_variation =       number_variation_t->value->int8;
  }
  if (ornament_base_color_t) {
    curPrefs.ornament_base_color =    ornament_base_color_t->value->int32;
  }
  if (ornament_variation_t) {
    curPrefs.ornament_variation =     ornament_variation_t->value->int8;
  }
  if (invert_t) {
    curPrefs.invert =                 invert_t->value->int8;
  }
  if (monochrome_t) {
    curPrefs.monochrome =             monochrome_t->value->int8;
  }
  if (center_t) {
    curPrefs.center =                 center_t->value->int8;
  }
  if (btvibe_t) {
    curPrefs.btvibe =                 btvibe_t->value->int8;
  }
  if (contrast_t) {
    curPrefs.contrast =               contrast_t->value->int8;
  }
  if (nightsaver_t) {
    curPrefs.nightsaver =             nightsaver_t->value->int8;
  }
  if (ns_start_t) {
    curPrefs.ns_start =               ns_start_t->value->int8;
  }
  if (ns_stop_t) {
    curPrefs.ns_stop =                ns_stop_t->value->int8;
  }
  if (backlight_t) {
    curPrefs.backlight =              backlight_t->value->int8;
  }
  persist_write_data(PREFERENCES_KEY, &curPrefs, sizeof(curPrefs));
  vibes_short_pulse();
  #if defined(PBL_COLOR)
  if (curPrefs.contrast == false) {
    contrastmode = false;
    previous_contrastmode = false;
  } else {
    BatteryChargeState charge_state = battery_state_service_peek();
    if (charge_state.is_plugged) {
      contrastmode = true;
      previous_contrastmode = true;
    }
  }
  #endif
  if (curPrefs.backlight == false) {
    light_enable(false);
  } else {
    BatteryChargeState charge_state = battery_state_service_peek();
    if (charge_state.is_plugged) {
      light_enable(true);
    }
  }
  if (debug) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Tearing down");
  }
  teardownUI();
  if (debug) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Setting up");
  }
  setupUI();
  if (debug) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Done");
  }
  
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Dropped a message because %i", (int)reason);
}


static void init() {  
  window = window_create();
  
  // Set up preferences
  if(persist_exists(PREFERENCES_KEY)){
    persist_read_data(PREFERENCES_KEY, &curPrefs, sizeof(curPrefs));
  } else {
    curPrefs = (Preferences) {
      .large_mode = false,
      .eu_date = false,
      .quick_start = false,
      .leading_zero = false,
      .background_color = 0b11000000,
      .number_base_color = 0b11001010,
      .number_variation = true,
      .ornament_base_color = 0b11100010,
      .ornament_variation = true,
      .invert = false,
      .monochrome = false,
      .center = false,
      .btvibe = false,
      .contrast = false,
      .nightsaver = false,
      .ns_start = 0,
      .ns_stop = 6,
      .backlight = false,
    };
  }
  
  setupUI();
  
  BatteryChargeState charge_state = battery_state_service_peek();
  
  if (charge_state.is_plugged) {
    #if defined(PBL_COLOR)
    if (CONTRAST_WHILE_CHARGING) {
      previous_contrastmode = true;
      contrastmode = true;
      teardownUI();
      setupUI();
    }
    #endif
    if (LIGHT_WHILE_CHARGING) {
      light_enable(true);
    }
  }
  
  // Setup app message
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_open(200,0);
	
	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  
  handle_bluetooth(connection_service_peek_pebble_app_connection());
  
  battery_state_service_subscribe(battery_handler);
  
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });
}

static void deinit() {
	tick_timer_service_unsubscribe();
  connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  teardownUI();
  window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}