#ifndef DISPLAY_H
#define DISPLAY_H

#include "Adafruit_GFX.h"
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

// LCD parameters
#define LCD_WIDTH		320
#define LCD_HEIGHT	172

// UI parameters
#define MAIN_FILL_HEIGHT	130

#define PRESET_NUM_FONT			FreeSansBold18pt7b
#define STANDARD_PRESET_FONT	FreeSansBold18pt7b
#define LARGE_PRESET_FONT		FreeSansBold24pt7b

// Colours
#define GEN_LOSS_BLUE	0xa6fd

void display_Init();
void display_DrawMainScreen();


#endif // DISPLAY_H