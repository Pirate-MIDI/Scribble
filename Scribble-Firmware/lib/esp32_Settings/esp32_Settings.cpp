#include "esp32_Settings.h"
#include <LittleFS.h>

#define FORMAT_LITTLEFS_IF_FAILED true
#define DEVICE_CONFIGURED_VALUE 114 // Arbitrary value to indicate the device has been configured

char* SETTINGS_TAG = "ESP32_SETTINGS";

void* globalSettingsPtr = NULL;
void* presetsPtr = NULL;
size_t numPresets = 0;

// Checks for the standard combination of a global settings file and a presets file
// If the files are not present or the sizes do not match, it will format the file
// This function also initialises the global and preset settings pointers as well as the number of presets
// The bootflag is a pointer to the global settings boot state flag
void esp32Settings_BootCheck(	void* GlobalSettingsPtr, void* PresetsPtr,
										size_t numPresets, uint8_t* bootFlagPtr)
{
	// Perform the boot check
	ESP_LOGI(SETTINGS_TAG, "Boot check initiated.");
	if (globalSettingsPtr == NULL || presetsPtr == NULL || numPresets == 0)
	{
		ESP_LOGE(SETTINGS_TAG, "Invalid settings or presets pointers.");
		return;
	}

	// Assign the global settings and presets pointers
	globalSettingsPtr = GlobalSettingsPtr;
	presetsPtr = PresetsPtr;
	numPresets = numPresets;
	
	// Check if an appropriate file system is available
	ESP_LOGI(SETTINGS_TAG, "Checking boot state...");
	if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
	{
		ESP_LOGI(SETTINGS_TAG, "LittleFS Mount Failed. Formatting...");
		newDeviceConfig();
	}

	// Check for the correct file structures
	ESP_LOGI(SETTINGS_TAG, "Checking file system...");
	uint8_t structureOk = 1;
	if (!LittleFS.exists("/global.txt"))
		structureOk = 0;

	if (!LittleFS.exists("/presets.txt"))
		structureOk = 0;

	if (!structureOk)
	{
		ESP_LOGI(SETTINGS_TAG, "File system structure incorrect.");
		newDeviceConfig();
	}

	// Check the global settings size from the file system
	ESP_LOGI(SETTINGS_TAG, "Validating global config size...");
	File globalConfigFile = LittleFS.open("/global.txt", "r");
	size_t globalConfigFileSize = globalConfigFile.size();
	ESP_LOGI(SETTINGS_TAG, "Global config file size: %d", globalConfigFileSize);
	if (globalConfigFileSize != sizeof(GlobalSettingsPtr))
	{
		ESP_LOGI(SETTINGS_TAG, "Global settings file size does not match.");
		newDeviceConfig();
	}
	globalConfigFile.close();
	
	// Read the global settings
	//readGlobalSettings();

	// Check the preset file size from the file system
	File presetsFile = LittleFS.open("/presets.txt", "r");
	size_t presetsFileSize = presetsFile.size();
	presetsFile.close();

	if (presetsFileSize != sizeof(PresetsPtr)*numPresets)
	{
		ESP_LOGI(SETTINGS_TAG, "Presets file size does not match.");
		//newDeviceConfig();
	}

	// Read the preset data
	//readPresets();

	// Uncomment to force a new device configuration
	// globalSettings.bootState = 0;
	if (*bootFlagPtr != DEVICE_CONFIGURED_VALUE)
	{
		ESP_LOGI(SETTINGS_TAG, "Configuring new device...");
		//newDeviceConfig();
	}
	else
	{
		ESP_LOGI(SETTINGS_TAG, "Performing standard boot...");
		//standardBoot();
	}
}

// Configures the device to a factory state
void esp32Settings_NewDeviceConfig()
{
	leds.setPixelColor(0, LED_BOOT_COLOUR);
	leds.setPixelColor(1, LED_BOOT_COLOUR);
	leds.show();

	// Configure default values for global settings
	globalSettings.bootState = DEVICE_CONFIGURED_VALUE;
	strcpy(globalSettings.firmwareVersion, FW_VERSION);
	globalSettings.currentPreset = 0;
	globalSettings.profileId = 0;
	const char* newDeviceName = "CLiCK";
	strcpy(globalSettings.deviceName, newDeviceName);

	globalSettings.mainMidiChannel = MIDI_CHANNEL_OMNI;
	globalSettings.tonexOneMidiChannel = TONEX_ONE_DEFAULT_MIDI_CHANNEL;
	globalSettings.midiOutMode = MidiOutA;
	globalSettings.expOutMin = 0;
	globalSettings.expOutMax = 255;
	globalSettings.ledBrightness = 255;
	globalSettings.ledFunctions[0] = LedMidiInput;
	globalSettings.ledFunctions[1] = LedWireless;

	globalSettings.esp32ManagerConfig.wirelessType = Esp32BLE;
	globalSettings.esp32ManagerConfig.wifiMode = Esp32WiFiDevice;
	globalSettings.esp32ManagerConfig.bleMode = Esp32BLEServer;

	// Format the file system to revert to a default state
	ESP_LOGI(SETTINGS_TAG, "Formatting file system...");
	LittleFS.format();

	// Create the storage file for the global config
	ESP_LOGI(SETTINGS_TAG, "Creating global settings file...");
	File globalConfigFile = LittleFS.open("/global.txt", "w");
	globalConfigFile.write((uint8_t *)&globalSettings, sizeof(GlobalSettings));
	globalConfigFile.close();

	ESP_LOGI(SETTINGS_TAG, "%d", globalSettings.bootState);
	// Configure default preset values
	for (uint8_t i = 0; i < NUM_PRESETS; i++)
	{
		presets[i].tipState = 0;
		presets[i].ringState = 0;
		presets[i].expValue = 0;
		sprintf(presets[i].presetName, "New Preset %d", i);
	}

	// Create the presets storage file
	ESP_LOGI(SETTINGS_TAG, "Creating presets file...");
	File presetsFile = LittleFS.open("/presets.txt", "w");
	presetsFile.write((uint8_t *)presets, sizeof(Preset)*NUM_PRESETS);
	presetsFile.close();
	
	File testFile = LittleFS.open("/global.txt", "r");
	testFile.read((uint8_t *)&globalSettings, sizeof(GlobalSettings));
	testFile.close();
	ESP_LOGI(SETTINGS_TAG, "Device configured. Rebooting.");

	// Flash the control LED three times to indicate a successful first use boot
	for (uint8_t i = 0; i < 3; i++)
	{
		leds.setPixelColor(CTRL_LED, 0xffffff);
		leds.show();
		delay(500);
		leds.setPixelColor(CTRL_LED, 0);
		leds.show();
		delay(500);
	}
	leds.setPixelColor(0, 0x000000);
	leds.setPixelColor(1, 0x000000);
	leds.show();
	delay(10);
	softwareReset();
}

void esp32Settings_StandardBoot()
{
	// Read the preset data into the struct array
	ESP_LOGI(SETTINGS_TAG, "Reading global.txt");
	File globalConfigFile = LittleFS.open("/global.txt", "r");
	globalConfigFile.read((uint8_t *)&globalSettings, sizeof(GlobalSettings));
	globalConfigFile.close();

	// Read the preset data into the struct array
	ESP_LOGI(SETTINGS_TAG, "Reading presets...");

	File presetsFile = LittleFS.open("/presets.txt", "r");
	presetsFile.read((uint8_t *)&presets, sizeof(Preset)*NUM_PRESETS);
	presetsFile.close();

	// Utility setup functions
	generateSineTable();

	ESP_LOGI(SETTINGS_TAG, "Standard boot complete!");
	listDir(LittleFS, "/", 1);
}

void esp32Settings_SoftwareReset()
{
	ESP.restart();
}