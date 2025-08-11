#include <Arduino.h>
#include "hardware_Def.h"
#include "esp32_Settings.h"
#include "main.h"
#include "display.h"
#include "midi_Handling.h"
#include "task_Priorities.h"

static const char* MAIN_TAG = "MAIN";

uint8_t wifiState = WIFI_STATE_NOT_CONNECTED;
float currentBpm = 120.0;
GlobalSettings globalSettings;
Preset presets[NUM_PRESETS];

void defaultGlobalSettingsAssignment();
void defaultPresetsAssignment();

void indicatorTask(void* parameter);

void setup()
{
	//Serial0.begin(115200);
	Serial.begin(115200);
	ESP_LOGI("MAIN", "Starting setup...");
	
	// Assign global and preset settings and boot the file system
	esp32Settings_AssignDefaultGlobalSettings(defaultGlobalSettingsAssignment);
	esp32Settings_AssignDefaultPresetSettings(defaultPresetsAssignment);
	esp32Settings_BootCheck(&globalSettings, sizeof(GlobalSettings), presets, sizeof(Preset), NUM_PRESETS, &globalSettings.bootState);

	// Click system tasks
	BaseType_t taskResult;
	taskResult = xTaskCreatePinnedToCore(
		indicatorTask, // Task function. 
		"LED Task", // name of task. 
		5000, // Stack size of task 
		NULL, // parameter of the task 
		LED_TASK_PRIORITY, // priority of the task 
		NULL, // Task handle to keep track of created task 
		1); // pin task to core 1 
	ESP_LOGI(MAIN_TAG, "LED task created: %d", taskResult);

	midi_Init();
	display_Init();
}

void loop()
{
	midi_ReadAll();
}


void defaultGlobalSettingsAssignment()
{
	// System settings
	globalSettings.bootState = 0; 						// Initial boot state
	globalSettings.currentPreset = 0; 					// Start with the first preset

	// UI settings
	globalSettings.uiLightMode = 0; 						// Auto dark mode
	globalSettings.mainColour = GEN_LOSS_BLUE;
	globalSettings.useLargePresetFont = 0;				// Use small text by default

	// MIDI settings
	globalSettings.midiChannel = MIDI_CHANNEL_OMNI; // Default MIDI channel
	globalSettings.clockMode = 0; 						// Use preset BPM by default
	globalSettings.globalBpm = 120.0; 					// Default global BPM
	globalSettings.midiOutMode = MIDI_OUT_TYPE_A; 	// Type A MIDI output by default
	// TRS MIDI thru flags
	globalSettings.midiThruFlags[MIDI_TRS][MIDI_TRS] = 1;
	globalSettings.midiThruFlags[MIDI_TRS][MIDI_BLE] = 1;
	globalSettings.midiThruFlags[MIDI_TRS][MIDI_WIFI] = 1;
	// BLE MIDI thru flags
	globalSettings.midiThruFlags[MIDI_BLE][MIDI_TRS] = 1;
	globalSettings.midiThruFlags[MIDI_BLE][MIDI_BLE] = 1;
	globalSettings.midiThruFlags[MIDI_BLE][MIDI_WIFI] = 1;
	// WiFi MIDI thru flags
	globalSettings.midiThruFlags[MIDI_WIFI][MIDI_TRS] = 1;
	globalSettings.midiThruFlags[MIDI_WIFI][MIDI_BLE] = 1;
	globalSettings.midiThruFlags[MIDI_WIFI][MIDI_WIFI] = 1;
}

void defaultPresetsAssignment()
{
	for (int i = 0; i < NUM_PRESETS; i++)
	{
		char str[64];
		sprintf(str, presets[i].name, "Preset %d", i + 1);
		sprintf(str, presets[i].secondaryText, "Secondary %d", i + 1);
		presets[i].colourOverrideFlag = 0; // Use main colour by default
		presets[i].colourOverride = GEN_LOSS_BLUE; // Default colour
		presets[i].bpm = 120.0; // Set default BPM
	}
}


void midi_ControlChangeHandler(byte channel, byte number, byte value)
{
	if(channel == globalSettings.midiChannel || globalSettings.midiChannel == MIDI_CHANNEL_OMNI)
	{
		switch(number)
		{
			case PRESET_UP_CC:
				// Increment preset index
				if(globalSettings.currentPreset < NUM_PRESETS - 1)
				{
					globalSettings.currentPreset++;
				}
				// Wrap around to the first preset
				else
				{
					globalSettings.currentPreset = 0;
				}
				break;
			case PRESET_DOWN_CC:
				// Decrement preset index
				if(globalSettings.currentPreset > 0)
				{
					globalSettings.currentPreset--;
				}
				// Wrap around to the last preset
				else
				{
					globalSettings.currentPreset = NUM_PRESETS - 1;
				}
				break;
			case PRESET_SELECT_CC:
				if(value < NUM_PRESETS)
				{
					globalSettings.currentPreset = value;
				}
				break;
			default:
				break;
		}
	}
}

void midi_ProgramChangeHandler(byte channel, byte number)
{
	if(number >= NUM_PRESETS)
	{
		ESP_LOGE(MAIN_TAG, "Invalid program change number: %d", number);
		return;
	}
	globalSettings.currentPreset = number;
	display_DrawPresetNumber(globalSettings.currentPreset);
}

void midi_SysExHandler(byte* data, unsigned length)
{

}

void midi_ClockHandler()
{

}

void indicatorTask(void* parameter)
{
	static uint16_t ledBreathingIndex = 0;
	static uint16_t ledBlinkingIndex = 0;
	while(1)
	{
		/*
		// Displaying wireless status information
				// WiFi  feedback
					// If in AP mode for the WiFi configuration portal, rapid breathing alt colour
					if(esp32Info.wifiConnected == 3)
					{
						// Breathing LED effect
						ledBreathingIndex += 8;
						if(ledBreathingIndex >= 256)
						{
							ledBreathingIndex = 0;
						}


						uint32_t breathingColour = pixel_ScaleColour(WIFI_CONFIG_COLOUR, sineTable[ledBreathingIndex]);
						leds.setPixelColor(i, breathingColour);
						leds.show();
					}
					// If connected to WiFi with internet access, breathing 
					else if(esp32Info.wifiConnected == 2)
					{
						// Breathing LED effect
						ledBreathingIndex += 2;
						if(ledBreathingIndex >= 256)
						{
							ledBreathingIndex = 0;
						}


						uint32_t breathingColour = pixel_ScaleColour(WIFI_CONNECTED_NET_COLOUR, sineTable[ledBreathingIndex]);
						leds.setPixelColor(i, breathingColour);
						leds.show();
					}
					// If connected to WiFi without internet access, blinking
					else if(esp32Info.wifiConnected == 1)
					{
						// Blinking LED effect
						ledBlinkingIndex += 2;
						if(ledBlinkingIndex >= 256)
						{
							ledBlinkingIndex = 0;
						}
						if(ledBlinkingIndex >= 127)
							leds.setPixelColor(i, WIFI_CONNECTED_NET_COLOUR);
						else
							leds.setPixelColor(i, 0x000000);
						leds.show();
					}
					// In WiFi mode but not connected to a network, solid red
					else if(esp32Info.wifiConnected == 0)
					{
						leds.setPixelColor(i, LED_NOT_CONNECTED_COLOUR);
						leds.show();
					}

				
				// BLE feedback
					// If connected to client (in server mode), breathing
					if(esp32Info.bleConnected == 1)
					{
						// Breathing LED effect
						ledBreathingIndex += 2;
						if(ledBreathingIndex >= 256)
						{
							ledBreathingIndex = 0;
						}
						uint32_t breathingColour = 0;
						if(esp32ConfigPtr->bleMode == Esp32BLEClient)
							breathingColour = pixel_ScaleColour(BLE_CLIENT_CONNECTED_COLOUR, sineTable[ledBreathingIndex]);

						else if(esp32ConfigPtr->bleMode == Esp32BLEServer)
							breathingColour = pixel_ScaleColour(BLE_SERVER_CONNECTED_COLOUR, sineTable[ledBreathingIndex]);

						leds.setPixelColor(i, breathingColour);
						leds.show();
					}
					// If not connected to a BLE client or server, solid red
					else if(esp32Info.bleConnected == 0)
					{
						leds.setPixelColor(i, LED_NOT_CONNECTED_COLOUR);
						leds.show();
					}
					*/
		if(bleMidiReceived)
		{
			display_DrawMidiIndicator(true);
			vTaskDelay(MIDI_INDICATOR_ON_TIME / portTICK_PERIOD_MS);
			display_DrawMidiIndicator(false);
			bleMidiReceived = 0;
		}
		if(newBleEvent)
		{
			if(bleConnected)
			{
				display_DrawWirelessIndicator(WIRELESS_MODE_BLE, 1); // BLE connected
			}
			else
			{
				display_DrawWirelessIndicator(WIRELESS_MODE_BLE, 0); // BLE disconnected
			}
			newBleEvent = 0;
		}
				
		vTaskDelay(50 / portTICK_PERIOD_MS);
	}
}