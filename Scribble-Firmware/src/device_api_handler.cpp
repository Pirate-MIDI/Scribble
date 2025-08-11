#include "device_api_handler.h"
#include "device_api_utility.h"
#include "device_api.h"
#include "ArduinoJson.h"
#include "math.h"
#include "esp32-hal-tinyusb.h" // required for entering download mode
#include "midi_handling.h"
#include "main.h"
#include "esp_log.h"
#include "esp32_settings.h"


// Transmit functions
void sendCheckResponse(uint8_t transport)
{
	// Allocate the JSON document
	JsonDocument doc;

	// Device information
	doc["deviceModel"] = "Scribble";
	doc["firmwareVersion"] = FW_VERSION;
	doc["hardwareVersion"] = HW_VERSION;
	doc["uId"] = ((ESP.getEfuseMac() << 40) >> 40);
	doc["deviceName"] = "Scribble";
	doc["profileId"] = 0;

	// ESP32 Manager info


	if(transport == USB_CDC_TRANSPORT)
	{
		serializeJson(doc, Serial);
		sendPacketTermination(USB_CDC_TRANSPORT);
	}
	else if(transport == MIDI_TRANSPORT)
	{
		CustomWriter writer;
		writer.transport = MIDI_TRANSPORT;
		serializeJson(doc, writer);
		writer.flush();
		sendPacketTermination(MIDI_TRANSPORT);
	}
	return;
}

void sendGlobalSettings(uint8_t transport)
{
	JsonDocument doc;
	doc["deviceName"] = globalSettings.deviceName;
	doc[USB_CURRENT_BANK_STRING] = globalSettings.currentPreset;

	if(globalSettings.uiLightMode == MIDI_CLOCK_PRESET)
		doc["lightMode"] = "dark";
	else if(globalSettings.uiLightMode == MIDI_CLOCK_EXTERNAL)
		doc["lightMode"] = "light";
	else
		doc["lightMode"] = "auto";

	doc["mainColour"] = globalSettings.mainColour;
	doc["useLargePresetFont"] = (bool)globalSettings.useLargePresetFont;

	// MIDI TRS thru handles
	doc[USB_MIDI1_THRU_HANDLES_STRING][USB_MIDI1_STRING] = (bool)globalSettings.midiTrsThruHandles[MIDI_TRS];
	doc[USB_MIDI1_THRU_HANDLES_STRING][USB_BLE_STRING] = (bool)globalSettings.midiTrsThruHandles[MIDI_BLE];
	doc[USB_MIDI1_THRU_HANDLES_STRING][USB_WIFI_STRING] = (bool)globalSettings.midiTrsThruHandles[MIDI_WIFI];

	// BLE thru handles
	doc[USB_BLE_THRU_HANDLES_STRING][USB_MIDI1_STRING] = (bool)globalSettings.midiBleThruHandles[MIDI_TRS];
	doc[USB_BLE_THRU_HANDLES_STRING][USB_BLE_STRING] = (bool)globalSettings.midiBleThruHandles[MIDI_BLE];
	doc[USB_BLE_THRU_HANDLES_STRING][USB_WIFI_STRING] = (bool)globalSettings.midiBleThruHandles[MIDI_WIFI];

	// WiFi thru handles
	doc[USB_WIFI_THRU_HANDLES_STRING][USB_MIDI1_STRING] = (bool)globalSettings.midiWifiThruHandles[MIDI_TRS];
	doc[USB_WIFI_THRU_HANDLES_STRING][USB_BLE_STRING] = (bool)globalSettings.midiWifiThruHandles[MIDI_BLE];
	doc[USB_WIFI_THRU_HANDLES_STRING][USB_WIFI_STRING] = (bool)globalSettings.midiWifiThruHandles[MIDI_WIFI];

	// MIDI channels
	doc[USB_MIDI_CHANNEL_STRING] = globalSettings.midiChannel;
	doc["globalBpm"] = globalSettings.globalBpm;

	if(globalSettings.clockMode == MIDI_CLOCK_PRESET)
		doc["clockMode"] = "preset";
	else if(globalSettings.clockMode == MIDI_CLOCK_EXTERNAL)
		doc["clockMode"] = "external";
	else if(globalSettings.clockMode == MIDI_CLOCK_GLOBAL)
		doc["clockMode"] = "global";
	else
		doc["clockMode"] = "none";

	// MIDI out port mode
	if(globalSettings.midiOutMode == MIDI_OUT_TYPE_A)
		doc["midiOutPortMode"] = "midiOutA";
	else if(globalSettings.midiOutMode == MIDI_OUT_TYPE_B)
		doc["midiOutPortMode"] = "midiOutB";

	// ESP32 Manager config
	if(globalSettings.wirelessType == WIRELESS_MODE_WIFI)
		doc["wirelessType"] = "wifi";
	else if(globalSettings.wirelessType == WIRELESS_MODE_BLE)
		doc["wirelessType"] = "ble";
	else
		doc["wirelessType"] = "none";

	
	if(transport == USB_CDC_TRANSPORT)
	{
		serializeJson(doc, Serial);
		sendPacketTermination(USB_CDC_TRANSPORT);
	}
	else if(transport == MIDI_TRANSPORT)
	{
		CustomWriter writer;
		writer.transport = MIDI_TRANSPORT;
		serializeJson(doc, writer);
		writer.flush();
		sendPacketTermination(MIDI_TRANSPORT);
	}
}

void sendBankSettings(int bankNum, uint8_t transport)
{
	JsonDocument doc;
	doc["colourOverride"] = (bool)presets[bankNum].colourOverrideFlag;
	doc["colour"] = presets[bankNum].colourOverride;
	doc["name"] = presets[bankNum].name;
	doc["secondaryText"] = presets[bankNum].secondaryText;
	doc["bpm"] = presets[bankNum].bpm;
	

	if(transport == USB_CDC_TRANSPORT)
	{
		serializeJson(doc, Serial);
		sendPacketTermination(USB_CDC_TRANSPORT);
	}
	else if(transport == MIDI_TRANSPORT)
	{
		CustomWriter writer;
		writer.transport = MIDI_TRANSPORT;
		serializeJson(doc, writer);
		writer.flush();
		sendPacketTermination(MIDI_TRANSPORT);
	}
	return;
}

void sendBankId(int bankNum, uint8_t transport)
{
	
}

void sendCurrentBank(uint8_t transport)
{
	JsonDocument doc;
	doc[USB_CURRENT_BANK_STRING] = globalSettings.currentPreset;
	if(transport == USB_CDC_TRANSPORT)
	{
		serializeJson(doc, Serial);
		sendPacketTermination(USB_CDC_TRANSPORT);
	}
	else if(transport == MIDI_TRANSPORT)
	{
		CustomWriter writer;
		writer.transport = MIDI_TRANSPORT;
		serializeJson(doc, writer);
		writer.flush();
		sendPacketTermination(MIDI_TRANSPORT);
	}
}


// Parsing functions
void parseGlobalSettings(char* appData, uint8_t transport)
{
	// Allocate the JSON document
	JsonDocument doc;

	// Deserialize the JSON document
	DeserializationError error = deserializeJson(doc, appData);
	// Test if parsing succeeds
	if(error)
	{
		Serial.printf("deserializeJson() failed: %s\n", error.c_str());
		return;
	}

	const char* newDeviceName = doc[USB_DEVICE_NAME_STRING];
	strcpy(globalSettings.deviceName, newDeviceName);
	globalSettings.currentPreset = doc[USB_CURRENT_BANK_STRING];
	globalSettings.profileId = doc[USB_PROFILE_ID_STRING];
	
	if(strcmp(doc["lightMode"], "light") == 0)
		globalSettings.uiLightMode = UI_MODE_LIGHT;
	else if(strcmp(doc["lightMode"], "dark") == 0)
		globalSettings.uiLightMode = UI_MODE_DARK;
	else
		globalSettings.uiLightMode = UI_MODE_AUTO;

	globalSettings.pedalModel = doc["pedalModel"];
	globalSettings.mainColour = doc["mainColour"];
	globalSettings.useLargePresetFont = (bool)doc["useLargePresetFont"];;

	// MIDI 1 thru handles
	globalSettings.midiTrsThruHandles[MIDI_TRS] = (uint8_t)doc[USB_MIDI1_THRU_HANDLES_STRING][USB_MIDI1_STRING];
	globalSettings.midiTrsThruHandles[MIDI_BLE] = (uint8_t)doc[USB_MIDI1_THRU_HANDLES_STRING][USB_BLE_STRING];
	globalSettings.midiTrsThruHandles[MIDI_WIFI] = (uint8_t)doc[USB_MIDI1_THRU_HANDLES_STRING][USB_WIFI_STRING];

	// BLE thru handles
	globalSettings.midiBleThruHandles[MIDI_TRS] = (uint8_t)doc[USB_BLE_THRU_HANDLES_STRING][USB_MIDI1_STRING];
	globalSettings.midiBleThruHandles[MIDI_BLE] = (uint8_t)doc[USB_BLE_THRU_HANDLES_STRING][USB_BLE_STRING];
	globalSettings.midiBleThruHandles[MIDI_WIFI] = (uint8_t)doc[USB_BLE_THRU_HANDLES_STRING][USB_WIFI_STRING];

	// WiFi thru handles
	globalSettings.midiWifiThruHandles[MIDI_TRS] = (uint8_t)doc[USB_WIFI_THRU_HANDLES_STRING][USB_MIDI1_STRING];
	globalSettings.midiWifiThruHandles[MIDI_BLE] = (uint8_t)doc[USB_WIFI_THRU_HANDLES_STRING][USB_BLE_STRING];
	globalSettings.midiWifiThruHandles[MIDI_WIFI] = (uint8_t)doc[USB_WIFI_THRU_HANDLES_STRING][USB_WIFI_STRING];


	// MIDI channels
	globalSettings.midiChannel = doc[USB_MIDI_CHANNEL_STRING];
	
	// MIDI out port mode
	if(strcmp(doc["midiOutPortMode"], "midiOutA") == 0)
		globalSettings.midiOutMode = MIDI_OUT_TYPE_A;
	else if(strcmp(doc["midiOutPortMode"], "midiOutB") == 0)
		globalSettings.midiOutMode = MIDI_OUT_TYPE_B;


	// ESP32 Manager config
	if(strcmp(doc["wirelessType"], "wifi") == 0)
		globalSettings.wirelessType = WIRELESS_MODE_WIFI;
	else if(strcmp(doc["wirelessType"], "ble") == 0)
	{
		// If the device is switching from a WiFi configuration, disable the WiFi and reset the settings
		if(globalSettings.wirelessType == WIRELESS_MODE_WIFI)
		{
			//wifi_Disconnect();
		}
		globalSettings.wirelessType = WIRELESS_MODE_BLE;
	}
	else
		globalSettings.wirelessType = WIRELESS_MODE_NONE;
}

void parseBankSettings(char* appData, uint16_t bankNum, uint8_t transport)
{
	// Allocate the JSON document
	JsonDocument doc;

	// Deserialize the JSON document
	DeserializationError error = deserializeJson(doc, appData);
	// Test if parsing succeeds
	if(error)
	{
		Serial.printf("deserializeJson() failed: %s\n", error.c_str());
		return;
	}

	const char* newPresetName = doc["name"];
	strcpy(presets[bankNum].name, newPresetName);
	const char* newSecondaryText = doc["secondaryText"];
	strcpy(presets[bankNum].secondaryText, newSecondaryText);
	presets[bankNum].colourOverrideFlag = (bool)doc["colourOverride"];
	presets[bankNum].colourOverride = doc["colour"];
	presets[bankNum].bpm = doc["bpm"];
}

void ctrlCommandHandler(char* appData, uint8_t transport)
{
	// Allocate the JSON document
	JsonDocument doc;

	// Deserialize the JSON document
	DeserializationError error = deserializeJson(doc, appData);
	// Test if parsing succeeds
	if (error)
	{
		Serial.write("deserializeJson() failed: ", strlen("deserializeJson() failed: "));
		Serial.write(error.c_str(), strlen(error.c_str()));
		Serial.write("\n", strlen("\n"));
		return;
	}

	if(doc[USB_COMMAND_STRING])
	{
		// If there is an array of commands
		uint8_t numCommands = doc.size();
		for(uint16_t i=0; i<numCommands; i++)
		{
			// Strings
			if(doc[USB_COMMAND_STRING][i].is<const char*>())
			{
				const char* command = doc[USB_COMMAND_STRING][i];
				// Prioritise the restart command
				if(strcmp(command, USB_RESTART_STRING) == 0)
				{
					ESP.restart();
				}
				else if(strcmp(command, USB_ENTER_BOOTLOADER_STRING) == 0)
				{
					enterBootloader();
				}
				else if(strcmp(command, USB_BANK_UP_STRING) == 0)
				{
					presetUp();
				}
				else if(strcmp(command, USB_BANK_DOWN_STRING) == 0)
				{
					presetDown();
				}
				else if(strcmp(command, "savePresets") == 0)
				{
					esp32Setting_SavePresets();
					//savePresets();
				}
#ifdef USE_BLE_MIDI				
				else if(strcmp(command, "turnOffBLE") == 0)
				{
					turnOffBLE();
				}
				else if(strcmp(command, "turnOnBLE") == 0)
				{
					turnOnBLE();
				}
#endif		
				
				else if(strcmp(command, USB_FACTORY_RESET_STRING) == 0)
				{
					factoryReset();
				}
				else
				{
					ESP_LOGD("Device API", "Unknown CTRL command: %s", command);
				}
			}
			else
			{
				if(!doc[USB_COMMAND_STRING][i][USB_GO_TO_BANK_STRING].isNull())
				{
					uint16_t bankIndex = doc[USB_COMMAND_STRING][i][USB_GO_TO_BANK_STRING];
					if(bankIndex < 128)
					{
						goToPreset(bankIndex);
					}
				}
			}
		}
	}
}

