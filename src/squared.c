/*
 * Original source code by lastfuture
 * SDK 2.0beta4 port by Jnm
 * SDK 3.0 port and colorizing by hexahedria
 * adaptations for Chalk and Aplite as well as continued development since SDK 3.8.2 by lastfuture
 */

#include <pebble.h>

Window *window;

typedef struct {
  bool large_mode;
  bool eu_date;
  bool quick_start;
  bool leading_zero;
  uint8_t background_color;
  uint8_t number_base_color;
  bool number_variation;
  uint8_t ornament_base_color;
  bool ornament_variation;
  bool invert;
  bool monochrome;
  bool center;
  bool btvibe;
  bool contrast;
  bool nightsaver;
  uint8_t ns_start;
  uint8_t ns_stop;
  bool backlight;
  bool weekday;
  uint8_t bottomrow;
  uint8_t wristflick;
  uint16_t stepgoal;
  bool cheeky;
} Preferences;

Preferences curPrefs;



typedef struct {
  uint8_t daynum;
  uint8_t monthnum;
  uint8_t weekdaychar1;
  uint8_t weekdaychar2;
} Date;

Date curDate;


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
  KEY_WEEKDAY,
  KEY_BOTTOMROW,
  KEY_WRISTFLICK,
  KEY_STEPGOAL,
  KEY_CHEEKY,
};

#define KEY_DEBUGWATCH 50

#define PREFERENCES_KEY 0

#define SCREENSHOTMODE false

#define DEBUG false
#define SUPERDEBUG false

#define BOTTOMROW (curPrefs.bottomrow)
#define WRISTFLICK (curPrefs.wristflick)
#define STEP_GOAL (curPrefs.stepgoal)
#define CHEEKY_REMARKS (curPrefs.cheeky)

#define EU_DATE (curPrefs.eu_date) // true == MM/DD, false == DD/MM
#define WEEKDAY (curPrefs.weekday)
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
#define DIGIT_CHANGE_ANIM_DURATION (curPrefs.quick_start ? 1500 : 2000)
#define STARTDELAY (curPrefs.quick_start ? 700 : 2000)

#define NUMBER_BASE_COLOR_ARGB8   (curPrefs.number_base_color)
#define ORNAMENT_BASE_COLOR_ARGB8 (curPrefs.ornament_base_color)
#define NUMBER_ADD_VARIATION      (curPrefs.number_variation)
#define ORNAMENT_ADD_VARIATION    (curPrefs.ornament_variation)
#define BACKGROUND_COLOR    PBL_IF_BW_ELSE((INVERT ? GColorWhite : GColorBlack), ((GColor8) { .argb = curPrefs.background_color }))

#define FONT_WIDTH_BLOCKS 5
#define FONT_HEIGHT_BLOCKS 5
#define TOTALBLOCKS FONT_WIDTH_BLOCKS*FONT_HEIGHT_BLOCKS
#define FONT_HEIGHT FONT_WIDTH_BLOCKS*TILE_SIZE
#define FONT_WIDTH FONT_HEIGHT_BLOCKS*TILE_SIZE

#define TILES_X ( \
    FONT_WIDTH + SPACING_X + FONT_WIDTH)
#define TILES_Y ( \
    FONT_HEIGHT + SPACING_Y + FONT_HEIGHT)

#define ORIGIN_X PBL_IF_RECT_ELSE(((144 - TILES_X)/2), ((180 - TILES_X)/2))
#define ORIGIN_Y PBL_IF_RECT_ELSE((curPrefs.large_mode ? 1 : TILE_SIZE*1.5), (TILE_SIZE*2.2))



typedef struct {
	Layer             *layer;
  bool              mirror;
	uint8_t           prevDigit;
	uint8_t           curDigit;
	uint8_t           divider;
	AnimationProgress normTime;
  uint8_t           slotIndex;
} digitSlot;

digitSlot slot[NUMSLOTS];

static char weekday_buffer[2];

AnimationImplementation animImpl;
Animation *anim;
static bool splashEnded = false, debug = false, in_shake_mode = false, prev_chargestate = false;
static uint16_t stepprogress = 0;
static uint8_t battprogress = 0;

static const char * locales[6] = {"en", "de", "es", "fr", "it", "pt"};
static const char * weekdays[6][7] =  {
{ "SU","MO","TU","WE","TH","FR","SA" }, // EN
{ "SO","MO","DI","MI","DO","FR","SA" }, // DE
{ "DO","LU","MA","MI","JU","VI","SA" }, // ES - from https://forums.getpebble.com/discussion/comment/166975/#Comment_166975
{ "DI","LU","MA","ME","JE","VE","SA" }, // FR - from https://www.quora.com/How-are-days-of-the-week-abbreviated-in-various-languages
{ "DO","LU","MA","ME","GI","VE","SA" }, // IT - from https://www.quora.com/How-are-days-of-the-week-abbreviated-in-various-languages
{ "DO","SG","TE","QA","QI","SX","SA" }  // PT - from http://www.brazil-help.com/week.htm (is this correct?)
}; // required Letters: ADEFGHIJLMOQRSTUVWX

// character_map[] maps 0-9 (digits), 10-13 (ornaments), ascii codes of uppercase letters and 100-109 (progress indicators) to characters[]
static const uint8_t character_map[] = {
// DIGITS
[0] = 0,
[1] = 1,
[2] = 2,
[3] = 3,
[4] = 4,
[5] = 5,
[6] = 6,
[7] = 7,
[8] = 8,
[9] = 9,
// ORNAMENTS
[10] = 10,
[11] = 11,
[12] = 12,
[13] = 13,
// PUNCTUATION
[14] = 14,
[15] = 15,
[16] = 16,
[17] = 13, // same as 13
[37] = 17,
[42] = 18,
// UPPERCASE ASCII CHARACTERS
[65] = 19,
[66] = 20,
[67] = 21,
[68] = 22,
[69] = 23,
[70] = 24,
[71] = 25,
[72] = 26,
[73] = 27,
[74] = 28,
[75] = 29,
[76] = 30,
[77] = 31,
[78] = 32,
[79] = 0, // same as 0
[80] = 33,
[81] = 34,
[82] = 35,
[83] = 36,
[84] = 37,
[85] = 38,
[86] = 39,
[87] = 40,
[88] = 41,
[89] = 42,
// PROGRESS
[100] = 10, // same as ornament 10
[101] = 43,
[102] = 44,
[103] = 45,
[104] = 46,
[105] = 47,
[106] = 48,
[107] = 49,
[108] = 50,
[109] = 13, // same as ornament 13
[110] = 11, // same as ornament 11
[111] = 51,
[112] = 52,
[113] = 53,
[114] = 54,
[115] = 55,
[116] = 56,
[117] = 57,
[118] = 58,
[119] = 12, // same as ornament 12
};

// left column is digit color, right column is ornament color
static const uint8_t characters[][10] =  {
// DIGITS
{
  0b11111, 0b00000,
  0b10001, 0b00000,
  0b10001, 0b00100,
  0b10001, 0b00000,
  0b11111, 0b00000
},
{
  0b00011, 0b11000,
  0b00001, 0b00000,
  0b00001, 0b11100,
  0b00001, 0b00000,
  0b00001, 0b11100
},
{
  0b11111, 0b00000,
  0b00001, 0b00000,
  0b11111, 0b00000,
  0b10000, 0b00000,
  0b11111, 0b00000
},
{
  0b11111, 0b00000,
  0b00001, 0b00000,
  0b01111, 0b00000,
  0b00001, 0b00000,
  0b11111, 0b00000
},
{
  0b10001, 0b00100,
  0b10001, 0b00000,
  0b11111, 0b00000,
  0b00001, 0b00000,
  0b00001, 0b11100
},
{
  0b11111, 0b00000,
  0b10000, 0b00000,
  0b11111, 0b00000,
  0b00001, 0b00000,
  0b11111, 0b00000
},
{
  0b11111, 0b00000,
  0b10000, 0b00000,
  0b11111, 0b00000,
  0b10001, 0b00000,
  0b11111, 0b00000
},
{
  0b11111, 0b00000,
  0b00001, 0b00000,
  0b00001, 0b10100,
  0b00001, 0b10100,
  0b00001, 0b10100
},
{
  0b11111, 0b00000,
  0b10001, 0b00000,
  0b11111, 0b00000,
  0b10001, 0b00000,
  0b11111, 0b00000
},
{
  0b11111, 0b00000,
  0b10001, 0b00000,
  0b11111, 0b00000,
  0b00001, 0b00000,
  0b11111, 0b00000
},
// ORNAMENTS
{
  0b00000, 0b11111,
  0b00000, 0b00000,
  0b00000, 0b11111,
  0b00000, 0b00000,
  0b00000, 0b11111
},
{
  0b00000, 0b10101,
  0b00000, 0b10101,
  0b00000, 0b10101,
  0b00000, 0b10101,
  0b00000, 0b10101
},
{
  0b11111, 0b00000,
  0b00000, 0b00000,
  0b11111, 0b00000,
  0b00000, 0b00000,
  0b11111, 0b00000
},
{
  0b10101, 0b00000,
  0b10101, 0b00000,
  0b10101, 0b00000,
  0b10101, 0b00000,
  0b10101, 0b00000
},
// PUNCTUATION
{ // •••
  0b00000, 0b10101,
  0b00000, 0b10101,
  0b00000, 0b00000,
  0b10101, 0b00000,
  0b10101, 0b00000
},
{ // •••
  0b00000, 0b10101,
  0b00000, 0b00000,
  0b10101, 0b00000,
  0b10101, 0b00000,
  0b10101, 0b00000
},
{ // •••
  0b00000, 0b00000,
  0b10101, 0b00000,
  0b10101, 0b00000,
  0b10101, 0b00000,
  0b10101, 0b00000
},
{ // %
  0b10001, 0b00000,
  0b00010, 0b00000,
  0b00100, 0b00000,
  0b01000, 0b00000,
  0b10001, 0b00000
},
{ // *
  0b10101, 0b00000,
  0b01110, 0b00000,
  0b11111, 0b00000,
  0b01110, 0b00000,
  0b10101, 0b00000
},
// UPPERCASE ASCII CHARACTERS
{ // A
  0b11111, 0b00000,
  0b10001, 0b00000,
  0b11111, 0b00000,
  0b10001, 0b00000,
  0b10001, 0b00100
},
{ // B
  0b11110, 0b00000,
  0b10001, 0b00000,
  0b11110, 0b00000,
  0b10001, 0b00000,
  0b11110, 0b00000
},
{ // C
  0b11111, 0b00000,
  0b10000, 0b00000,
  0b10000, 0b00111,
  0b10000, 0b00000,
  0b11111, 0b00000
},
{ // D
  0b11111, 0b00000,
  0b10001, 0b00000,
  0b10001, 0b00100,
  0b10001, 0b00000,
  0b11110, 0b00000
},
{ // E
  0b11111, 0b00000,
  0b10000, 0b00000,
  0b11110, 0b00000,
  0b10000, 0b00000,
  0b11111, 0b00000
},
{ // F
  0b11111, 0b00000,
  0b10000, 0b00000,
  0b11110, 0b00000,
  0b10000, 0b00000,
  0b10000, 0b00111
},
{ // G
  0b11111, 0b00000,
  0b10000, 0b00000,
  0b10111, 0b00000,
  0b10001, 0b00000,
  0b11111, 0b00000
},
{ // H
  0b10001, 0b00100,
  0b10001, 0b00000,
  0b11111, 0b00000,
  0b10001, 0b00000,
  0b10001, 0b00100
},
{ // I
  0b11111, 0b00000,
  0b00100, 0b00000,
  0b00100, 0b10001,
  0b00100, 0b00000,
  0b11111, 0b00000
},
{ // J
  0b00111, 0b10000,
  0b00001, 0b00000,
  0b00001, 0b11100,
  0b00001, 0b00000,
  0b11111, 0b00000
},
{ // K
  0b10001, 0b00000,
  0b10010, 0b00000,
  0b11100, 0b00000,
  0b10010, 0b00000,
  0b10001, 0b00000
},
{ // L
  0b10000, 0b00101,
  0b10000, 0b00101,
  0b10000, 0b00101,
  0b10000, 0b00000,
  0b11111, 0b00000
},
{ // M
  0b11111, 0b00000,
  0b10101, 0b00000,
  0b10101, 0b00000,
  0b10001, 0b00000,
  0b10001, 0b00100
},
{ // N
  0b10001, 0b00000,
  0b11001, 0b00000,
  0b10101, 0b00000,
  0b10011, 0b00000,
  0b10001, 0b00000
},
// O is same as 0
{ // P
  0b11111, 0b00000,
  0b10001, 0b00000,
  0b11111, 0b00000,
  0b10000, 0b00000,
  0b10000, 0b00111
},
{ // Q
  0b11111, 0b00000,
  0b10001, 0b00000,
  0b10001, 0b00000,
  0b10101, 0b00000,
  0b11111, 0b00000
},
{ // R
  0b11111, 0b00000,
  0b00001, 0b00000,
  0b11111, 0b00000,
  0b10010, 0b00000,
  0b10001, 0b00100
},
{ // S
  0b11111, 0b00000,
  0b10000, 0b00000,
  0b11111, 0b00000,
  0b00001, 0b00000,
  0b11111, 0b00000
},
{ // T
  0b11111, 0b00000,
  0b00100, 0b00000,
  0b00100, 0b10001,
  0b00100, 0b10001,
  0b00100, 0b10001
},
{ // U
  0b10001, 0b00100,
  0b10001, 0b00100,
  0b10001, 0b00100,
  0b10001, 0b00000,
  0b11111, 0b00000
},
{ // V
  0b10001, 0b00100,
  0b10001, 0b00100,
  0b10001, 0b00100,
  0b10001, 0b00000,
  0b11110, 0b00000
},
{ // W
  0b10001, 0b00100,
  0b10001, 0b00000,
  0b10101, 0b00000,
  0b10101, 0b00000,
  0b11111, 0b00000
},
{ // X
  0b10001, 0b00000,
  0b01010, 0b00000,
  0b00100, 0b00000,
  0b01010, 0b00000,
  0b10001, 0b00000
},
{ // Y
  0b10001, 0b00000,
  0b10001, 0b00000,
  0b11111, 0b00000,
  0b00001, 0b00000,
  0b11111, 0b00000
},
// PROGRESS
// 01% is same as ornament 10
{ // 12%
  0b00000, 0b11111,
  0b00000, 0b00000,
  0b00000, 0b11111,
  0b00000, 0b00000,
  0b10000, 0b00111
},
{ // 23%
  0b00000, 0b11111,
  0b00000, 0b00000,
  0b10000, 0b00111,
  0b10000, 0b00000,
  0b10000, 0b00111
},
{ // 34%
  0b10000, 0b00111,
  0b10000, 0b00000,
  0b10000, 0b00111,
  0b10000, 0b00000,
  0b10000, 0b00111
},
{ // 45%
  0b10000, 0b00111,
  0b10000, 0b00000,
  0b10000, 0b00111,
  0b10000, 0b00000,
  0b10100, 0b00001
},
{ // 56%
  0b10000, 0b00111,
  0b10000, 0b00000,
  0b10100, 0b00001,
  0b10100, 0b00000,
  0b10100, 0b00001
},
{ // 67%
  0b10100, 0b00001,
  0b10100, 0b00000,
  0b10100, 0b00001,
  0b10100, 0b00000,
  0b10100, 0b00001
},
{ // 78%
  0b10100, 0b00001,
  0b10100, 0b00000,
  0b10100, 0b00001,
  0b10100, 0b00000,
  0b10101, 0b00000
},
{ // 89%
  0b10100, 0b00001,
  0b10100, 0b00000,
  0b10101, 0b00000,
  0b10101, 0b00000,
  0b10101, 0b00000
},
// 100% is same as ornament 13

// PROGRESS 2
// 01% is same as ornament 11
{ // 12%
  0b00000, 0b10101,
  0b00000, 0b10101,
  0b00000, 0b10101,
  0b00000, 0b00101,
  0b10000, 0b00101
},
{ // 23%
  0b00000, 0b10101,
  0b00000, 0b10101,
  0b00000, 0b10101,
  0b00000, 0b00001,
  0b11100, 0b00001
},
{ // 34%
  0b00000, 0b10101,
  0b00000, 0b10101,
  0b00000, 0b10101,
  0b00000, 0b00000,
  0b11111, 0b00000
},
{ // 45%
  0b00000, 0b10101,
  0b00000, 0b00101,
  0b10000, 0b00101,
  0b00000, 0b00000,
  0b11111, 0b00000
},
{ // 56%
  0b00000, 0b10101,
  0b00000, 0b00001,
  0b11100, 0b00001,
  0b00000, 0b00000,
  0b11111, 0b00000
},
{ // 67%
  0b00000, 0b10101,
  0b00000, 0b00000,
  0b11111, 0b00000,
  0b00000, 0b00000,
  0b11111, 0b00000
},
{ // 78%
  0b10000, 0b00101,
  0b00000, 0b00000,
  0b11111, 0b00000,
  0b00000, 0b00000,
  0b11111, 0b00000
},
{ // 89%
  0b11100, 0b00001,
  0b00000, 0b00000,
  0b11111, 0b00000,
  0b00000, 0b00000,
  0b11111, 0b00000
}
// 100% is same as ornament 12
};

static const uint8_t progress_top_seq[19] = {
  0, 10, 20, 30, 31, 32, 33, 43, 53, 63, 64, 65, 66, 76, 86, 96, 97, 98, 99
};

static const uint8_t startDigit[18] = {
	11,12,12,11,11,12,10,13,12,11,12,11,11,12,10,13,12,10 // 2x h, 2x m, 4x date, 2x filler top, 4x filler sides, 2x filler bottom, 2x filler bottom sides
};

static const uint8_t variation[100] = {
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

static const uint8_t shadowtable[256] = {
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
static const uint8_t alpha = 0b10111111;

static bool contrastmode = false, allow_animate = true, initial_anim = false;

#if defined(PBL_COLOR)
static bool previous_contrastmode = false;
#endif

static void handle_bluetooth(bool connected) {
  if (DISCONNECT_VIBRATION && !connected) {
    static const uint32_t segments[] = { 200, 200, 50, 150, 200 };
    VibePattern pat = {
    	.durations = segments,
    	.num_segments = ARRAY_LENGTH(segments),
    };
    vibes_enqueue_custom_pattern(pat);
  }
}

static GRect slotFrame(int8_t i) {
	int16_t x, y, w, h;
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
    y = (int8_t) (ORIGIN_Y - FONT_HEIGHT - SPACING_Y);
  } else if (i<14) { // side filler for round
    w = FONT_WIDTH;
		h = FONT_HEIGHT;
    if (i%2) {
			x = ORIGIN_X + FONT_WIDTH + SPACING_X + FONT_WIDTH + SPACING_X;
		} else {
			x = (int8_t) (ORIGIN_X - FONT_WIDTH - SPACING_X);
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

static uint8_t fetchrect(uint8_t digit, uint8_t x, uint8_t y, bool mirror) {
  // character_map maps 0-9 (digits), 10-13 (ornaments), ascii codes of uppercase letters and 100-109 (progress) to characters[]
  uint8_t color1 = characters[character_map[digit]][(y*2)]; // get one row of digit colors
  uint8_t color2 = characters[character_map[digit]][(y*2)+1]; // get one row of ornament colors
  uint8_t mask = 0b10000 >> x;
  if (mirror) {
    mask = 0b00001 << x;
  }
  if (color1 & mask) { // check column of row
    return 1;
  } else if (color2 & mask) {
    return 2;
  } else {
    return 0;
  }
}

static GColor8 getSlotColor(uint8_t x, uint8_t y, uint8_t digit, uint8_t pos, bool mirror) {
  static uint8_t argb;
  static bool should_add_var = false;
  uint8_t thisrect = fetchrect(digit, x, y, mirror);
  if (SCREENSHOTMODE) {
    thisrect = 1;
  }
  if (thisrect == 0) {
    if (contrastmode) {
      return GColorBlack;
    }
    return BACKGROUND_COLOR;
  } else if (thisrect == 1) {
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
    if (argb == 0b11111111) {
      argb -= variation[ ( y*5 + x + digit*17 + pos*19 )%sizeof(variation) ];
    } else {
      argb += variation[ ( y*5 + x + digit*17 + pos*19 )%sizeof(variation) ];
    }
  }
  if (pos >= 8) {
    uint8_t argb_temp = shadowtable[alpha & argb];
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
    
    GColor8 oldColor = getSlotColor(tx, ty, slot->prevDigit, slot->slotIndex, slot->mirror);
    GColor8 newColor = getSlotColor(tx, ty, slot->curDigit, slot->slotIndex, slot->mirror);
    
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

static unsigned short get_display_hour(uint8_t hour) {
    if (clock_is_24h_style()) {
        return hour;
    }
    uint8_t display_hour = hour % 12;
    return display_hour ? display_hour : 12;
}

static void setupAnimation() {
  anim = animation_create();
	animation_set_delay(anim, 0);
	animation_set_duration(anim, contrastmode ? 500 : in_shake_mode ? DIGIT_CHANGE_ANIM_DURATION/2 : DIGIT_CHANGE_ANIM_DURATION);
	animation_set_implementation(anim, &animImpl);
  animation_set_curve(anim, AnimationCurveEaseInOut);
  if (debug) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Set up anim %i", (int)anim);
  }
}

static void destroyAnimation() {
  if (debug) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Destroying anim %i", (int)anim);
  }
  animation_destroy(anim);
  anim = NULL;
}

static void setProgressSlots(uint16_t progress, bool showgoal, bool bottom) {
  static uint8_t digits[4];
  static uint8_t progressoffset;
  uint8_t mappedProgress;
  if (bottom) {
    progressoffset = 100;
    mappedProgress = (((progress+3)*0.95*40)/100);
    digits[0] = 4;
    digits[1] = 5;
    digits[2] = 6;
    digits[3] = 7;
    BatteryChargeState charge_state;
    if (showgoal && progress >= 102) {
      uint16_t input = progress;
      uint16_t hundreds = input/100;
      input -= (hundreds)*100;
      uint8_t tens = input/10;
      input -= (tens)*10;
      uint8_t units=input; 
      slot[digits[0]].curDigit = hundreds;
      slot[digits[1]].curDigit = tens;
      slot[digits[2]].curDigit = units;
      slot[digits[3]].curDigit = '%';
    } else if (!showgoal && progress >= 100) {
      slot[digits[0]].curDigit = progressoffset+9;
      slot[digits[1]].curDigit = progressoffset+9;
      slot[digits[2]].curDigit = progressoffset+9;
      slot[digits[3]].curDigit = progressoffset+9;
    } else if (showgoal && progress >= 100) {
      slot[digits[0]].curDigit = 'G';
      slot[digits[1]].curDigit = 'O';
      slot[digits[2]].curDigit = 'A';
      slot[digits[3]].curDigit = 'L';
    } else {
      for(uint8_t dig = 0; dig < sizeof digits; dig++) {
        slot[digits[dig]].curDigit = 100;
      }
      uint8_t partialSegment = progressoffset+mappedProgress%10;
      slot[digits[0]].curDigit = partialSegment;
      if (mappedProgress >= 10) {
        slot[digits[0]].curDigit = progressoffset+9;
        slot[digits[1]].curDigit = partialSegment;
      }
      if (mappedProgress >= 20) {
        slot[digits[1]].curDigit = progressoffset+9;
        slot[digits[2]].curDigit = partialSegment;
      }
      if (mappedProgress >= 30) {
        slot[digits[2]].curDigit = progressoffset+9;
        slot[digits[3]].curDigit = partialSegment;
      }
      if (mappedProgress >= 40) {
        slot[digits[3]].curDigit = progressoffset+9;
      }
    }
    if (BOTTOMROW == 1) {
      charge_state = battery_state_service_peek();
      if (charge_state.is_charging) {
        slot[digits[0]].curDigit = 14;
        slot[digits[1]].curDigit = 15;
        slot[digits[2]].curDigit = 16;
        slot[digits[3]].curDigit = 17;
      }
    }
  } else {
    progressoffset = 110;
    mappedProgress = (((progress+3)*0.92*40)/100);
    if (!showgoal || (showgoal && progress < 100)) {
      uint8_t front_digit = progress_top_seq[mappedProgress%20]/10;
      uint8_t back_digit = progress_top_seq[mappedProgress%20]%10;
      if (mappedProgress<19) {
        slot[0].curDigit = progressoffset;
        slot[1].curDigit = progressoffset;
        slot[2].curDigit = progressoffset+front_digit;
        slot[3].curDigit = progressoffset+back_digit;
      } else {
        slot[0].curDigit = progressoffset+front_digit;
        slot[1].curDigit = progressoffset+back_digit;
        slot[2].curDigit = progressoffset+9;
        slot[3].curDigit = progressoffset+9;
      }
      if (progress >= 100) {
        slot[0].curDigit = progressoffset+9;
        slot[1].curDigit = progressoffset+9;
        slot[2].curDigit = progressoffset+9;
        slot[3].curDigit = progressoffset+9;
      }
    } else if (showgoal && progress > 999) {
      slot[4].curDigit = 9;
      slot[5].curDigit = 9;
      slot[6].curDigit = 9;
      slot[7].curDigit = '%';
    } else if (showgoal && progress >= 100) {
      uint16_t input = progress;
      uint16_t hundreds=input/100; 
      input-=(hundreds)*100; 
      uint8_t tens=input/10; 
      input-=(tens)*10; 
      uint8_t units=input; 
      slot[4].curDigit = hundreds;
      slot[5].curDigit = tens;
      slot[6].curDigit = units;
      slot[7].curDigit = '%';
    }
    APP_LOG(APP_LOG_LEVEL_INFO, "Cheeky mode is %d", CHEEKY_REMARKS);
    if (CHEEKY_REMARKS && showgoal && progress >= 999) {
      slot[0].curDigit = 'F';
      slot[1].curDigit = '*';
      slot[2].curDigit = 'C';
      slot[3].curDigit = 'K';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 750) {
      slot[0].curDigit = 'Y';
      slot[1].curDigit = 'O';
      slot[2].curDigit = 'L';
      slot[3].curDigit = 'O';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 500) {
      slot[0].curDigit = 'W';
      slot[1].curDigit = 'H';
      slot[2].curDigit = 'A';
      slot[3].curDigit = 'T';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 400) {
      slot[0].curDigit = 'T';
      slot[1].curDigit = 'I';
      slot[2].curDigit = 'L';
      slot[3].curDigit = 'T';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 300) {
      slot[0].curDigit = 'O';
      slot[1].curDigit = 'M';
      slot[2].curDigit = 'F';
      slot[3].curDigit = 'G';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 250) {
      slot[0].curDigit = 'H';
      slot[1].curDigit = 'O';
      slot[2].curDigit = 'L';
      slot[3].curDigit = 'Y';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 200) {
      slot[0].curDigit = 'G';
      slot[1].curDigit = 'A';
      slot[2].curDigit = 'S';
      slot[3].curDigit = 'P';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 175) {
      slot[0].curDigit = 'D';
      slot[1].curDigit = 'A';
      slot[2].curDigit = 'N';
      slot[3].curDigit = 'G';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 150) {
      slot[0].curDigit = 'W';
      slot[1].curDigit = 'H';
      slot[2].curDigit = 'O';
      slot[3].curDigit = 'A';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 130) {
      slot[0].curDigit = 'S';
      slot[1].curDigit = 'W';
      slot[2].curDigit = 'A';
      slot[3].curDigit = 'G';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 115) {
      slot[0].curDigit = 'C';
      slot[1].curDigit = 'O';
      slot[2].curDigit = 'O';
      slot[3].curDigit = 'L';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 105) {
      slot[0].curDigit = 'Y';
      slot[1].curDigit = 'E';
      slot[2].curDigit = 'A';
      slot[3].curDigit = 'H';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 100) {
      slot[0].curDigit = 'G';
      slot[1].curDigit = 'O';
      slot[2].curDigit = 'A';
      slot[3].curDigit = 'L';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 78) {
      slot[4].curDigit = 'N';
      slot[5].curDigit = 'I';
      slot[6].curDigit = 'C';
      slot[7].curDigit = 'E';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 62) {
      slot[4].curDigit = 'N';
      slot[5].curDigit = 'E';
      slot[6].curDigit = 'A';
      slot[7].curDigit = 'T';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 45) {
      slot[4].curDigit = 'G';
      slot[5].curDigit = 'O';
      slot[6].curDigit = 'O';
      slot[7].curDigit = 'D';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 28) {
      slot[4].curDigit = 'O';
      slot[5].curDigit = 'K';
      slot[6].curDigit = 'A';
      slot[7].curDigit = 'Y';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 16) {
      slot[4].curDigit = 'W';
      slot[5].curDigit = 'E';
      slot[6].curDigit = 'L';
      slot[7].curDigit = 'L';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 12) {
      slot[4].curDigit = 'A';
      slot[5].curDigit = 'H';
      slot[6].curDigit = 'E';
      slot[7].curDigit = 'M';
    } else if (CHEEKY_REMARKS && showgoal && progress >= 8) {
      slot[4].curDigit = 'L';
      slot[5].curDigit = 'A';
      slot[6].curDigit = 'M';
      slot[7].curDigit = 'E';
    } else if (CHEEKY_REMARKS && showgoal) {
      slot[4].curDigit = 'O';
      slot[5].curDigit = 'U';
      slot[6].curDigit = 'C';
      slot[7].curDigit = 'H';
    } else if (!CHEEKY_REMARKS && showgoal) {
      slot[4].curDigit = 'S';
      slot[5].curDigit = 'T';
      slot[6].curDigit = 'E';
      slot[7].curDigit = 'P';
    } else {
      slot[4].curDigit = 'B';
      slot[5].curDigit = 'A';
      slot[6].curDigit = 'T';
      slot[7].curDigit = 'T';
    }
  }
}

static void update_step_goal() {
  #if defined(PBL_HEALTH)
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  // Check the metric has data available for today
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, 
                                                                         start, end);
  if(mask & HealthServiceAccessibilityMaskAvailable) {
    // Data is available!
    stepprogress = (uint16_t)(((float)health_service_sum_today(metric)/(float)STEP_GOAL)*100);
    APP_LOG(APP_LOG_LEVEL_INFO, "Step progress: %d%%", stepprogress);
  } else {
    // No data recorded yet today
    APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
  }
  #endif
}

static void handle_tick(struct tm *t, TimeUnits units_changed) {
	static uint8_t ho, mi, da, mo;

  if (splashEnded && !initial_anim) {
    if (animation_is_scheduled(anim)){
      animation_unschedule(anim);
      animation_destroy(anim);
    }
    ho = get_display_hour(t->tm_hour);
    mi = t->tm_min;
    da = t->tm_mday;
    mo = t->tm_mon+1;
    if (debug && SUPERDEBUG) {
      ho = 8+(mi%4);
    }
    
    uint8_t localeid = 0;
    static char weekdayname[3];
    static char locale[3];
    strncpy(locale, i18n_get_system_locale(), 2);
    if (WEEKDAY) {
      strftime(weekday_buffer, sizeof(weekday_buffer), "%w", t);
      for (uint8_t lid = 0; lid < 6; lid++) {
        if (strncmp(locales[lid], locale, 2) == 0) { localeid = lid; }
      }
      uint8_t weekdaynum = ((int)weekday_buffer[0])-0x30;
      if (debug && SUPERDEBUG) {
        weekdaynum = (int)mi%7;
      }
      strcpy(weekdayname, weekdays[localeid][weekdaynum]);
    }
    
    allow_animate = true;
    if (DISABLE_ANIM) {
      if (DISABLE_ANIM_START_TIME == DISABLE_ANIM_END_TIME) {
        allow_animate = false;
        if (debug) {
          APP_LOG(APP_LOG_LEVEL_INFO, "Animation always off");
        }
      } else if (DISABLE_ANIM_START_TIME > DISABLE_ANIM_END_TIME) {
        // across midnight
        if (t->tm_hour >= DISABLE_ANIM_START_TIME || t->tm_hour < DISABLE_ANIM_END_TIME) {
          allow_animate = false;
          if (debug) {
            APP_LOG(APP_LOG_LEVEL_INFO, "Animation off (%d:00 - %d:00)", (int)DISABLE_ANIM_START_TIME , (int)DISABLE_ANIM_END_TIME );
          }
        }
      } else {
        // prior to midnight
        if (t->tm_hour >= DISABLE_ANIM_START_TIME && t->tm_hour < DISABLE_ANIM_END_TIME) {
          allow_animate = false;
          if (debug) {
            APP_LOG(APP_LOG_LEVEL_INFO, "Animation off (%d:00 - %d:00)", (int)DISABLE_ANIM_START_TIME , (int)DISABLE_ANIM_END_TIME );
          }
        }
      }
    }

    for (uint8_t i=0; i<NUMSLOTS; i++) {
      slot[i].prevDigit = slot[i].curDigit;
    }

    slot[0].curDigit = ho/10;
    slot[1].curDigit = ho%10;
    slot[2].curDigit = mi/10;
    slot[3].curDigit = mi%10;
    
    if (BOTTOMROW == 2) {
      update_step_goal();
      setProgressSlots(stepprogress, true, true);
    } else if (BOTTOMROW == 1) {
      BatteryChargeState charge_state = battery_state_service_peek();
      battprogress = charge_state.charge_percent;
      setProgressSlots(battprogress, false, true);
    } else {
      if (!EU_DATE) {
        if (WEEKDAY) {
          slot[4].curDigit = (uint8_t) weekdayname[0];
          slot[5].curDigit = (uint8_t) weekdayname[1];
        } else {
          slot[4].curDigit = mo/10;
          slot[5].curDigit = mo%10;
        }
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
        if (WEEKDAY) {
          slot[6].curDigit = (uint8_t) weekdayname[0];
          slot[7].curDigit = (uint8_t) weekdayname[1];
        } else {
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
      }
    }
    
    if (NO_ZERO) {
      if (slot[0].curDigit == 0) {
        if (NUMSLOTS > 8) {
          if (slot[10].prevDigit != 10 && slot[10].prevDigit != 12) {
            slot[0].curDigit = 11;
          } else {
            slot[0].curDigit = 10;
          }
        } else {
          if (slot[0].prevDigit == 10) {
            slot[0].curDigit = 11;
          } else {
            slot[0].curDigit = 10;
          }
        }
      }
      if (BOTTOMROW == 0) {
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

static void initialAnimationDone() {
  initial_anim = false;
}


void handle_timer(void *data) {
  splashEnded = true;
  time_t curTime = time(NULL);
  handle_tick(localtime(&curTime), SECOND_UNIT|MINUTE_UNIT|HOUR_UNIT|DAY_UNIT|MONTH_UNIT|YEAR_UNIT);
	in_shake_mode = false;
  initial_anim = true;
  app_timer_register(contrastmode ? 500 : in_shake_mode ? DIGIT_CHANGE_ANIM_DURATION/2 : DIGIT_CHANGE_ANIM_DURATION, initialAnimationDone, NULL);

}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (WRISTFLICK > 0) {
    if (!in_shake_mode) {
      for (uint8_t i=0; i<NUMSLOTS; i++) {
        slot[i].prevDigit = slot[i].curDigit;
      }
      if (WRISTFLICK == 2) {
        update_step_goal();
        setProgressSlots(stepprogress, true, false);
      } else if (WRISTFLICK == 1) {
        BatteryChargeState charge_state = battery_state_service_peek();
        battprogress = charge_state.charge_percent;
        setProgressSlots(battprogress, false, false); // only show "GOAL" if PERCENTAGE is STEP_PERCENTAGE
      }
      in_shake_mode = true;
      setupAnimation();
      animation_schedule(anim);
      app_timer_register(3000, handle_timer, NULL);
    }
  }
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

static void deinitSlot(uint8_t i) {
	layer_destroy(slot[i].layer);
}

static void animateDigits(struct Animation *anim, const AnimationProgress normTime) {
	for (uint8_t i=0; i<NUMSLOTS; i++) {
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
  
  window_set_background_color(window, contrastmode ? GColorBlack : BACKGROUND_COLOR);
	
	window_stack_push(window, true);

	rootLayer = window_get_root_layer(window);

	for (uint8_t i=0; i<NUMSLOTS; i++) {
		initSlot(i, rootLayer);
	}

	animImpl.setup = NULL;
	animImpl.update = animateDigits;
	animImpl.teardown = destroyAnimation;

	setupAnimation();

	app_timer_register(contrastmode ? 0 : STARTDELAY, handle_timer, NULL);
}

static void teardownUI() {
	for (uint8_t i=0; i<NUMSLOTS; i++) {
		deinitSlot(i);
	}
	
	animation_destroy(anim);
}

static void battery_handler(BatteryChargeState charge_state) {
  if (BOTTOMROW == 1 || WRISTFLICK == 1) {
    battprogress = charge_state.charge_percent;
  }
  #if defined(PBL_COLOR)
  if (CONTRAST_WHILE_CHARGING) {
    previous_contrastmode = contrastmode;
    if (charge_state.is_plugged) {
      contrastmode = true;
    } else {
      contrastmode = false;
    }
    if (previous_contrastmode != contrastmode) {
      window_set_background_color(window, contrastmode ? GColorBlack : BACKGROUND_COLOR);
      app_timer_register(0, handle_timer, NULL);
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
  if (prev_chargestate != charge_state.is_plugged) {
    window_set_background_color(window, contrastmode ? GColorBlack : BACKGROUND_COLOR);
    app_timer_register(0, handle_timer, NULL);
  }
  prev_chargestate = charge_state.is_plugged;
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
  Tuple *weekday_t = dict_find(iter, KEY_WEEKDAY);
  Tuple *bottomrow_t = dict_find(iter, KEY_BOTTOMROW);
  Tuple *wristflick_t = dict_find(iter, KEY_WRISTFLICK);
  Tuple *stepgoal_t = dict_find(iter, KEY_STEPGOAL);
  Tuple *debug_t = dict_find(iter, KEY_DEBUGWATCH);
  Tuple *cheeky_t = dict_find(iter, KEY_CHEEKY);
  
  if (debug_t) {
    if (debug_t->value->int8 == 1) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Setting debug watch");
      debug = true;
    }
  }
  
  uint8_t old_largemode = curPrefs.large_mode;
  
  if (debug) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Got config");
  }
  if (large_mode_t) {          curPrefs.large_mode =             large_mode_t->value->int8; }
  if (eu_date_t) {             curPrefs.eu_date =                eu_date_t->value->int8; }
  if (quick_start_t) {         curPrefs.quick_start =            quick_start_t->value->int8; }
  if (leading_zero_t) {        curPrefs.leading_zero =           leading_zero_t->value->int8; }
  if (background_color_t) {    curPrefs.background_color =       background_color_t->value->int8; }
  if (number_base_color_t) {   curPrefs.number_base_color =      number_base_color_t->value->int8; }
  if (number_variation_t) {    curPrefs.number_variation =       number_variation_t->value->int8; }
  if (ornament_base_color_t) { curPrefs.ornament_base_color =    ornament_base_color_t->value->int8; }
  if (ornament_variation_t) {  curPrefs.ornament_variation =     ornament_variation_t->value->int8; }
  if (invert_t) {              curPrefs.invert =                 invert_t->value->int8; }
  if (monochrome_t) {          curPrefs.monochrome =             monochrome_t->value->int8; }
  if (center_t) {              curPrefs.center =                 center_t->value->int8; }
  if (btvibe_t) {              curPrefs.btvibe =                 btvibe_t->value->int8; }
  if (contrast_t) {            curPrefs.contrast =               contrast_t->value->int8; }
  if (nightsaver_t) {          curPrefs.nightsaver =             nightsaver_t->value->int8; }
  if (ns_start_t) {            curPrefs.ns_start =               ns_start_t->value->int8; }
  if (ns_stop_t) {             curPrefs.ns_stop =                ns_stop_t->value->int8; }
  if (backlight_t) {           curPrefs.backlight =              backlight_t->value->int8; }
  if (weekday_t) {             curPrefs.weekday =                weekday_t->value->int8; }
  if (bottomrow_t) {           curPrefs.bottomrow =              bottomrow_t->value->int8; }
  if (wristflick_t) {          curPrefs.wristflick =             wristflick_t->value->int8; }
  if (stepgoal_t) {            curPrefs.stepgoal =               stepgoal_t->value->int16; }
  if (cheeky_t) {              curPrefs.cheeky =                 cheeky_t->value->int8; }

  if (debug) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Writing config");
  }
  persist_write_data(PREFERENCES_KEY, &curPrefs, sizeof(curPrefs));
  if (debug) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Wrote config");
  }
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
  if (old_largemode == curPrefs.large_mode) {
    window_set_background_color(window, contrastmode ? GColorBlack : BACKGROUND_COLOR);
    app_timer_register(0, handle_timer, NULL);
  } else {
    teardownUI();
    setupUI();
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Dropped message because %i", (int)reason);
}


static void init() {
  window = window_create();
  
  if (DEBUG) {
    debug = true;
  }
  
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
      .weekday = false,
      .bottomrow = 0,
      .wristflick = 0,
      .stepgoal = 10000,
      .cheeky = true
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
  app_message_open(260,0);
	
	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  
  handle_bluetooth(connection_service_peek_pebble_app_connection());
  
  battery_state_service_subscribe(battery_handler);
  
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });
  
  accel_tap_service_subscribe(tap_handler);
}

static void deinit() {
	tick_timer_service_unsubscribe();
  connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  accel_tap_service_unsubscribe();
  teardownUI();
  window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}