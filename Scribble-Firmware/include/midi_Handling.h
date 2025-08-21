#ifndef MIDI_HANDLING_H
#define MIDI_HANDLING_H

#include "stdint.h"

// MIDI map
#define PRESET_UP_CC					0x01
#define PRESET_DOWN_CC				0x02
#define PRESET_SELECT_CC			0x03
#define CUSTOM_PRESET_STACK_CC	0x10
#define CUSTOM_GLOBAL_STACK_CC	0x11

#define MIDI_CLOCK_EVENT_CLEAR	0
#define MIDI_CLOCK_EVENT_CHANGE	1
#define MIDI_CLOCK_EVENT_START	2
#define MIDI_CLOCK_EVENT_STOP		3
#define MIDI_CLOCK_INDICATOR_ON	10
#define MIDI_CLOCK_INDICATOR_OFF	11

typedef struct
{
	uint8_t statusByte;
	uint8_t data1Byte;
	uint8_t data2Byte;
} MidiMessage;

void midi_Init();

void midi_ReadAll();

void midi_Send(MidiMessage message);

void midi_SendDeviceApiSysExString(const char* array, unsigned size, uint8_t containsFraming);
void midi_SetTempo();
void midi_ControlChangeHandler(byte channel, byte number, byte value);
void midi_ProgramChangeHandler(byte channel, byte number);
void midi_SysExHandler(byte* data, unsigned length);

extern uint8_t bleConnected;
extern uint8_t newBleEvent;
extern uint8_t midiReceived;
extern uint8_t newClockEvent;

#endif // MIDI_HANDLING_H