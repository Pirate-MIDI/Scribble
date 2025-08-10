#ifndef MIDI_HANDLING_H
#define MIDI_HANDLING_H

#include "MIDI.h"

// MIDI map
#define PRESET_UP_CC				0x01
#define PRESET_DOWN_CC			0x02
#define PRESET_SELECT_CC		0x03

void midi_Init();
void midi_InitWiFiRTP();

#endif // MIDI_HANDLING_H