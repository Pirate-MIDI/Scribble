#include <Arduino.h>
#include "hardware_Def.h"
#include "esp32_Settings.h"
#include "main.h"
#include "display.h"

float currentBpm = 120.0;
GlobalSettings globalSettings;
Presets presets[NUM_PRESETS];

void setup()
{
	Serial0.begin(115200);
	Serial.begin(115200);
	ESP_LOGI("MAIN", "Starting setup...");
	
	// Assign global and preset settings and boot the file system
	esp32Settings_AssignDefaultGlobalSettings(defaultGlobalSettingsAssignment);
	esp32Settings_AssignDefaultPresetSettings(defaultPresetsAssignment);
	esp32Settings_BootCheck(&globalSettings, presets, NUM_PRESETS, &globalSettings.bootState);

	display_Init();
}

void loop()
{

}


void defaultGlobalSettingsAssignment()
{

}

void defaultPresetsAssignment()
{

}