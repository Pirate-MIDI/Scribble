#ifndef MAIN_H
#define MAIN_H
#include "stdint.h"

#define NUM_PRESETS 			128

#define MIDI_OUT_TYPE_A			0
#define MIDI_OUT_TYPE_B			1
#define BLE_MIDI_DEVICE_NAME 	"Scribble-BLE"
#define RTP_SESSION_NAME		"Scribble-RTP"

// MIDI interface indices for thru routing
#define MIDI_TRS					0	
#define MIDI_BLE					1	
#define NUM_MIDI_INTERFACES	2	// TRS, BLE
#define MIDI_INDICATOR_ON_TIME		80 // Time in milliseconds for MIDI indicator to stay on

#define MIDI_CLOCK_PRESET		0
#define MIDI_CLOCK_EXTERNAL	1
#define MIDI_CLOCK_GLOBAL		2
#define MIDI_CLOCK_OFF			3

#define WIRELESS_MODE_NONE		0
#define WIRELESS_MODE_BLE		1

#define UI_MODE_LIGHT		0
#define UI_MODE_DARK			1
#define UI_MODE_AUTO			2

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
	
	// MIDI settings
	uint8_t midiChannel;				// MIDI channel for incoming messages
	float globalBpm;					// Global BPM value
	uint8_t midiOutMode; 			// 0 = Type A, 1 = Type B
	uint8_t clockMode;				
	uint8_t clockDisplayType;		// 0 = BPM, 1 = millisecond, 2 = flashing indicator
	uint8_t midiTrsThruHandles[NUM_MIDI_INTERFACES];
	uint8_t midiBleThruHandles[NUM_MIDI_INTERFACES];
	uint8_t midiClockOutHandles[NUM_MIDI_INTERFACES];
	// Connectivity settings
	uint8_t wirelessType;			// 0 = None, 1 = BLE
} GlobalSettings;

typedef struct
{
	uint32_t id;							// Preset ID;
	char name[16+1];							// Preset name
	char secondaryText[16+1];				// Secondary text info
	uint8_t colourOverrideFlag;		// 0 = use main colour, 1 = use preset colour override
	uint16_t colourOverride;			// Main UI colour override (16-bit RGB565)
	uint8_t textColourOverrideFlag;	// 0 = use main colour, 1 = use preset colour override
	uint16_t textColourOverride;		// Main text colour override (16-bit RGB565)
	float bpm;
} Preset;

extern int8_t bleRssi;
extern float currentBpm;
extern GlobalSettings globalSettings;
extern Preset presets[];


void midi_ControlChangeHandler(byte channel, byte number, byte value);
void midi_ProgramChangeHandler(byte channel, byte number);
void midi_SysExHandler(byte* data, unsigned length);
void midi_ClockHandler();

void presetUp();
void presetDown();
void goToPreset(uint16_t presetIndex);
void savePresets();
void enterBootloader();
void factoryReset();

#endif // MAIN_H