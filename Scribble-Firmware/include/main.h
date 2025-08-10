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
#define MIDI_WIFI					2
#define NUM_MIDI_INTERFACES	3	// TRS, BLE, WiFi

#define WIRELESS_MODE_NONE		0
#define WIRELESS_MODE_BLE		1
#define WIRELESS_MODE_WIFI		2

#define WIFI_STATE_NOT_CONNECTED		0
#define WIFI_STATE_CONNECTED			1
#define WIFI_STATE_AP					2

typedef struct
{
	// System settings
	uint8_t bootState;				// File system boot state flag
	uint16_t currentPreset;			// Current preset index (0-127)
	
	// UI settings
	uint8_t uiLightMode;				// 0 = dark, 1 = light, 2 = auto (follow pedal model)
	uint16_t mainColour;				// Main UI colour (16-bit RGB565)
	uint8_t useLargePresetFont;	// 0 = use small text, 1 = use large text
	
	// MIDI settings
	uint8_t midiChannel;				// MIDI channel for incoming messages
	float globalBpm;					// Global BPM value
	uint8_t midiOutMode; 			// 0 = Type A, 1 = Type B
	uint8_t clockMode;				// 0 = preset BPM, 1 = external MIDI clock, 2 = global BPM
	uint8_t midiThruFlags[NUM_MIDI_INTERFACES][NUM_MIDI_INTERFACES];

	// Connectivity settings
	uint8_t wirelessType;			// 0 = None, 1 = BLE, 2 = WiFi
} GlobalSettings;

typedef struct
{
	char name[32];						// Preset name
	char secondaryText[32];			// Secondary text info
	uint8_t colourOverrideFlag;	// 0 = use main colour, 1 = use preset colour override
	uint16_t colourOverride;		// Main UI colour override (16-bit RGB565)
	float bpm;
} Preset;

extern uint8_t wifiState;			 // 0 = not connected, 1 = connected, 2 = AP
extern float currentBpm;
extern GlobalSettings globalSettings;
extern Preset presets[];
#endif // MAIN_H