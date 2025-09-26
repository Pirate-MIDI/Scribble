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
#include "ota_updating.h"
#include "wifi_management.h"

static const char* DEVICE_API_TAG = "Device API";

const char *midiInterfaceStrings[NUM_MIDI_INTERFACES] = {	USB_USB_STRING,
																				USB_BLE_STRING,
																				USB_MIDI1_STRING};

void packMessageStack(const JsonArray& jsonArray, MidiMessage* messages, uint16_t numMessages);
void parseMessageStack(const JsonArray& jsonArray, MidiMessage* messages, uint16_t numMessages);
uint16_t rgb888_to_rgb565(uint32_t rgb888);

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
	doc["profileId"] = globalSettings.profileId;


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

	if(globalSettings.uiLightMode == UI_MODE_DARK)
		doc["lightMode"] = "dark";
	else if(globalSettings.uiLightMode == UI_MODE_LIGHT)
		doc["lightMode"] = "light";

	doc["mainColour"] = globalSettings.mainColour;
	doc["textColour"] = globalSettings.textColour;
	//doc["displayBrightness"] = globalSettings.displayBrightness/2;
	doc["displayBrightness"] = devApi_roundMap(globalSettings.displayBrightness, 1, 255, 1, 100);

	// MIDI channels
	doc[USB_MIDI_CHANNEL_STRING] = globalSettings.midiChannel;
	doc["globalBpm"] = globalSettings.globalBpm;

	// MIDI out port mode
	if(globalSettings.midiOutMode == MIDI_OUT_TYPE_A)
		doc["midiOutPortMode"] = "midiOutA";
	else if(globalSettings.midiOutMode == MIDI_OUT_TYPE_B)
		doc["midiOutPortMode"] = "midiOutB";

	// MIDI clock mode	
	if(globalSettings.clockMode == MIDI_CLOCK_PRESET)
		doc["clockMode"] = "preset";
	else if(globalSettings.clockMode == MIDI_CLOCK_EXTERNAL)
		doc["clockMode"] = "external";
	else if(globalSettings.clockMode == MIDI_CLOCK_GLOBAL)
		doc["clockMode"] = "global";
	else
		doc["clockMode"] = "none";	

	// MIDI clock display type
	if(globalSettings.clockDisplayType == MIDI_CLOCK_DISPLAY_BPM)
		doc["clockDisplayType"] = "bpm";
	else if(globalSettings.clockDisplayType == MIDI_CLOCK_DISPLAY_MS)
		doc["clockDisplayType"] = "ms";
	else
		doc["clockDisplayType"] = "indicator";

	// MIDI USBD thru handles
	doc[USB_USBD_THRU_HANDLES_STRING][USB_USBD_STRING] = (bool)globalSettings.usbdThruHandles[MidiUSBD];
	doc[USB_USBD_THRU_HANDLES_STRING][USB_BLE_STRING] = (bool)globalSettings.usbdThruHandles[MidiBLE];
	//doc[USB_USBD_THRU_HANDLES_STRING][USB_WIFI_STRING] = (bool)globalSettings.usbdThruHandles[MidiWiFiRTP];
	doc[USB_USBD_THRU_HANDLES_STRING][USB_MIDI1_STRING] = (bool)globalSettings.usbdThruHandles[MidiSerial1];

	// BLE thru handles
	doc[USB_BLE_THRU_HANDLES_STRING][USB_USBD_STRING] = (bool)globalSettings.bleThruHandles[MidiUSBD];
	doc[USB_BLE_THRU_HANDLES_STRING][USB_BLE_STRING] = (bool)globalSettings.bleThruHandles[MidiBLE];
	//doc[USB_BLE_THRU_HANDLES_STRING][USB_WIFI_STRING] = (bool)globalSettings.bleThruHandles[MidiWiFiRTP];
	doc[USB_BLE_THRU_HANDLES_STRING][USB_MIDI1_STRING] = (bool)globalSettings.bleThruHandles[MidiSerial1];

	// MIDI WiFi thru handles
	//doc[USB_WIFI_THRU_HANDLES_STRING][USB_USBD_STRING] = (bool)globalSettings.wifiThruHandles[MidiUSBD];
	//doc[USB_WIFI_THRU_HANDLES_STRING][USB_BLE_STRING] = (bool)globalSettings.wifiThruHandles[MidiBLE];
	//doc[USB_WIFI_THRU_HANDLES_STRING][USB_WIFI_STRING] = (bool)globalSettings.wifiThruHandles[MidiWiFiRTP];
	//doc[USB_WIFI_THRU_HANDLES_STRING][USB_MIDI1_STRING] = (bool)globalSettings.wifiThruHandles[MidiSerial1];

	// MIDI TRS thru handles
	doc[USB_MIDI1_THRU_HANDLES_STRING][USB_USBD_STRING] = (bool)globalSettings.midi1ThruHandles[MidiUSBD];
	doc[USB_MIDI1_THRU_HANDLES_STRING][USB_BLE_STRING] = (bool)globalSettings.midi1ThruHandles[MidiBLE];
	//doc[USB_MIDI1_THRU_HANDLES_STRING][USB_WIFI_STRING] = (bool)globalSettings.midi1ThruHandles[MidiWiFiRTP];
	doc[USB_MIDI1_THRU_HANDLES_STRING][USB_MIDI1_STRING] = (bool)globalSettings.midi1ThruHandles[MidiSerial1];

	// MIDI clock output handles
	doc[USB_MIDI_CLOCK_OUT_HANDLES_STRING][USB_USBD_STRING] = (bool)globalSettings.midiClockOutHandles[MidiUSBD];
	doc[USB_MIDI_CLOCK_OUT_HANDLES_STRING][USB_BLE_STRING] = (bool)globalSettings.midiClockOutHandles[MidiBLE];
	//doc[USB_MIDI_CLOCK_OUT_HANDLES_STRING][USB_WIFI_STRING] = (bool)globalSettings.midiClockOutHandles[MidiWiFiRTP];
	doc[USB_MIDI_CLOCK_OUT_HANDLES_STRING][USB_MIDI1_STRING] = (bool)globalSettings.midiClockOutHandles[MidiSerial1];


	// Message stacks
	for(uint8_t i=0; i<2; i++)
	{
		if(globalSettings.switchMode[i] == SwitchPressPresetUp)
			doc["switches"][i]["mode"] = "pressPresetUp";
		else if(globalSettings.switchMode[i] == SwitchPressPresetDown)
			doc["switches"][i]["mode"] = "pressPresetDown";
		else if(globalSettings.switchMode[i] == SwitchHoldPresetUp)
			doc["switches"][i]["mode"] = "holdPresetUp";
		else if(globalSettings.switchMode[i] == SwitchHoldPresetDown)
			doc["switches"][i]["mode"] = "holdPresetDown";
		else
			doc["switches"][i]["mode"] = "messagesOnly";
			
		// Press messages
		doc["switches"][i]["pressMessages"]["numMessages"] = globalSettings.numSwitchPressMessages[i];
		packMessageStack(	doc["switches"][i]["pressMessages"]["messages"].to<JsonArray>(),
									globalSettings.switchPressMessages[i], globalSettings.numSwitchPressMessages[i]);
		// Hold messages
		doc["switches"][i]["holdMessages"]["numMessages"] = globalSettings.numSwitchHoldMessages[i];
		packMessageStack(	doc["switches"][i]["holdMessages"]["messages"].to<JsonArray>(),
									globalSettings.switchHoldMessages[i], globalSettings.numSwitchHoldMessages[i]);
	}	
	// Custom messages
	doc["customMessages"]["numMessages"] = globalSettings.numCustomMessages;
	packMessageStack(	doc["customMessages"]["messages"].to<JsonArray>(),
										globalSettings.customMessages, globalSettings.numCustomMessages);

	doc["presetUpCC"] = globalSettings.presetUpCC;
	doc["presetDownCC"] = globalSettings.presetDownCC;
	doc["goToPresetCC"] = globalSettings.goToPresetCC;
	doc["globalCustomMessagesCC"] = globalSettings.globalCustomMessagesCC;
	doc["presetCustomMessagesCC"] = globalSettings.presetCustomMessagesCC;

	// Wireless config
	if(globalSettings.esp32ManagerConfig.wirelessType == Esp32BLE)
		doc["wirelessType"] = "ble";
	else if(globalSettings.esp32ManagerConfig.wirelessType == Esp32WiFi)
		doc["wirelessType"] = "wifi";
	else
		doc["wirelessType"] = "none";

	if(globalSettings.esp32ManagerConfig.bleMode == Esp32BLEServer)
		doc["bleMode"] = "server";
	else
		doc["bleMode"] = "client";

	// Static IP
	doc["useStaticIP"] = (bool)globalSettings.esp32ManagerConfig.useStaticIp;

	doc["staticIP"] = (String(globalSettings.esp32ManagerConfig.staticIp[0]) + "." +
							String(globalSettings.esp32ManagerConfig.staticIp[1]) + "." +
							String(globalSettings.esp32ManagerConfig.staticIp[2]) + "." +
							String(globalSettings.esp32ManagerConfig.staticIp[3]));

	doc["staticGateway"] = (String(globalSettings.esp32ManagerConfig.staticGatewayIp[0]) + "." +
									String(globalSettings.esp32ManagerConfig.staticGatewayIp[1]) + "." +
									String(globalSettings.esp32ManagerConfig.staticGatewayIp[2]) + "." +
									String(globalSettings.esp32ManagerConfig.staticGatewayIp[3]));

	
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
	doc["bankId"] = presets[bankNum].id;
	doc["bankName"] = presets[bankNum].name;
	doc["secondaryText"] = presets[bankNum].secondaryText;

	doc["colourOverride"] = (bool)presets[bankNum].colourOverrideFlag;
	doc["colour"] = presets[bankNum].colourOverride;
	doc["textColourOverride"] = (bool)presets[bankNum].textColourOverrideFlag;
	doc["textColour"] = presets[bankNum].textColourOverride;
	
	doc["bpm"] = presets[bankNum].bpm;

	for(uint8_t i=0; i<2; i++)
	{
		// Press messages
		doc["switches"][i]["pressMessages"]["numMessages"] = presets[bankNum].numSwitchPressMessages[i];
		packMessageStack(	doc["switches"][i]["pressMessages"]["messages"].to<JsonArray>(),
									presets[bankNum].switchPressMessages[i], presets[bankNum].numSwitchPressMessages[i]);
		// Hold messages
		doc["switches"][i]["holdMessages"]["numMessages"] = presets[bankNum].numSwitchHoldMessages[i];
		packMessageStack(	doc["switches"][i]["holdMessages"]["messages"].to<JsonArray>(),
									presets[bankNum].switchHoldMessages[i], presets[bankNum].numSwitchHoldMessages[i]);
	}	
	// Custom messages
	doc["customMessages"]["numMessages"] = presets[bankNum].numCustomMessages;
	packMessageStack(	doc["customMessages"]["messages"].to<JsonArray>(),
										presets[bankNum].customMessages, presets[bankNum].numCustomMessages);

	// Preset messages
	doc["presetMessages"]["numMessages"] = presets[bankNum].numPresetMessages;
	packMessageStack(	doc["presetMessages"]["messages"].to<JsonArray>(),
										presets[bankNum].presetMessages, presets[bankNum].numPresetMessages);

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
		Serial.write(appData	, strlen(appData));
		return;
	}

	const char* newDeviceName = doc[USB_DEVICE_NAME_STRING];
	strcpy(globalSettings.deviceName, newDeviceName);
	//globalSettings.currentPreset = doc[USB_CURRENT_BANK_STRING];
	globalSettings.profileId = doc[USB_PROFILE_ID_STRING];
	
	if(strcmp(doc["lightMode"], "light") == 0)
		globalSettings.uiLightMode = UI_MODE_LIGHT;
	else if(strcmp(doc["lightMode"], "dark") == 0)
		globalSettings.uiLightMode = UI_MODE_DARK;

	globalSettings.mainColour = rgb888_to_rgb565(doc["mainColour"]);
	globalSettings.textColour = rgb888_to_rgb565(doc["textColour"]);
	// todo brightness
	uint8_t tempBrightness = doc["displayBrightness"];
	globalSettings.displayBrightness = devApi_roundMap(doc["displayBrightness"], 0, 100, 0, 255);


	// MIDI channels
	globalSettings.midiChannel = doc[USB_MIDI_CHANNEL_STRING];
	
	globalSettings.globalBpm = doc["globalBpm"];

	// MIDI out port mode
	if(strcmp(doc["midiOutPortMode"], "midiOutA") == 0)
		globalSettings.midiOutMode = MIDI_OUT_TYPE_A;
	else if(strcmp(doc["midiOutPortMode"], "midiOutB") == 0)
		globalSettings.midiOutMode = MIDI_OUT_TYPE_B;

	

	// USBD thru handles
	globalSettings.usbdThruHandles[MidiUSBD] = (uint8_t)doc[USB_USBD_THRU_HANDLES_STRING][USB_USBD_STRING];
	globalSettings.usbdThruHandles[MidiBLE] = (uint8_t)doc[USB_USBD_THRU_HANDLES_STRING][USB_BLE_STRING];
	//globalSettings.usbdThruHandles[MidiWiFiRTP] = (uint8_t)doc[USB_USBD_THRU_HANDLES_STRING][USB_WIFI_STRING];
	globalSettings.usbdThruHandles[MidiSerial1] = (uint8_t)doc[USB_USBD_THRU_HANDLES_STRING][USB_MIDI1_STRING];

	// BLE thru handles
	globalSettings.bleThruHandles[MidiUSBD] = (uint8_t)doc[USB_BLE_THRU_HANDLES_STRING][USB_USBD_STRING];
	globalSettings.bleThruHandles[MidiBLE] = (uint8_t)doc[USB_BLE_THRU_HANDLES_STRING][USB_BLE_STRING];
	//globalSettings.bleThruHandles[MidiWiFiRTP] = (uint8_t)doc[USB_BLE_THRU_HANDLES_STRING][USB_WIFI_STRING];
	globalSettings.bleThruHandles[MidiSerial1] = (uint8_t)doc[USB_BLE_THRU_HANDLES_STRING][USB_MIDI1_STRING];

	// WIFI thru handles
	//globalSettings.wifiThruHandles[MidiUSBD] = (uint8_t)doc[USB_WIFI_THRU_HANDLES_STRING][USB_USBD_STRING];
	//globalSettings.wifiThruHandles[MidiBLE] = (uint8_t)doc[USB_WIFI_THRU_HANDLES_STRING][USB_BLE_STRING];
	//globalSettings.wifiThruHandles[MidiWiFiRTP] = (uint8_t)doc[USB_WIFI_THRU_HANDLES_STRING][USB_WIFI_STRING];
	//globalSettings.wifiThruHandles[MidiSerial1] = (uint8_t)doc[USB_WIFI_THRU_HANDLES_STRING][USB_MIDI1_STRING];

	// MIDI 1 thru handles
	globalSettings.midi1ThruHandles[MidiUSBD] = (uint8_t)doc[USB_MIDI1_THRU_HANDLES_STRING][USB_USBD_STRING];
	globalSettings.midi1ThruHandles[MidiBLE] = (uint8_t)doc[USB_MIDI1_THRU_HANDLES_STRING][USB_BLE_STRING];
	//globalSettings.midi1ThruHandles[MidiWiFiRTP] = (uint8_t)doc[USB_MIDI1_THRU_HANDLES_STRING][USB_WIFI_STRING];
	globalSettings.midi1ThruHandles[MidiSerial1] = (uint8_t)doc[USB_MIDI1_THRU_HANDLES_STRING][USB_MIDI1_STRING];


	// Clock output handles
	globalSettings.midiClockOutHandles[MidiUSBD] = (uint8_t)doc[USB_MIDI_CLOCK_OUT_HANDLES_STRING][USB_USBD_STRING];
	globalSettings.midiClockOutHandles[MidiBLE] = (uint8_t)doc[USB_MIDI_CLOCK_OUT_HANDLES_STRING][USB_BLE_STRING];
	//globalSettings.midiClockOutHandles[MidiWiFiRTP] = (uint8_t)doc[USB_MIDI_CLOCK_OUT_HANDLES_STRING][USB_WIFI_STRING];
	globalSettings.midiClockOutHandles[MidiSerial1] = (uint8_t)doc[USB_MIDI_CLOCK_OUT_HANDLES_STRING][USB_MIDI1_STRING];
	

	// Switch messages
	for(uint8_t i=0; i<2; i++)
	{
		if(strcmp(doc["switches"][i]["mode"], "pressPresetUp") == 0)
			globalSettings.switchMode[i] = SwitchPressPresetUp;
		else if(strcmp(doc["switches"][i]["mode"], "pressPresetDown") == 0)
			globalSettings.switchMode[i] = SwitchPressPresetDown;
		else if(strcmp(doc["switches"][i]["mode"], "holdPresetUp") == 0)
			globalSettings.switchMode[i] = SwitchHoldPresetUp;
		else if(strcmp(doc["switches"][i]["mode"], "holdPresetDown") == 0)
			globalSettings.switchMode[i] = SwitchHoldPresetDown;
		else
			globalSettings.switchMode[i] = SwitchMidiOnly;

		// Press messages
		globalSettings.numSwitchPressMessages[i] = doc["switches"][i]["pressMessages"]["numMessages"];
		parseMessageStack(doc["switches"][i]["pressMessages"]["messages"],
									globalSettings.switchPressMessages[i], globalSettings.numSwitchPressMessages[i]);
		// Hold messages
		globalSettings.numSwitchHoldMessages[i] = doc["switches"][i]["holdMessages"]["numMessages"];
		parseMessageStack(doc["switches"][i]["holdMessages"]["messages"],
									globalSettings.switchHoldMessages[i], globalSettings.numSwitchHoldMessages[i]);
	}

	// Custom messages
	globalSettings.numCustomMessages = doc["customMessages"]["numMessages"];
	parseMessageStack(doc["customMessages"]["messages"],
								globalSettings.customMessages, globalSettings.numCustomMessages);

	// CC assignments
	globalSettings.presetUpCC = doc["presetUpCC"];
	globalSettings.presetDownCC = doc["presetDownCC"];
	globalSettings.goToPresetCC = doc["goToPresetCC"];
	globalSettings.globalCustomMessagesCC = doc["globalCustomMessagesCC"];
	globalSettings.presetCustomMessagesCC = doc["presetCustomMessagesCC"];

	// Block wireless MIDI events until the device reboots
	blockWirelessMidi = 1;
	// ESP32 Manager config
	if(strcmp(doc["wirelessType"], "wifi") == 0)
		globalSettings.esp32ManagerConfig.wirelessType = Esp32WiFi;
	else if(strcmp(doc["wirelessType"], "ble") == 0)
	{
		// If the device is switching from a WiFi configuration, disable the WiFi and reset the settings
		if(globalSettings.esp32ManagerConfig.wirelessType == Esp32WiFi)
		{
			wifi_Disconnect();
			//wifi_ResetSettings();
		}
		globalSettings.esp32ManagerConfig.wirelessType = Esp32BLE;
	}
	else
		globalSettings.esp32ManagerConfig.wirelessType = Esp32None;

	if(strcmp(doc["bleMode"], "server") == 0)
		globalSettings.esp32ManagerConfig.bleMode = Esp32BLEServer;
	else if(strcmp(doc["bleMode"], "client") == 0)
		globalSettings.esp32ManagerConfig.bleMode = Esp32BLEClient;

	// Static IP
	globalSettings.esp32ManagerConfig.useStaticIp = (uint8_t)doc["useStaticIp"];

	String staticIpString = doc["staticIp"];
	int ipParts[4] = {0, 0, 0, 0};
	sscanf(staticIpString.c_str(), "%d.%d.%d.%d", &ipParts[0], &ipParts[1], &ipParts[2], &ipParts[3]);
	for(int i=0; i<4; i++)
	{
		if(ipParts[i] < 0)
			ipParts[i] = 0;
		else if(ipParts[i] > 255)
			ipParts[i] = 255;
		globalSettings.esp32ManagerConfig.staticIp[i] = (uint8_t)ipParts[i];
	}

	String gatewayIpString = doc["gatewayIp"];
	int gatewayParts[4] = {0, 0, 0, 0};
	sscanf(gatewayIpString.c_str(), "%d.%d.%d.%d", &gatewayParts[0], &gatewayParts[1], &gatewayParts[2], &gatewayParts[3]);
	for(int i=0; i<4; i++)
	{
		if(gatewayParts[i] < 0)
			gatewayParts[i] = 0;
		else if(gatewayParts[i] > 255)
			gatewayParts[i] = 255;
		globalSettings.esp32ManagerConfig.staticGatewayIp[i] = (uint8_t)gatewayParts[i];
	}

	esp32Settings_SaveGlobalSettings();
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
	
	presets[bankNum].id = doc["bankId"];
	const char* newPresetName = doc["bankName"];
	strcpy(presets[bankNum].name, newPresetName);
	const char* newSecondaryText = doc["secondaryText"];
	strcpy(presets[bankNum].secondaryText, newSecondaryText);
	presets[bankNum].colourOverrideFlag = (bool)doc["colourOverride"];
	presets[bankNum].colourOverride = rgb888_to_rgb565(doc["colour"]);
	presets[bankNum].textColourOverrideFlag = (bool)doc["textColourOverride"];
	presets[bankNum].textColourOverride = rgb888_to_rgb565(doc["textColour"]);
	presets[bankNum].bpm = doc["bpm"];

	// Switch messages
	for(uint8_t i=0; i<2; i++)
	{
		// Press messages
		presets[bankNum].numSwitchPressMessages[i] = doc["switches"][i]["pressMessages"]["numMessages"];
		parseMessageStack(doc["switches"][i]["pressMessages"]["messages"],
									presets[bankNum].switchPressMessages[i], presets[bankNum].numSwitchPressMessages[i]);
		// Hold messages
		presets[bankNum].numSwitchHoldMessages[i] = doc["switches"][i]["holdMessages"]["numMessages"];
		parseMessageStack(doc["switches"][i]["holdMessages"]["messages"],
									presets[bankNum].switchHoldMessages[i], presets[bankNum].numSwitchHoldMessages[i]);
	}

	// Preset messages
	presets[bankNum].numPresetMessages = doc["presetMessages"]["numMessages"];
	parseMessageStack(doc["presetMessages"]["messages"],
								presets[bankNum].presetMessages, presets[bankNum].numPresetMessages);

	// Custom messages
	presets[bankNum].numCustomMessages = doc["customMessages"]["numMessages"];
	parseMessageStack(doc["customMessages"]["messages"],
								presets[bankNum].customMessages, presets[bankNum].numCustomMessages);

	esp32Settings_SavePresets();
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
		uint8_t numCommands = doc[USB_COMMAND_STRING].size();
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
					esp32Settings_SavePresets();
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
				else if(strcmp(command, "checkFirmwareUpdate") == 0)
				{
					ESP32OTAPull ota;
					int otaCheckResult = ota.CheckForOTAUpdate(OTA_INFO_URL, "0.1.0", ota.DONT_DO_UPDATE);
					if(otaCheckResult == ESP32OTAPull::UPDATE_AVAILABLE)
					{
						ESP_LOGI(DEVICE_API_TAG, "OTA update available");
						Serial.print("{\"otaResult\":\"updateAvailable\"}");
					}
					else if(otaCheckResult == ESP32OTAPull::NO_UPDATE_AVAILABLE)
					{
						ESP_LOGI(DEVICE_API_TAG, "No OTA update available");
						Serial.print("{\"otaResult\":\"noUpdateAvailable\"}");
					}
					else
					{
						ESP_LOGI(DEVICE_API_TAG, "OTA check result: %d", otaCheckResult);
						Serial.printf("{\"otaResult\":\"error\"}");
					}
				}
				else if(strcmp(command, "doFirmwareUpdate") == 0)
				{
					ESP32OTAPull ota;
					int otaCheckResult = ota.CheckForOTAUpdate(OTA_INFO_URL, "0.1.0");
				}
				else if(strcmp(command, "checkLatestFirmwareVersion") == 0)
				{
					Serial.println(ota_GetLatestVersion(OTA_INFO_URL));
				}
				else
				{
					ESP_LOGD(DEVICE_API_TAG, "Unknown CTRL command: %s", command);
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
				if(!doc[USB_COMMAND_STRING][i]["wifiSsid"].isNull())
				{
					const char* ssidPtr = doc[USB_COMMAND_STRING][i]["wifiSsid"];
					strcpy(globalSettings.wifiSsid, ssidPtr);
					esp32Settings_SaveGlobalSettings();
				}
				if(!doc[USB_COMMAND_STRING][i]["wifiPassword"].isNull())
				{
					const char* passwordPtr = doc[USB_COMMAND_STRING][i]["wifiPassword"];
					strcpy(globalSettings.wifiPassword, passwordPtr);
					esp32Settings_SaveGlobalSettings();
				}
			}
		}
	}
}

void packMessageStack(const JsonArray& jsonArray, MidiMessage* messages, uint16_t numMessages)
{
	if(messages == NULL)
	{
		return;
	}
	// If there are no messages, return an empty array
	if(numMessages == 0)
	{
		//jsonArray[0] = JsonArray();
		//return;
	}
	for(uint16_t i=0; i<numMessages; i++)
	{
		// Parse the standard MIDI message component
		jsonArray[i][USB_STATUS_BYTE_STRING] = messages[i].status;
		// MIDI outputs
		// Normal MIDI messages
		jsonArray[i][USB_DATA_BYTE1_STRING] = messages[i].data1;
		jsonArray[i][USB_DATA_BYTE2_STRING] = messages[i].data2;
		for(uint8_t j=0; j<NUM_MIDI_INTERFACES; j++)
		{
			jsonArray[i][USB_MIDI_OUTPUTS_STRING][midiInterfaceStrings[j]] = false;
			if((messages[i].midiInterface>>j) & 1)
			{
				jsonArray[i][USB_MIDI_OUTPUTS_STRING][midiInterfaceStrings[j]] = true;
			}
		}
	}
}

void parseMessageStack(const JsonArray& jsonArray, MidiMessage* messages, uint16_t numMessages)
{
	MidiMessage* message;
	
	for(uint16_t i=0; i<numMessages; i++)
	{
		message = &messages[i];
		message->status = jsonArray[i][USB_STATUS_BYTE_STRING];
		
		// MIDI outputs
		message->data1 = jsonArray[i][USB_DATA_BYTE1_STRING];
		message->data2 = jsonArray[i][USB_DATA_BYTE2_STRING];
		message->midiInterface = 0;
		for(uint8_t j=0; j<NUM_MIDI_INTERFACES; j++)
		{
			if(jsonArray[i][USB_MIDI_OUTPUTS_STRING][midiInterfaceStrings[j]] == true)
			{
				message->midiInterface |= (1<<j);
			}
		}
	}
}

uint16_t rgb888_to_rgb565(uint32_t rgb888)
{
    // Extract 8-bit components from 24-bit color
    uint8_t r = (rgb888 >> 16) & 0xFF;  // Red component
    uint8_t g = (rgb888 >> 8) & 0xFF;   // Green component
    uint8_t b = rgb888 & 0xFF;          // Blue component
    
    // Convert to 5-6-5 format by scaling and bit shifting
    uint16_t r5 = (r >> 3) & 0x1F;      // 5 bits for red
    uint16_t g6 = (g >> 2) & 0x3F;      // 6 bits for green
    uint16_t b5 = (b >> 3) & 0x1F;      // 5 bits for blue
    
    // Combine into 16-bit value: RRRRR GGGGGG BBBBB
    return (r5 << 11) | (g6 << 5) | b5;
}