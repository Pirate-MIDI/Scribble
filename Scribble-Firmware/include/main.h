#ifndef MAIN_H
#define MAIN_H
#include "stdint.h"

#define NUM_PRESETS 127

typedef struct
{
	uint8_t bootState;				// File system boot state flag
	uint16_t currentPreset;			// Current preset index (0-127)
	uint8_t midiChannel;				// MIDI channel for incoming messages
	uint8_t uiLightMode;				// 0 = dark, 1 = light, 2 = auto (follow pedal model)
	uint16_t mainColour;				// Main UI colour (16-bit RGB565)
	uint8_t clockMode;				// 0 = preset BPM, 1 = external MIDI clock, 2 = global BPM
	float globalBpm;					// Global BPM value
	uint8_t midiOutMode; 			// 0 = Type A, 1 = Type B
} GlobalSettings;

typedef struct
{
	char name[32];					// Preset name
	char secondaryText[32];		// Secondary text info
	uint16_t colourOverride;	// Main UI colour override (16-bit RGB565)
	float bpm;
} Presets;


extern GlobalSettings globalSettings;
extern Presets presets[];
#endif // MAIN_H