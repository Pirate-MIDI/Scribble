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

#define PRESET_NUM_X_OFFSET		LCD_WIDTH - 96
#define PRESET_NUM_Y_OFFSET		30

#define BPM_X_OFFSET					10
#define BPM_Y_OFFSET					30

#define PRESET_NAME_X_OFFSET	10
#define PRESET_NAME_Y_OFFSET	120

#define PRESET_NUM_FONT			FreeSansBold18pt7b
#define BPM_FONT					FreeSansBold18pt7b
#define PRESET_NAME_FONT		FreeSansBold24pt7b
#define LARGE_PRESET_FONT		FreeSansBold24pt7b

// Colours
#define GEN_LOSS_BLUE	0xa6fd

void display_Init();
void display_DrawMainScreen();


#endif // DISPLAY_H