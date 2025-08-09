#include "MIDI.h"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, trsMidi);

void midi_Init()
{
	// Configure pins
	trsMidi.begin();
}

void midi_SetOutTypeA()
{

}

void midi_SetOutTypeB()
{

}
