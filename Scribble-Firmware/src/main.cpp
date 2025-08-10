#include <Arduino.h>
#include "hardware_Def.h"
#include "esp32_Settings.h"
#include "main.h"
#include "display.h"
#include "midi_Handling.h"

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
	esp32Settings_BootCheck(&globalSettings, presets, NUM_PRESETS, &globalSettings.bootState);
	
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