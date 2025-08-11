#include "Arduino.h"
#include	"hardware_def.h"
#include "Adafruit_ST7789.h"
#include "display.h"
#include "main.h"

static const char* DISPLAY_TAG = "DISPLAY";

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN);

void display_Init()
{
	SPI.begin(SPI_SCK_PIN, -1, SPI_MOSI_PIN, -1);
	lcd.init(172, 320);           // Init ST7789 172x320
	lcd.setRotation(3); // rotates the screen
  	display_DrawMainScreen();
}

void display_DrawMainScreen()
{
	int16_t  x1, y1;
	uint16_t w, h;

	// Draw main colour boxes
	lcd.fillRect(0, LCD_HEIGHT-MAIN_FILL_HEIGHT, 320, MAIN_FILL_HEIGHT, GEN_LOSS_BLUE);
	lcd.fillRect(0, 0, 320, LCD_HEIGHT-MAIN_FILL_HEIGHT, ST77XX_BLACK);

	// Draw info bar
	/*
	lcd.setFont(&PRESET_NUM_FONT);
	lcd.setTextColor(ST77XX_WHITE);
	lcd.setCursor(PRESET_NUM_X_OFFSET, PRESET_NUM_Y_OFFSET);
	char presetNumString[4];
	sprintf(presetNumString, "%d", globalSettings.currentPreset + 1);
	lcd.print(presetNumString);
	*/
	display_DrawPresetNumber(globalSettings.currentPreset);
	//lcd.print(128);

	lcd.setFont(&BPM_FONT);
	lcd.setTextColor(ST77XX_WHITE);
	lcd.setCursor(BPM_X_OFFSET, BPM_Y_OFFSET);
	char bpmString[6];
	float testBpm = 120.0; // Example BPM value
	sprintf(bpmString, "%.1f", testBpm);
	lcd.print(bpmString);

	// Connectivity states
	// Wireless state
	display_DrawWirelessIndicator(globalSettings.wirelessType, 0);
	// MIDI
	display_DrawMidiIndicator(false);	// WiFi

	// Draw main text
	if(globalSettings.useLargePresetFont)
	{
		lcd.setFont(&LARGE_PRESET_FONT);
	}
	else
	{
		lcd.setFont(&PRESET_NAME_FONT);
	}
	
	char testPresetName[] = {"Preset 1\n"};
	lcd.setTextColor(ST77XX_WHITE);
	lcd.setCursor(PRESET_NAME_X_OFFSET, PRESET_NAME_Y_OFFSET);
	lcd.getTextBounds(testPresetName, PRESET_NAME_X_OFFSET, PRESET_NAME_Y_OFFSET, &x1, &y1, &w, &h);
	lcd.setCursor(LCD_WIDTH/2 - w/2, PRESET_NAME_Y_OFFSET);
	lcd.print(testPresetName);
}

// Draw a black rectangle around the preset number area to clear previous text
// Then write the new preset number
void display_DrawPresetNumber(uint16_t	 presetNumber)
{
	lcd.fillRect(PRESET_NUM_X_OFFSET, 0, (LCD_WIDTH - (PRESET_NUM_X_OFFSET)), INFO_BAR_HEIGHT, ST77XX_BLACK);
	lcd.setFont(&PRESET_NUM_FONT);
	lcd.setTextColor(ST77XX_WHITE);
	lcd.setCursor(PRESET_NUM_X_OFFSET, PRESET_NUM_Y_OFFSET);
	char presetNumString[4];
	sprintf(presetNumString, "%d", presetNumber + 1);
	lcd.print(presetNumString);
}

void display_DrawMidiIndicator(bool active)
{
	if(active)
	{
		lcd.fillCircle((LCD_WIDTH/2) - (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, MIDI_INDICATOR_COLOUR);
	}
	else
	{
		lcd.fillCircle((LCD_WIDTH/2) - (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_BLACK);
		lcd.drawCircle((LCD_WIDTH/2) - (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, MIDI_INDICATOR_COLOUR);
	}
}

// Type: 0 = is None, 1 = BLE, 2 = WiFi
// State: 0 = disconnected, 1 = connected, 2 = AP (WiFi only)
void display_DrawWirelessIndicator(uint8_t type, uint8_t state)
{
	if(type == 1)
	{
		if(state == 0)
		{
			lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_BLACK);
			lcd.drawCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, BLE_INDICATOR_COLOUR);
		}
		else if(state == 1)
		{
			lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, BLE_INDICATOR_COLOUR);
		}
	}
}
