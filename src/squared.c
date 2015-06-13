/*
 * Original source code by LastFuture
 * SDK 2.0beta4 port by Jnm
 * SDK 3.0 port and colorizing by hexahedria
 */

#include <pebble.h>

Window *window;

#define US_DATE true // true == MM/DD, false == DD/MM
#define NO_ZERO true // true == replaces leading Zero for hour, day, month with a "cycler"
#define TILE_SIZE 10
#define NUMSLOTS 8
#define SPACING_X TILE_SIZE
#define SPACING_Y TILE_SIZE
#define DIGIT_CHANGE_ANIM_DURATION 1700
#define STARTDELAY 2000

#define NUMBER_BASE_COLOR_ARGB8   0b11001010
#define ORNAMENT_BASE_COLOR_ARGB8 0b11100010
#define ADD_VARIATION true // true == colors are randomly shifted

#define FONT blocks // blocks or clean. clean == numbers with no ornaments
	
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
}};
unsigned char clean[][5][5] =  {{
	{1,1,1,1,1},
	{1,0,0,0,1},
	{1,0,0,0,1},
	{1,0,0,0,1},
	{1,1,1,1,1}
}, {
	{0,0,0,1,1},
	{0,0,0,0,1},
	{0,0,0,0,1},
	{0,0,0,0,1},
	{0,0,0,0,1}
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
	{1,0,0,0,1},
	{1,0,0,0,1},
	{1,1,1,1,1},
	{0,0,0,0,1},
	{0,0,0,0,1}
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
	{0,0,0,0,1},
	{0,0,0,0,1},
	{0,0,0,0,1}
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
	{2,2,2,2,2},
	{2,2,2,2,2},
	{2,2,2,2,2},
	{2,2,2,2,2}
}, {
	{2,2,2,2,2},
	{2,2,2,2,2},
	{2,2,2,2,2},
	{2,2,2,2,2},
	{2,2,2,2,2}
}};

int startDigit[NUMSLOTS] = {
	11,10,10,11,11,10,10,11
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

#define FONT_HEIGHT_BLOCKS (sizeof *FONT / sizeof **FONT)
#define FONT_WIDTH_BLOCKS (sizeof **FONT)
#define TOTALBLOCKS FONT_HEIGHT_BLOCKS * FONT_WIDTH_BLOCKS
#define FONT_HEIGHT FONT_HEIGHT_BLOCKS*TILE_SIZE
#define FONT_WIDTH FONT_WIDTH_BLOCKS*TILE_SIZE

#define SCREEN_WIDTH 144

#define TILES_X ( \
    FONT_WIDTH + SPACING_X + FONT_WIDTH)
#define TILES_Y ( \
    FONT_HEIGHT + SPACING_Y + FONT_HEIGHT)

#define ORIGIN_X ((SCREEN_WIDTH - TILES_X)/2)
#define ORIGIN_Y TILE_SIZE*1.5
	
static GRect slotFrame(int i) {
	int x, y, w, h;
	if (i<4) {
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
	} else {
		w = FONT_WIDTH/2;
		h = FONT_HEIGHT/2;
		x = ORIGIN_X + (FONT_WIDTH + SPACING_X) * (i - 4) / 2;
		y = ORIGIN_Y + (FONT_HEIGHT + SPACING_Y) * 2;
	}
	return GRect(x, y, w, h);
}

static GColor8 getSlotColor(int x, int y, int digit, int pos) {
  int argb;
  if (FONT[digit][y][x] == 0) {
    return GColorBlack;
  } else if (FONT[digit][y][x] == 1) {
    argb = NUMBER_BASE_COLOR_ARGB8;
  } else {
    argb = ORNAMENT_BASE_COLOR_ARGB8;
  }
  if (ADD_VARIATION) {
    argb += variation[ ( y*5 + x + digit*17 + pos*19 )%sizeof(variation) ];
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
	
	graphics_context_set_fill_color(ctx, GColorBlack);
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
  anim = animation_create();
	animation_set_delay(anim, 0);
	animation_set_duration(anim, DIGIT_CHANGE_ANIM_DURATION);
	animation_set_implementation(anim, &animImpl);
}

static void destroyAnimation() {
  animation_destroy(anim);
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

    for (i=0; i<NUMSLOTS; i++) {
      slot[i].prevDigit = slot[i].curDigit;
    }

    slot[0].curDigit = ho/10;
    slot[1].curDigit = ho%10;
    slot[2].curDigit = mi/10;
    slot[3].curDigit = mi%10;
    if (US_DATE) {
      slot[6].curDigit = da/10;
      slot[7].curDigit = da%10;
      slot[4].curDigit = mo/10;
      slot[5].curDigit = mo%10;
    } else {
      slot[4].curDigit = da/10;
      slot[5].curDigit = da%10;
      slot[6].curDigit = mo/10;
      slot[7].curDigit = mo%10;
    }

    if (NO_ZERO) {
      if (slot[0].curDigit == 0) {
        slot[0].curDigit = 10;
        if (slot[0].prevDigit == 10) {
          slot[0].curDigit++;
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
	s->prevDigit = 0;
	s->curDigit = startDigit[i];
	if (i<4) {
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
			slot[i].normTime = normTime;
			layer_mark_dirty(slot[i].layer);
		}
	}
}

static void init() {
	Layer *rootLayer;
	int i;

	window = window_create();
	window_set_background_color(window, GColorBlack);
	window_stack_push(window, true);

	rootLayer = window_get_root_layer(window);

	for (i=0; i<NUMSLOTS; i++) {
		initSlot(i, rootLayer);
	}

	animImpl.setup = NULL;
	animImpl.update = animateDigits;
	animImpl.teardown = destroyAnimation;

	setupAnimation();

	app_timer_register(STARTDELAY, handle_timer, NULL);
	
	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
}

static void deinit() {
	int i;

	tick_timer_service_unsubscribe();

	for (i=0; i<NUMSLOTS; i++) {
		deinitSlot(i);
	}
	
	animation_destroy(anim);
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}