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
#include "buttons.h"
#include "midi_clock.h"

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

void setOutTypeA();
void setOutTypeB();
void assignMidiCallbacks();

void setup()
{
	Serial.begin(115200);
	Serial.setRxBufferSize(8192);
	ESP_LOGI("MAIN", "Starting setup...");
	assignMidiCallbacks();
	esp32ConfigPtr = &globalSettings.esp32ManagerConfig;
	esp32Manager_Init();
	// Assign global and preset settings and boot the file system
	esp32Settings_AssignDefaultGlobalSettings(defaultGlobalSettingsAssignment);
	esp32Settings_AssignDefaultPresetSettings(defaultPresetsAssignment);
	esp32Settings_BootCheck(&globalSettings, sizeof(GlobalSettings), presets, sizeof(Preset), NUM_PRESETS, &globalSettings.bootState);

	display_Init();

	// Configure pins
	if(globalSettings.midiOutMode == MIDI_OUT_TYPE_A)
	{
		setOutTypeA();
	}
	else if(globalSettings.midiOutMode == MIDI_OUT_TYPE_B)
	{
		setOutTypeB();
	}

	// Display indicator task
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

	// Device API task
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
	clock_Init();
	buttons_Init();
}

void loop()
{
	midi_ReadAll();
	buttons_Process();	
}

void defaultGlobalSettingsAssignment()
{
	// Set the display to indicate the new device configuration process
	display_Init();
	display_ConfigureNewDeviceScreen();
	delay(1000);
	// System settings
	globalSettings.bootState = 0; 						// Initial boot state
	globalSettings.currentPreset = 0; 					// Start with the first preset

	// UI settings
	globalSettings.uiLightMode = UI_MODE_DARK;
	globalSettings.mainColour = GEN_LOSS_BLUE;
	globalSettings.displayBrightness = DEFAULT_DISPLAY_BRIGHTNESS;
	globalSettings.switchMode[0] = SwitchPressPresetDown;
	globalSettings.switchMode[1] = SwitchPressPresetUp;

	// MIDI settings
	globalSettings.midiChannel = MIDI_CHANNEL_OMNI; // Default MIDI channel
	globalSettings.clockMode = MIDI_CLOCK_EXTERNAL; 						// Use preset BPM by default
	globalSettings.globalBpm = 120.0; 					// Default global BPM
	globalSettings.midiOutMode = MIDI_OUT_TYPE_A; 	// Type A MIDI output by default

	// Thru handle assignments
	globalSettings.usbdThruHandles[MidiUSBD] = 1;
	globalSettings.usbdThruHandles[MidiBLE] = 1;
	globalSettings.usbdThruHandles[MidiWiFiRTP] = 1;
	globalSettings.usbdThruHandles[MidiSerial1] = 1;

	globalSettings.bleThruHandles[MidiUSBD] = 1;
	globalSettings.bleThruHandles[MidiBLE] = 1;
	globalSettings.bleThruHandles[MidiWiFiRTP] = 1;
	globalSettings.bleThruHandles[MidiSerial1] = 1;

	globalSettings.wifiThruHandles[MidiUSBD] = 1;
	globalSettings.wifiThruHandles[MidiBLE] = 1;
	globalSettings.wifiThruHandles[MidiWiFiRTP] = 1;
	globalSettings.wifiThruHandles[MidiSerial1] = 1;

	globalSettings.midi1ThruHandles[MidiUSBD] = 1;
	globalSettings.midi1ThruHandles[MidiBLE] = 1;
	globalSettings.midi1ThruHandles[MidiWiFiRTP] = 1;
	globalSettings.midi1ThruHandles[MidiSerial1] = 1;
	
	// Default MIDI mapping
	globalSettings.presetUpCC = PRESET_UP_CC;
	globalSettings.presetDownCC = PRESET_DOWN_CC;
	globalSettings.goToPresetCC = PRESET_SELECT_CC;
	globalSettings.globalCustomMessagesCC = CUSTOM_GLOBAL_STACK_CC;
	globalSettings.presetCustomMessagesCC = CUSTOM_PRESET_STACK_CC;

	globalSettings.wirelessType = WIRELESS_MODE_BLE;

	// Set all message stacks to empty
	for(uint8_t i=0; i<NUM_SWITCH_MESSAGES; i++)
	{
		// A 0 status byte indicates an 'unset' message and the end of available messages
		globalSettings.switchPressMessages[0][i].statusByte = 0;
		globalSettings.switchPressMessages[1][i].statusByte = 0;
		globalSettings.switchHoldMessages[0][i].statusByte = 0;
		globalSettings.switchHoldMessages[1][i].statusByte = 0;
	}
	for(uint8_t i=0; i<NUM_CUSTOM_MESSAGES; i++)
	{
		globalSettings.customMessages[i].statusByte = 0;
	}
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
		for(uint8_t j=0; j<NUM_SWITCH_MESSAGES; j++)
		{
			// A 0 status byte indicates an 'unset' message and the end of available messages
			presets[i].switchPressMessages[0][j].statusByte = 0;
			presets[i].switchPressMessages[1][j].statusByte = 0;
			presets[i].switchHoldMessages[0][j].statusByte = 0;
			presets[i].switchHoldMessages[1][j].statusByte = 0;
		}
		for(uint8_t j=0; j<NUM_PRESET_MESSAGES; j++)
		{
			presets[i].presetMessages[j].statusByte = 0;
		}
		for(uint8_t j=0; j<NUM_CUSTOM_MESSAGES; j++)
		{
			presets[i].customMessages[j].statusByte = 0;
		}
	}
}

void assignMidiCallbacks()
{
	// Assign thru handling pointers
	usbdMidiThruHandlesPtr = globalSettings.usbdThruHandles;
	bleMidiThruHandlesPtr = globalSettings.bleThruHandles;
	wifiMidiThruHandlesPtr = globalSettings.wifiThruHandles;
	serial1MidiThruHandlesPtr = globalSettings.midi1ThruHandles;

	midi_AssignControlChangeCallback(controlChangeHandler);
	midi_AssignProgramChangeCallback(programChangeHandler);
	midi_AssignSysemExclusiveCallback(sysExHandler);
}

void controlChangeHandler(MidiInterfaceType interface, byte channel, byte number, byte value)
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

void programChangeHandler(MidiInterfaceType interface, byte channel, byte number)
{
	if(channel	== globalSettings.midiChannel || globalSettings.midiChannel == MIDI_CHANNEL_OMNI)
	{
		goToPreset(number);	
	}
	midiReceived = 1;
}

void sysExHandler(MidiInterfaceType interface, byte* data, unsigned length)
{

}

void sendMidiMessage(MidiMessage message)
{
	// Get the transmission interfaces
	uint8_t sendUsbd = message.midiInterface & 0x01;
	uint8_t sendBle = (message.midiInterface >> 1) & 0x01;
	uint8_t sendWifi = (message.midiInterface >> 2) & 0x01;
	uint8_t sendSerial1 = (message.midiInterface >> 3) & 0x01;
	// Channel messages
	if((message.statusByte & 0xF0) <= midi::PitchBend)
	{
		uint8_t type = message.statusByte & 0xF0;
		uint8_t channel = message.statusByte & 0x0F;
		if(sendUsbd)
			midi_SendMessage(MidiUSBD, (midi::MidiType)type, channel, message.data1Byte, message.data2Byte);
		if(sendBle)
			midi_SendMessage(MidiBLE, (midi::MidiType)type, channel, message.data1Byte, message.data2Byte);
		if(sendWifi)
			midi_SendMessage(MidiWiFiRTP, (midi::MidiType)type, channel, message.data1Byte, message.data2Byte);
		if(sendSerial1)
			midi_SendMessage(MidiSerial1, (midi::MidiType)type, channel, message.data1Byte, message.data2Byte);
	}
	else
	{
		if(sendUsbd)
			midi_SendMessage(MidiUSBD, (midi::MidiType)message.statusByte, 0, message.data1Byte, message.data2Byte);
		if(sendBle)
			midi_SendMessage(MidiBLE, (midi::MidiType)message.statusByte, 0, message.data1Byte, message.data2Byte);
		if(sendWifi)
			midi_SendMessage(MidiWiFiRTP, (midi::MidiType)message.statusByte, 0, message.data1Byte, message.data2Byte);
		if(sendSerial1)
			midi_SendMessage(MidiSerial1, (midi::MidiType)message.statusByte, 0, message.data1Byte, message.data2Byte);
	}
}

void setOutTypeA()
{
	// For Type A, use the tip as the transmitting pin and the ring as the current source
	TRS_SERIAL_PORT.setPins(MIDI_RX_PIN, MIDI_TX_TIP_PIN, -1, -1);
	pinMode(MIDI_TX_TIP_PIN, OUTPUT);
	digitalWrite(MIDI_TX_TIP_PIN, HIGH);
}

void setOutTypeB()
{
	// For Type B, use the ring as the transmitting pin and the tip as the current source
	TRS_SERIAL_PORT.setPins(MIDI_RX_PIN, MIDI_TX_RING_PIN, -1, -1);
	pinMode(MIDI_TX_RING_PIN, OUTPUT);
	digitalWrite(MIDI_TX_RING_PIN, HIGH);
}


//---------------- MIDI Clock ----------------//

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
	clock_SetTempo();
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
	clock_SetTempo();
}

void goToPreset(uint16_t presetIndex)
{
	if(presetIndex < NUM_PRESETS)
	{
		globalSettings.currentPreset = presetIndex;
	}
	display_DrawPresetNumber(globalSettings.currentPreset);
	display_DrawMainText(presets[globalSettings.currentPreset].name, presets[globalSettings.currentPreset].secondaryText);
	clock_SetTempo();
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