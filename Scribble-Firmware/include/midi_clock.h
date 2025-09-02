#ifndef MIDI_CLOCK_H
#define MIDI_CLOCK_H

#include "stdint.h"

#define MIDI_CLOCK_EVENT_CLEAR	0
#define MIDI_CLOCK_EVENT_CHANGE	1
#define MIDI_CLOCK_EVENT_START	2
#define MIDI_CLOCK_EVENT_STOP		3
#define MIDI_CLOCK_INDICATOR_ON	10
#define MIDI_CLOCK_INDICATOR_OFF	11

void clock_Init();
void clock_Task(void* parameter);
void clock_ExternalClockHandler();
void clock_ExternalClockStart();
void clock_ExternalClockStop();
void clock_OnSync24Callback(uint32_t tick);
void clock_OnClockStart();
void clock_OnClockStop();
void clock_SetTempo();

extern uint8_t bleConnected;
extern uint8_t newBleEvent;
extern uint8_t midiReceived;
extern uint8_t newClockEvent;

#endif // CLOCK_H