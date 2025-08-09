#include <Arduino.h>
#include "hardware_Def.h"
#include "esp32_Settings.h"
#include "main.h"
#include "display.h"

float currentBpm = 120.0;
GlobalSettings globalSettings;
Presets presets[NUM_PRESETS];

void defaultGlobalSettingsAssignment();
void defaultPresetsAssignment();

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
	globalSettings.bootState = 0; // Initial boot state
	globalSettings.currentPreset = 0; // Start with the first preset
	globalSettings.midiChannel = 0; // Default MIDI channel
	globalSettings.uiLightMode = 0; // Auto light mode
	globalSettings.mainColour = GEN_LOSS_BLUE; // Default main colour
	globalSettings.clockMode = 0; // Use preset BPM by default
	globalSettings.globalBpm = 120.0; // Default global BPM
	globalSettings.midiOutMode = 0; // Type A MIDI output by default
}

void defaultPresetsAssignment()
{
	for (int i = 0; i < NUM_PRESETS; i++) {
		snprintf(presets[i].name, sizeof(presets[i].name), "Preset %d", i + 1);
		snprintf(presets[i].secondaryText, sizeof(presets[i].secondaryText), "Secondary %d", i + 1);
		presets[i].colourOverrideFlag = 0; // Use main colour by default
		presets[i].colourOverride = GEN_LOSS_BLUE; // Default colour
		presets[i].bpm = 120.0; // Set default BPM
	}
}