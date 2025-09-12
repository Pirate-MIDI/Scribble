#ifndef MAIN_H
#define MAIN_H
#include "stdint.h"
#include "esp32_manager.h"
#include "midi_handling.h"

#define NUM_PRESETS 			128

#define MIDI_OUT_TYPE_A			0
#define MIDI_OUT_TYPE_B			1
#define BLE_MIDI_DEVICE_NAME 	"Scribble-BLE"
#define RTP_SESSION_NAME		"Scribble-RTP"

#define NUM_MIDI_INTERFACES			3	// USBD, BLE, Serial1
#define MIDI_INDICATOR_ON_TIME		80 // Time in milliseconds for MIDI indicator to stay on

#define MIDI_CLOCK_PRESET		0
#define MIDI_CLOCK_EXTERNAL	1
#define MIDI_CLOCK_GLOBAL		2
#define MIDI_CLOCK_OFF			3

#define MIDI_CLOCK_DISPLAY_BPM			0
#define MIDI_CLOCK_DISPLAY_MS				1
#define MIDI_CLOCK_DISPLAY_INDICATOR	2

#define UI_MODE_LIGHT		0
#define UI_MODE_DARK			1
#define UI_MODE_AUTO			2

#define DEFAULT_DISPLAY_BRIGHTNESS	255

#define NUM_SWITCH_MESSAGES			8
#define NUM_PRESET_MESSAGES			8
#define NUM_CUSTOM_MESSAGES			8

// MIDI map
#define PRESET_UP_CC					0x01
#define PRESET_DOWN_CC				0x02
#define PRESET_SELECT_CC			0x03
#define CUSTOM_PRESET_STACK_CC	0x10
#define CUSTOM_GLOBAL_STACK_CC	0x11

typedef enum
{
	SwitchPressPresetUp,
	SwitchPressPresetDown,
	SwitchHoldPresetUp,
	SwitchHoldPresetDown,
	SwitchMidiOnly
} SwitchMode;

typedef struct
{
	// MIDI interface to send the message on
	// Bit maskng is used to preserve memory and allow multiple interfaces
	// Bit 0 = USBD, Bit 1 = BLE, Bit 2 = WiFi, Bit 3 = Serial1
	uint8_t midiInterface;		
	uint8_t status;
	uint8_t data1;
	uint8_t data2;
} MidiMessage;

typedef struct
{
	// System settings
	uint8_t bootState;				// File system boot state flag
	uint32_t profileId;				// Profile ID for the device
	char deviceName[32];				// Device name
	uint16_t currentPreset;			// Current preset index (0-127)
	
	// UI settings
	uint32_t pedalModel;				// Pedal model ID
	uint8_t uiLightMode;				// 0 = dark, 1 = light, 2 = auto (follow pedal model)
	uint16_t mainColour;				// Main UI colour (16-bit RGB565)
	uint16_t textColour;				// Colour for main and secondary text (16-bit RGB565)
	uint8_t displayBrightness;		// 8-bit LCD backlight brightness
	SwitchMode switchMode[2];
	
	// MIDI settings
	uint8_t midiChannel;				// MIDI channel for incoming messages
	float globalBpm;					// Global BPM value
	uint8_t midiOutMode; 			// 0 = Type A, 1 = Type B
	uint8_t clockMode;				
	uint8_t clockDisplayType;		// 0 = BPM, 1 = millisecond, 2 = flashing indicator
	
	// MIDI thru handles
	uint8_t usbdThruHandles[NUM_MIDI_INTERFACES];
	uint8_t bleThruHandles[NUM_MIDI_INTERFACES];
	//uint8_t wifiThruHandles[NUM_MIDI_INTERFACES];
	uint8_t midi1ThruHandles[NUM_MIDI_INTERFACES];

	uint8_t midiClockOutHandles[NUM_MIDI_INTERFACES];
	uint8_t numSwitchPressMessages[2];
	MidiMessage switchPressMessages[2][NUM_SWITCH_MESSAGES];
	uint8_t numSwitchHoldMessages[2];
	MidiMessage switchHoldMessages[2][NUM_SWITCH_MESSAGES];
	uint8_t numCustomMessages;
	MidiMessage customMessages[NUM_CUSTOM_MESSAGES];			// Triggered by external CC
	uint8_t presetUpCC;
	uint8_t presetDownCC;
	uint8_t goToPresetCC;
	uint8_t globalCustomMessagesCC;
	uint8_t presetCustomMessagesCC;

	// ESP32 manager
	Esp32ManagerConfig esp32ManagerConfig;
	char wifiSsid[32];
	char wifiPassword[64];
} GlobalSettings;

typedef struct
{
	uint32_t id;							// Preset ID;
	char name[16+1];						// Preset name
	char secondaryText[16+1];			// Secondary text info
	uint8_t colourOverrideFlag;		// 0 = use main colour, 1 = use preset colour override
	uint16_t colourOverride;			// Main UI colour override (16-bit RGB565)
	uint8_t textColourOverrideFlag;	// 0 = use main colour, 1 = use preset colour override
	uint16_t textColourOverride;		// Main text colour override (16-bit RGB565)
	float bpm;
	uint8_t numSwitchPressMessages[2];
	MidiMessage switchPressMessages[2][NUM_SWITCH_MESSAGES];
	uint8_t numSwitchHoldMessages[2];
	MidiMessage switchHoldMessages[2][NUM_SWITCH_MESSAGES];
	uint8_t numPresetMessages;
	MidiMessage presetMessages[NUM_PRESET_MESSAGES];
	uint8_t numCustomMessages;
	MidiMessage customMessages[NUM_CUSTOM_MESSAGES];	// Triggered by external CC
} Preset;

extern int8_t bleRssi;
extern float currentBpm;
extern GlobalSettings globalSettings;
extern Preset presets[];


void controlChangeHandler(MidiInterfaceType interface, byte channel, byte number, byte value);
void programChangeHandler(MidiInterfaceType interface, byte channel, byte number);
void sysExHandler(MidiInterfaceType interface, byte* data, unsigned length);

void sendMidiMessage(MidiMessage message);

void presetUp();
void presetDown();
void goToPreset(uint16_t presetIndex);
void enterBootloader();
void factoryReset();

#endif // MAIN_H