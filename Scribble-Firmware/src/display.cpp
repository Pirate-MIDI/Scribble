#include "Arduino.h"
#include	"hardware_def.h"
#include "Adafruit_ST7789.h"
#include "display.h"
#include "main.h"

static const char* DISPLAY_TAG = "DISPLAY";

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN);

uint16_t clockTempoColour = ST77XX_WHITE; // Default clock tempo colour

void display_Init()
{
	// Set the display brightness
	display_SetBrightness(globalSettings.displayBrightness);
	// Initialise the display
	SPI.begin(SPI_SCK_PIN, -1, SPI_MOSI_PIN, -1);
	lcd.init(172, 320);           // Init ST7789 172x320
	lcd.setRotation(3); // rotates the screen
  	
	// Default clock tempo colour
	if(globalSettings.uiLightMode == UI_MODE_DARK)
	{
		clockTempoColour = ST77XX_WHITE;
	}
	else if(globalSettings.uiLightMode == UI_MODE_LIGHT)
	{
		clockTempoColour = ST77XX_BLACK;
	}
	display_DrawMainScreen();
}

void display_SetBrightness(uint8_t value)
{
	// Set the display brightness
	analogWrite(LCD_BL_PIN, value);
}

void display_ConfigureNewDeviceScreen()
{
	int16_t  x1, y1;
	uint16_t w, h;
	int16_t yOffset;
	// Set the display brightness
	analogWrite(LCD_BL_PIN, 255);
	lcd.fillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, ST77XX_BLACK);
	lcd.setFont(&INFO_TEXT_FONT);
	lcd.setTextColor(ST77XX_WHITE);

	lcd.setCursor(PRESET_NAME_X_OFFSET, PRESET_NAME_Y_TOP_OFFSET);
	lcd.getTextBounds("Configuring default", PRESET_NAME_X_OFFSET, yOffset, &x1, &y1, &w, &h);
	lcd.setCursor(LCD_WIDTH/2 - w/2, PRESET_NAME_Y_TOP_OFFSET);
	lcd.print("Configuring default");

	lcd.setCursor(PRESET_NAME_X_OFFSET, PRESET_NAME_Y_BOTTOM_OFFSET);
	lcd.getTextBounds("device settings...", PRESET_NAME_X_OFFSET, yOffset, &x1, &y1, &w, &h);
	lcd.setCursor(LCD_WIDTH/2 - w/2, PRESET_NAME_Y_BOTTOM_OFFSET);
	lcd.print("device settings...");
}

void display_DrawMainScreen()
{
	// Draw main colour boxes
	if(globalSettings.uiLightMode == UI_MODE_DARK)
	{
		lcd.fillRect(0, 0, 320, LCD_HEIGHT-MAIN_FILL_HEIGHT, ST77XX_BLACK);
	}
	else if(globalSettings.uiLightMode == UI_MODE_LIGHT)
	{
		lcd.fillRect(0, 0, 320, LCD_HEIGHT-MAIN_FILL_HEIGHT, ST77XX_WHITE);
	}

	// Draw info bar
	display_DrawPresetNumber(globalSettings.currentPreset);
	//lcd.print(128);
	float bpmTest = 120.0;
	display_DrawBpm(bpmTest);

	// Connectivity states
	// Wireless state
	if(globalSettings.esp32ManagerConfig.wirelessType == Esp32WiFi)
	{
		display_DrawWirelessIndicator(globalSettings.esp32ManagerConfig.wirelessType, esp32Info.wifiConnected);
	}
	else if(globalSettings.esp32ManagerConfig.wirelessType == Esp32BLE)
	{
		display_DrawWirelessIndicator(globalSettings.esp32ManagerConfig.wirelessType, esp32Info.bleConnected);
	}
	// MIDI
	display_DrawMidiIndicator(false);

	// Draw main text
	display_DrawMainText(presets[globalSettings.currentPreset].name, presets[globalSettings.currentPreset].secondaryText);
}

// Draw a black rectangle around the preset number area to clear previous text
// Then write the new preset number
void display_DrawPresetNumber(uint16_t	 presetNumber)
{
	int16_t  x1, y1;
	uint16_t w, h;
	int16_t yOffset;
	if(globalSettings.uiLightMode == UI_MODE_DARK)
	{
		lcd.fillRect(LCD_WIDTH-100, 0, 100, INFO_BAR_HEIGHT, ST77XX_BLACK);
		lcd.setTextColor(ST77XX_WHITE);
	}
	else if(globalSettings.uiLightMode == UI_MODE_LIGHT)
	{
		lcd.fillRect(LCD_WIDTH-100, 0, 100, INFO_BAR_HEIGHT, ST77XX_WHITE);
		lcd.setTextColor(ST77XX_BLACK);
	}
	
	char presetNumString[4];
	sprintf(presetNumString, "%d", presetNumber + 1);

	lcd.setFont(&PRESET_NUM_FONT);
	lcd.setCursor(PRESET_NUM_X_OFFSET, PRESET_NUM_Y_OFFSET);
	lcd.getTextBounds(presetNumString, 0, PRESET_NUM_Y_OFFSET, &x1, &y1, &w, &h);
	lcd.setCursor((PRESET_NUM_X_OFFSET) - w, PRESET_NUM_Y_OFFSET);
	lcd.print(presetNumString);
}

// Draw a coloured rectangle around the preset number area to clear previous text
// Then write the new preset name and secondary text
void display_DrawBpm(float value)
{
	if(globalSettings.uiLightMode == UI_MODE_DARK)
	{
		lcd.fillRect(BPM_X_OFFSET, 0, 100, INFO_BAR_HEIGHT, ST77XX_BLACK);
	}
	else if(globalSettings.uiLightMode == UI_MODE_LIGHT)
	{
		lcd.fillRect(BPM_X_OFFSET, 0, 100, INFO_BAR_HEIGHT, ST77XX_WHITE);
	}
	lcd.setFont(&BPM_FONT);
	lcd.setTextColor(clockTempoColour);
	lcd.setCursor(BPM_X_OFFSET, BPM_Y_OFFSET);
	char bpmString[6];
	if(globalSettings.clockDisplayType == MIDI_CLOCK_DISPLAY_BPM)
	{
		sprintf(bpmString, "%.1f", value);
		lcd.print(bpmString);
	}
	else if(globalSettings.clockDisplayType == MIDI_CLOCK_DISPLAY_MS)
	{
		sprintf(bpmString, "%.0fms", (60000.0 / value));
		lcd.print(bpmString);
	}

	// Ignore for a flashing indicator as that is handled in the indicator task
}

void display_SetBpmDrawColour(uint16_t colour)
{
	clockTempoColour = colour;
}

void display_DrawMainText(const char* text, const char* secondaryText)
{
	int16_t  x1, y1;
	uint16_t w, h;
	int16_t yOffset;
	// Check to use the global colour or the preset override colour
	if(presets[globalSettings.currentPreset].colourOverrideFlag)
	{
		lcd.fillRect(0, LCD_HEIGHT-MAIN_FILL_HEIGHT, 320, MAIN_FILL_HEIGHT, presets[globalSettings.currentPreset].colourOverride);
	}
	else
	{
		lcd.fillRect(0, LCD_HEIGHT-MAIN_FILL_HEIGHT, 320, MAIN_FILL_HEIGHT, globalSettings.mainColour);
	}
	// Check for the preset text override colour
	if(presets[globalSettings.currentPreset].textColourOverrideFlag)
	{
		lcd.setTextColor(presets[globalSettings.currentPreset].textColourOverride);
	}
	else
	{
		lcd.setTextColor(globalSettings.textColour);
	}
	
	// Draw main text
	if(text != NULL)
	{
		// If no secondary text is used, centre the main text
		if(secondaryText == NULL)
			yOffset = PRESET_NAME_Y_CENTRE_OFFSET;

		else
			yOffset = PRESET_NAME_Y_TOP_OFFSET;

		lcd.setFont(&PRESET_NAME_FONT);
		lcd.setCursor(PRESET_NAME_X_OFFSET, yOffset);
		lcd.getTextBounds(text, PRESET_NAME_X_OFFSET, yOffset, &x1, &y1, &w, &h);
		lcd.setCursor(LCD_WIDTH/2 - w/2, yOffset);
		lcd.print(text);
	}
	// Draw secondary text if available
	if(secondaryText != NULL)
	{
		yOffset = PRESET_NAME_Y_BOTTOM_OFFSET;
		lcd.setFont(&SECONDARY_TEXT_FONT);
		lcd.setCursor(PRESET_NAME_X_OFFSET, yOffset);
		lcd.getTextBounds(secondaryText, PRESET_NAME_X_OFFSET, yOffset, &x1, &y1, &w, &h);
		lcd.setCursor(LCD_WIDTH/2 - w/2, yOffset);
		lcd.print(secondaryText);
	}
}

void display_DrawMidiIndicator(bool active)
{
	if(active)
	{
		lcd.fillCircle((LCD_WIDTH/2) - (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, MIDI_INDICATOR_COLOUR);
	}
	else
	{
		if(globalSettings.uiLightMode == UI_MODE_DARK)
		{
			lcd.fillCircle((LCD_WIDTH/2) - (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_BLACK);
		}
		else if(globalSettings.uiLightMode == UI_MODE_LIGHT)
		{
			lcd.fillCircle((LCD_WIDTH/2) - (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_WHITE);
		}
		lcd.drawCircle((LCD_WIDTH/2) - (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, MIDI_INDICATOR_COLOUR);
	}
}

// Type: 0 = is None, 1 = BLE, 2 = WiFi
// State: 0 = disconnected, 1 = connected, 2 = AP (WiFi only)
void display_DrawWirelessIndicator(uint8_t type, uint8_t state)
{
	if(type == Esp32BLE)
	{
		if(state == 0)
		{
			if(globalSettings.uiLightMode == UI_MODE_DARK)
			{
				lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_BLACK);
			}
			else if(globalSettings.uiLightMode == UI_MODE_LIGHT)
			{
				lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_WHITE);
			}
			lcd.drawCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, BLE_INDICATOR_COLOUR);
		}
		else if(state == 1)
		{
			lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, BLE_INDICATOR_COLOUR);
		}
	}
	else if(type == Esp32WiFi)
	{
		// Circle outline
		if(state == 0)
		{
			if(globalSettings.uiLightMode == UI_MODE_DARK)
			{
				lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_BLACK);
			}
			else if(globalSettings.uiLightMode == UI_MODE_LIGHT)
			{
				lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_WHITE);
			}
			lcd.drawCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, WIFI_INDICATOR_COLOUR);
		}
		// Solid circle
		else if(state == 1 || state == 2)
		{
			lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, WIFI_INDICATOR_COLOUR);
		}
		// Circle outline with dot in the middle
		else if(state == 3)
		{
			if(globalSettings.uiLightMode == UI_MODE_DARK)
			{
				lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_BLACK);
			}
			else if(globalSettings.uiLightMode == UI_MODE_LIGHT)
			{
				lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_WHITE);
			}
			lcd.drawCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, WIFI_INDICATOR_COLOUR);
			lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE/2, WIFI_INDICATOR_COLOUR);
		}
		
	}
	else if(type == Esp32None)
	{
		if(state == 0)
		{
			if(globalSettings.uiLightMode == UI_MODE_DARK)
			{
				lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_BLACK);
			}
			else if(globalSettings.uiLightMode == UI_MODE_LIGHT)
			{
				lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_WHITE);
			}
			
			lcd.drawCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_WHITE);
		}
		else if(state == 1)
		{
			lcd.fillCircle((LCD_WIDTH/2) + (CIRCLE_INDICATOR_SIZE + CIRCLE_INDIACTOR_X_OFFSET), (INFO_BAR_HEIGHT)/2, CIRCLE_INDICATOR_SIZE, ST77XX_WHITE);
		}
	}
}
