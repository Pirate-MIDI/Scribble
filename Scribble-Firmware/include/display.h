#ifndef DISPLAY_H
#define DISPLAY_H

#include "Adafruit_GFX.h"
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

// LCD parameters
#define LCD_WIDTH		320
#define LCD_HEIGHT	172

// UI parameters
#define MAIN_FILL_HEIGHT	120
#define INFO_BAR_HEIGHT		LCD_HEIGHT-MAIN_FILL_HEIGHT

#define PRESET_NUM_X_OFFSET		LCD_WIDTH - 70
#define PRESET_NUM_Y_OFFSET		35

#define BPM_X_OFFSET					10
#define BPM_Y_OFFSET					35

#define PRESET_NAME_X_OFFSET			10
#define PRESET_NAME_Y_OFFSET			120
#define PRESET_NAME_Y_LARGE_OFFSET	100

#define CIRCLE_INDICATOR_SIZE			14
#define CIRCLE_INDIACTOR_X_OFFSET	8

#define PRESET_NUM_FONT			FreeSansBold18pt7b
#define BPM_FONT					FreeSansBold18pt7b
#define PRESET_NAME_FONT		FreeSansBold24pt7b
#define LARGE_PRESET_FONT		FreeSansBold24pt7b

// Colours
#define GEN_LOSS_BLUE				0xa69b
#define MIDI_INDICATOR_COLOUR		0xfcc0
#define BLE_INDICATOR_COLOUR		0x059f
#define WIFI_INDICATOR_COLOUR		0xc01f

void display_Init();
void display_DrawMainScreen();
void display_DrawPresetNumber(uint16_t	 presetNumber);
void display_DrawBpm(float value);
void display_DrawMainText(const char* text, const char* secondaryText);
void display_DrawMidiIndicator(bool active);
void display_DrawWirelessIndicator(uint8_t type, uint8_t state);


#endif // DISPLAY_H