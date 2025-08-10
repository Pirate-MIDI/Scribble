#include "MIDI.h"
// BLE MIDI
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32.h>
// WiFi RTP MIDI
#include <AppleMIDI.h>
#include <WiFi.h>
// General application
#include "midi_Handling.h"
#include "main.h"
#include "hardware_Def.h"

static const char* MIDI_TAG = "MIDI";

// TRS MIDI instance
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, trsMidi)

// BLE MIDI instance
BLEMIDI_CREATE_INSTANCE(BLE_MIDI_DEVICE_NAME, blueMidi)

// WiFi RTP MIDI instance
APPLEMIDI_NAMESPACE::AppleMIDISession<WiFiUDP> RTP(RTP_SESSION_NAME, DEFAULT_CONTROL_PORT); \
MIDI_NAMESPACE::MidiInterface<APPLEMIDI_NAMESPACE::AppleMIDISession<WiFiUDP>, APPLEMIDI_NAMESPACE::AppleMIDISettings> rtpMidi((APPLEMIDI_NAMESPACE::AppleMIDISession<WiFiUDP> &)RTP);


// Private function declarations
void midi_SetOutTypeA();
void midi_SetOutTypeB();

void midi_ControlChangeHandler(byte channel, byte number, byte value);
void midi_ProgramChangeHandler(byte channel, byte number);
void midi_SysExHandler(byte* data, unsigned length);
void midi_ClockHandler();


// Public functions
void midi_Init()
{
	// Configure pins
	if(globalSettings.midiOutMode == MIDI_OUT_TYPE_A)
	{
		midi_SetOutTypeA();
	}
	else if(globalSettings.midiOutMode == MIDI_OUT_TYPE_B)
	{
		midi_SetOutTypeB();
	}


	

	// Assign callback handlers
	trsMidi.setHandleControlChange(midi_ControlChangeHandler);
	trsMidi.setHandleProgramChange(midi_ProgramChangeHandler);
	trsMidi.setHandleSystemExclusive(midi_SysExHandler);
	trsMidi.setHandleClock(midi_ClockHandler);

	blueMidi.setHandleControlChange(midi_ControlChangeHandler);
	blueMidi.setHandleProgramChange(midi_ProgramChangeHandler);
	blueMidi.setHandleSystemExclusive(midi_SysExHandler);
	blueMidi.setHandleClock(midi_ClockHandler);

	rtpMidi.setHandleControlChange(midi_ControlChangeHandler);
	rtpMidi.setHandleProgramChange(midi_ProgramChangeHandler);
	rtpMidi.setHandleSystemExclusive(midi_SysExHandler);
	rtpMidi.setHandleClock(midi_ClockHandler);

	// Configure same port thru routing
	// TRS MIDI
	if(globalSettings.midiThruFlags[MIDI_TRS][MIDI_TRS])
		trsMidi.turnThruOn();
	else
		trsMidi.turnThruOff();

	if(globalSettings.midiThruFlags[MIDI_BLE][MIDI_BLE])
		blueMidi.turnThruOn();
	else
		blueMidi.turnThruOff();

	if(globalSettings.midiThruFlags[MIDI_WIFI][MIDI_WIFI])
		rtpMidi.turnThruOn();
	else
		rtpMidi.turnThruOff();


	// Initialize MIDI interfaces
	trsMidi.begin(globalSettings.midiChannel);
	blueMidi.begin(globalSettings.midiChannel);
}

// Because WiFi RTP MIDI requires a network connection, it is initialised separately
void midi_InitWiFiRTP()
{
	if(globalSettings.wirelessType != WIRELESS_MODE_WIFI)
	{
		ESP_LOGI(MIDI_TAG, "WiFi not enabled, cannot start RTP MIDI");
		return;
	}
	else
	{
		if(wifiState == WIFI_STATE_CONNECTED)
		{
			Serial.print("{\"debug\":{\"address\":\"");
			Serial.print(WiFi.localIP());
			Serial.print("\",\"port\":");
			Serial.print(RTP.getPort());
			Serial.print(",\"name\":\"");
			Serial.print(RTP.getName());
			Serial.print("\"}}~\n");
			rtpMidi.begin(globalSettings.midiChannel);
		}
		else
		{
			Serial.println("WiFi not connected, cannot start RTP MIDI");
		}
	}
}

void midi_SetOutTypeA()
{
	// For Type A, use the tip as the transmitting pin and the ring as the current source
	TRS_SERIAL_PORT.begin(31250, MIDI_RX_PIN, MIDI_TX_TIP_PIN);
	pinMode(MIDI_TX_TIP_PIN, OUTPUT);
	digitalWrite(MIDI_TX_TIP_PIN, HIGH);
}

void midi_SetOutTypeB()
{
	// For Type B, use the ring as the transmitting pin and the tip as the current source
	TRS_SERIAL_PORT.begin(31250, MIDI_RX_PIN, MIDI_TX_RING_PIN);
	pinMode(MIDI_TX_RING_PIN, OUTPUT);
	digitalWrite(MIDI_TX_RING_PIN, HIGH);
}

void midi_ControlChangeHandler(byte channel, byte number, byte value)
{
	if(channel == globalSettings.midiChannel || globalSettings.midiChannel == MIDI_CHANNEL_OMNI)
	{
		switch(number)
		{
			case PRESET_UP_CC:
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
				break;
			case PRESET_DOWN_CC:
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
				break;
			case PRESET_SELECT_CC:
				if(value < NUM_PRESETS)
				{
					globalSettings.currentPreset = value;
				}
				break;
			default:
				break;
		}
	}
}

void midi_ProgramChangeHandler(byte channel, byte number)
{
	if(number >= NUM_PRESETS)
	{
		ESP_LOGE(MIDI_TAG, "Invalid program change number: %d", number);
		return;
	}
	globalSettings.currentPreset = number;
}

void midi_SysExHandler(byte* data, unsigned length)
{

}

void midi_ClockHandler()
{

}
