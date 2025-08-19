#include <Arduino.h>
#include "hardware_def.h"
#include "esp32_settings.h"
#include "main.h"
#include "display.h"
#include "midi_handling.h"
#include "task_priorities.h"
#include "esp_system.h"
#include "soc/rtc_cntl_reg.h"
#include "MIDI.h"
#include "device_api.h"

static const char* MAIN_TAG = "MAIN";

char deviceApiBuffer[8192];

int8_t bleRssi;
float currentBpm = 120.0;
GlobalSettings globalSettings;
Preset presets[NUM_PRESETS];

void defaultGlobalSettingsAssignment();
void defaultPresetsAssignment();

void indicatorTask(void* parameter);
void deviceApiTask(void* parameter);

//HWCDC USBSerial;

void setup()
{
	Serial.begin(115200);
	Serial.setRxBufferSize(8192);
	ESP_LOGI("MAIN", "Starting setup...");
	
	// Assign global and preset settings and boot the file system
	esp32Settings_AssignDefaultGlobalSettings(defaultGlobalSettingsAssignment);
	esp32Settings_AssignDefaultPresetSettings(defaultPresetsAssignment);
	esp32Settings_BootCheck(&globalSettings, sizeof(GlobalSettings), presets, sizeof(Preset), NUM_PRESETS, &globalSettings.bootState);

	// Click system tasks
	BaseType_t taskResult;
	taskResult = xTaskCreatePinnedToCore(
		indicatorTask, // Task function. 
		"Inicator Task", // name of task. 
		5000, // Stack size of task 
		NULL, // parameter of the task 
		INDICATOR_TASK_PRIORITY, // priority of the task 
		NULL, // Task handle to keep track of created task 
		1); // pin task to core 1 
	ESP_LOGI(MAIN_TAG, "Indicator task created: %d", taskResult);

	taskResult = xTaskCreatePinnedToCore(
		deviceApiTask, // Task function. 
		"Device API Task", // name of task. 
		70000, // Stack size of task 
		NULL, // parameter of the task 
		DEVICE_API_TASK_PRIORITY, // priority of the task 
		NULL, // Task handle to keep track of created task 
		1); // pin task to core 1 
	ESP_LOGI(MAIN_TAG, "Device API task created: %d", taskResult);

	midi_Init();
	display_Init();
}

void loop()
{
	midi_ReadAll();
	
	//delay(10);		
}


void defaultGlobalSettingsAssignment()
{
	// System settings
	globalSettings.bootState = 0; 						// Initial boot state
	globalSettings.currentPreset = 0; 					// Start with the first preset

	// UI settings
	globalSettings.uiLightMode = 0; 						// Auto dark mode
	globalSettings.mainColour = GEN_LOSS_BLUE;

	// MIDI settings
	globalSettings.midiChannel = MIDI_CHANNEL_OMNI; // Default MIDI channel
	globalSettings.clockMode = MIDI_CLOCK_EXTERNAL; 						// Use preset BPM by default
	globalSettings.globalBpm = 120.0; 					// Default global BPM
	globalSettings.midiOutMode = MIDI_OUT_TYPE_A; 	// Type A MIDI output by default
	// TRS MIDI thru flags
	globalSettings.midiTrsThruHandles[MIDI_TRS] = 1;
	globalSettings.midiTrsThruHandles[MIDI_BLE] = 1;
	// BLE MIDI thru flags
	globalSettings.midiBleThruHandles[MIDI_TRS] = 1;
	globalSettings.midiBleThruHandles[MIDI_BLE] = 1;

	globalSettings.wirelessType = WIRELESS_MODE_BLE;
}

void defaultPresetsAssignment()
{
	for (int i = 0; i < NUM_PRESETS; i++)
	{
		char str[64];
		sprintf(presets[i].name, "Preset %d", i + 1);
		//strcpy(presets[i].name, str);
		sprintf(presets[i].secondaryText, "Secondary %d", i + 1);
		//strcpy(presets[i].secondaryText, str);
		presets[i].colourOverrideFlag = 0; // Use main colour by default
		presets[i].colourOverride = 0; // Default colour
		presets[i].textColourOverrideFlag = 0; // Use main colour by default
		presets[i].textColourOverride = 0; // Default colour
		presets[i].bpm = 40.0 + i; // Set default BPM
		ESP_LOGI(MAIN_TAG, "Preset %d: %s", i, presets[i].name);
	}
}


void midi_ControlChangeHandler(byte channel, byte number, byte value)
{
	if(channel == globalSettings.midiChannel || globalSettings.midiChannel == MIDI_CHANNEL_OMNI)
	{
		switch(number)
		{
			case PRESET_UP_CC:
				presetUp();
				break;
			case PRESET_DOWN_CC:
				presetDown();
				break;
			case PRESET_SELECT_CC:
				goToPreset(value);
				break;
			default:
				break;
		}
	}
	midiReceived = 1;
}

void midi_ProgramChangeHandler(byte channel, byte number)
{
	if(channel	== globalSettings.midiChannel || globalSettings.midiChannel == MIDI_CHANNEL_OMNI)
	{
		goToPreset(number);	
	}
	midiReceived = 1;
}

void midi_SysExHandler(byte* data, unsigned length)
{

}

void presetUp()
{
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
	display_DrawPresetNumber(globalSettings.currentPreset);
	display_DrawMainText(presets[globalSettings.currentPreset].name, presets[globalSettings.currentPreset].secondaryText);
	midi_SetTempo();
}

void presetDown()
{
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
	display_DrawPresetNumber(globalSettings.currentPreset);
	display_DrawMainText(presets[globalSettings.currentPreset].name, presets[globalSettings.currentPreset].secondaryText);
	midi_SetTempo();
}

void goToPreset(uint16_t presetIndex)
{
	if(presetIndex < NUM_PRESETS)
	{
		globalSettings.currentPreset = presetIndex;
	}
	display_DrawPresetNumber(globalSettings.currentPreset);
	display_DrawMainText(presets[globalSettings.currentPreset].name, presets[globalSettings.currentPreset].secondaryText);
	midi_SetTempo();
}

void enterBootloader()
{
	//wifi_Disconnect();
	REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
	esp_restart();
}

void factoryReset()
{
	esp32Settings_ResetAllSettings();
}

void deviceApiTask(void* parameter)
{
	ESP_LOGI(MAIN_TAG, "Device API task started");
	while(1)
	{		
		if(Serial.available())
		{
			deviceApi_Handler(deviceApiBuffer, 0);
		}
		vTaskDelay(2 / portTICK_PERIOD_MS);
	}
}

void indicatorTask(void* parameter)
{
	while(1)
	{
		// New MIDI input
		if(midiReceived)
		{
			display_DrawMidiIndicator(true);
			vTaskDelay(MIDI_INDICATOR_ON_TIME / portTICK_PERIOD_MS);
			display_DrawMidiIndicator(false);
			midiReceived = 0;
		}
		// New BLE event
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
		// New MIDI clock event
		if(newClockEvent)
		{
			if(globalSettings.clockMode == MIDI_CLOCK_EXTERNAL)
			{
				switch(newClockEvent)
				{
					case MIDI_CLOCK_EVENT_CHANGE:
						// The tempo has changed; update the display with new bpm
						display_DrawBpm(currentBpm);
					break;

					case MIDI_CLOCK_EVENT_START:
						// Set the BPM colour to green
						display_SetBpmDrawColour(CLOCK_START_COLOUR); 
						display_DrawBpm(currentBpm);
					break;

					case MIDI_CLOCK_EVENT_STOP:
						// Set the BPM colour to green
						display_SetBpmDrawColour(CLOCK_STOP_COLOUR); 
						display_DrawBpm(currentBpm);
					break;

					newClockEvent = MIDI_CLOCK_EVENT_CLEAR;
				}
			}
			else if(	globalSettings.clockMode == MIDI_CLOCK_PRESET ||
						globalSettings.clockMode == MIDI_CLOCK_GLOBAL)
			{
				switch(newClockEvent)
				{
					case MIDI_CLOCK_EVENT_CHANGE:
						// The tempo has changed; update the display with new bpm
						display_SetBpmDrawColour(CLOCK_START_COLOUR); 
						display_DrawBpm(currentBpm);
					break;
				}
				// TODO: is a clock tempo indicator needed considering most pedals have them already?
				// Might not be worth adding to avoid confusion
			}
		}
				
		vTaskDelay(20 / portTICK_PERIOD_MS);
	}
}