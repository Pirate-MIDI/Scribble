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

uint8_t wifiState = WIFI_STATE_NOT_CONNECTED;
int8_t wifiRssi;
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
	ESP_LOGI("MAIN", "Starting setup...");
	
	// Assign global and preset settings and boot the file system
	esp32Settings_AssignDefaultGlobalSettings(defaultGlobalSettingsAssignment);
	esp32Settings_AssignDefaultPresetSettings(defaultPresetsAssignment);
	esp32Settings_BootCheck(&globalSettings, sizeof(GlobalSettings), presets, sizeof(Preset), NUM_PRESETS, &globalSettings.bootState);

	// Click system tasks
	BaseType_t taskResult;
	taskResult = xTaskCreatePinnedToCore(
		indicatorTask, // Task function. 
		"Inidactor Task", // name of task. 
		5000, // Stack size of task 
		NULL, // parameter of the task 
		INDICATOR_TASK_PRIORITY, // priority of the task 
		NULL, // Task handle to keep track of created task 
		1); // pin task to core 1 
	ESP_LOGI(MAIN_TAG, "LED task created: %d", taskResult);

	taskResult = xTaskCreatePinnedToCore(
		deviceApiTask, // Task function. 
		"Device API Task", // name of task. 
		20000, // Stack size of task 
		NULL, // parameter of the task 
		DEVICE_API_TASK_PRIORITY, // priority of the task 
		NULL, // Task handle to keep track of created task 
		1); // pin task to core 1 
	ESP_LOGI(MAIN_TAG, "LED task created: %d", taskResult);

	midi_Init();
	display_Init();

	if(globalSettings.wirelessType == WIRELESS_MODE_WIFI)
	{
		// Start WiFi connection
		//wifi_Connect(WIFI_HOSTNAME, WIFI_AP_SSID, NULL);
		if(wifiState == WIFI_STATE_CONNECTED)
		{
			// Start the WiFi RTP MIDI
			//midi_InitWiFiRTP();
		}
	}
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
	globalSettings.useLargePresetFont = 0;				// Use small text by default

	// MIDI settings
	globalSettings.midiChannel = MIDI_CHANNEL_OMNI; // Default MIDI channel
	globalSettings.clockMode = 0; 						// Use preset BPM by default
	globalSettings.globalBpm = 120.0; 					// Default global BPM
	globalSettings.midiOutMode = MIDI_OUT_TYPE_A; 	// Type A MIDI output by default
	// TRS MIDI thru flags
	globalSettings.midiTrsThruHandles[MIDI_TRS] = 1;
	globalSettings.midiTrsThruHandles[MIDI_BLE] = 1;
	globalSettings.midiTrsThruHandles[MIDI_WIFI] = 1;
	// BLE MIDI thru flags
	globalSettings.midiBleThruHandles[MIDI_TRS] = 1;
	globalSettings.midiBleThruHandles[MIDI_BLE] = 1;
	globalSettings.midiBleThruHandles[MIDI_WIFI] = 1;
	// WiFi MIDI thru flags
	globalSettings.midiWifiThruHandles[MIDI_TRS] = 1;
	globalSettings.midiWifiThruHandles[MIDI_BLE] = 1;
	globalSettings.midiWifiThruHandles[MIDI_WIFI] = 1;
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

void presetUp()
{

}

void presetDown()
{

}

void goToPreset(uint16_t presetIndex)
{

}

void enterBootloader()
{
	//wifi_Disconnect();
	REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
	esp_restart();
}

void factoryReset()
{

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
	}
}

void indicatorTask(void* parameter)
{
	static uint16_t ledBreathingIndex = 0;
	static uint16_t ledBlinkingIndex = 0;
	while(1)
	{
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