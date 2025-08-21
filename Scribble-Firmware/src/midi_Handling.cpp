#include "MIDI.h"
// BLE MIDI
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32.h>
// General application
#include "midi_handling.h"
#include "main.h"
#include "hardware_def.h"
#include "task_priorities.h"
// MIDI Clock
#include <uClock.h>

static const char* MIDI_TAG = "MIDI";

uint8_t bleConnected = 0; 			// BLE state, 0 = not connected, 1 = connected
uint8_t newBleEvent = 0;			// Flag to indicate a new BLE event has occurred
uint8_t midiReceived = 0;
uint8_t newClockEvent = 0;


// TRS MIDI instance
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, trsMidi)

// BLE MIDI instance
BLEMIDI_CREATE_INSTANCE(BLE_MIDI_DEVICE_NAME, blueMidi)

// Private function declarations
void midi_SetOutTypeA();
void midi_SetOutTypeB();


void midi_ExternalClockHandler();
void midi_ExternalClockStart();
void midi_ExternalClockStop();

void midi_OnSync24Callback(uint32_t tick);
void midi_OnClockStart();
void midi_OnClockStop();

void midi_BleConnectedHandler();
void midi_BleDisconnectedHandler();

void midi_ClockTask(void* parameter);


//---------------- Setup and Utility ----------------//
void midi_Init()
{
	BaseType_t taskResult = xTaskCreatePinnedToCore(
		midi_ClockTask, // Task function. 
		"MIDI Clock Task", // name of task. 
		5000, // Stack size of task 
		NULL, // parameter of the task 
		MIDI_CLOCK_TASK_PRIORITY, // priority of the task 
		NULL, // Task handle to keep track of created task 
		1); // pin task to core 1 
	ESP_LOGI(MIDI_TAG, "MIDI Clock task created: %d", taskResult);

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
	trsMidi.setHandleClock(midi_ExternalClockHandler);
	trsMidi.setHandleStart(midi_ExternalClockStart);
	trsMidi.setHandleStop(midi_ExternalClockStop);

	blueMidi.setHandleControlChange(midi_ControlChangeHandler);
	blueMidi.setHandleProgramChange(midi_ProgramChangeHandler);
	blueMidi.setHandleSystemExclusive(midi_SysExHandler);
	blueMidi.setHandleClock(midi_ExternalClockHandler);
	blueMidi.setHandleStart(midi_ExternalClockStart);
	blueMidi.setHandleStop(midi_ExternalClockStop);

	BLEblueMidi.setHandleConnected(midi_BleConnectedHandler);	
	BLEblueMidi.setHandleDisconnected(midi_BleDisconnectedHandler);	

	// Configure same port thru routing
	// TRS MIDI
	if(globalSettings.midiTrsThruHandles[MIDI_TRS])
		trsMidi.turnThruOn();
	else
		trsMidi.turnThruOff();

	if(globalSettings.midiBleThruHandles[MIDI_BLE])
		blueMidi.turnThruOn();
	else
		blueMidi.turnThruOff();

	// Initialize MIDI interfaces
	trsMidi.begin(globalSettings.midiChannel);
	if(globalSettings.wirelessType == WIRELESS_MODE_BLE)
	{
		blueMidi.begin(globalSettings.midiChannel);
	}

	// Clock setup
	if(globalSettings.clockMode != MIDI_CLOCK_OFF)
	{
		if(globalSettings.clockMode == MIDI_CLOCK_GLOBAL)
		{
			uClock.setMode(uClock.EXTERNAL_CLOCK);
		}
		else if(	globalSettings.clockMode == MIDI_CLOCK_PRESET ||
					globalSettings.clockMode == MIDI_CLOCK_GLOBAL)
		{
			uClock.setMode(uClock.INTERNAL_CLOCK);
			midi_SetTempo();
		}

		uClock.init();
		uClock.start();
	}
	
	uClock.setOnSync24(midi_OnSync24Callback );
	uClock.setOnClockStart(midi_OnClockStart);
	uClock.setOnClockStop(midi_OnClockStop);
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

void midi_ReadAll()
{
	// Read from TRS MIDI
	if(trsMidi.read())
	{
		//trsMidiReceived = 1;
	}

	// Read from BLE MIDI
	if(globalSettings.wirelessType == WIRELESS_MODE_BLE)
	{
		if(blueMidi.read())
		{
			//bleMidiReceived = 1;
		}
	}
}


//---------------- Transmission ----------------//
void midi_Send(MidiMessage message)
{
	// Channel messages
	if((message.statusByte & 0xF0) <= midi::PitchBend)
	{
		uint8_t type = message.statusByte & 0xF0;
		uint8_t channel = message.statusByte & 0x0F;
		trsMidi.send((midi::MidiType)type, message.data1Byte, message.data2Byte, channel);
	}
	else
	{
		trsMidi.send((midi::MidiType)message.statusByte, message.data1Byte, message.data2Byte, 0);
	}
}



//---------------- MIDI Clock ----------------//
void midi_ExternalClockHandler()
{
	uClock.clockMe();
}

void midi_ExternalClockStart()
{
  uClock.start();
  midiReceived = 1;
  newClockEvent = MIDI_CLOCK_EVENT_START;
}

void midi_ExternalClockStop()
{
  uClock.stop();
  midiReceived = 1;
  newClockEvent = MIDI_CLOCK_EVENT_STOP;
}

void midi_OnSync24Callback(uint32_t tick)
{
	static uint8_t bpm_blink_timer = 1;
  	if(globalSettings.clockMode == MIDI_CLOCK_PRESET ||
		globalSettings.clockMode == MIDI_CLOCK_GLOBAL)
	{
		if(globalSettings.midiClockOutHandles[MIDI_TRS])
		{
			trsMidi.sendClock();
		}
		if(globalSettings.midiClockOutHandles[MIDI_BLE])
		{
			blueMidi.sendClock();
		}
	}
	// BPM indicator
	// First downbeat
	if ( !(tick % (96)) || (tick == 1) )
	{  
		bpm_blink_timer = 8;
		newClockEvent = MIDI_CLOCK_INDICATOR_ON;
	}
	// Each quarter note
	else if ( !(tick % (24)) )
	{   
		bpm_blink_timer = 1;
		newClockEvent = MIDI_CLOCK_INDICATOR_ON;
	}
	// Other clock intervals
	else if ( !(tick % bpm_blink_timer) )
	{
		newClockEvent = MIDI_CLOCK_INDICATOR_OFF;
	}
}

// The callback function wich will be called when clock starts by using Clock.start() method.
void midi_OnClockStart()
{
	if(globalSettings.clockMode == MIDI_CLOCK_PRESET ||
		globalSettings.clockMode == MIDI_CLOCK_GLOBAL)
	{
		if(globalSettings.midiClockOutHandles[MIDI_TRS])
		{
			trsMidi.sendStart();
		}
		if(globalSettings.midiClockOutHandles[MIDI_BLE])
		{
			blueMidi.sendStart();
		}
	}
}

// The callback function wich will be called when clock stops by using Clock.stop() method.
void midi_OnClockStop()
{
	if(globalSettings.clockMode == MIDI_CLOCK_PRESET ||
		globalSettings.clockMode == MIDI_CLOCK_GLOBAL)
	{
		if(globalSettings.midiClockOutHandles[MIDI_TRS])
		{
			trsMidi.sendStop();
		}
		if(globalSettings.midiClockOutHandles[MIDI_BLE])
		{
			blueMidi.sendStop();
		}
	}
}
 
void midi_SetTempo()
{
	// Ensure the MIDI clock can only be set in a valid configuration
	if(globalSettings.clockMode == MIDI_CLOCK_GLOBAL ||
		globalSettings.clockMode == MIDI_CLOCK_PRESET)
	{
		float newTempo;
		if(globalSettings.clockMode == MIDI_CLOCK_GLOBAL)
		{
			newTempo = globalSettings.globalBpm;
		}
		else if(globalSettings.clockMode == MIDI_CLOCK_PRESET)
		{
			newTempo = presets[globalSettings.currentPreset].bpm;
		}
		uClock.setTempo(newTempo);
	}
}


//---------------- BLE Events ----------------//
void midi_BleConnectedHandler()
{
	ESP_LOGI(MIDI_TAG, "BLE MIDI connected");
	bleConnected = 1;
	newBleEvent = 1;
}

void midi_BleDisconnectedHandler()
{
	ESP_LOGI(MIDI_TAG, "BLE MIDI disconnected");
	bleConnected = 0;
	newBleEvent = 1;
}


//---------------- SysEx ----------------//
void midi_SendDeviceApiSysExString(const char* array, unsigned size, uint8_t containsFraming)
{
	
}


//---------------- Free RTOS ----------------//
void midi_ClockTask(void* parameter)
{
	ESP_LOGI(MIDI_TAG, "MIDI Clock task started");
	while(1)
	{
		// If the clock is in external mode, handle the external clock
		if(globalSettings.clockMode == MIDI_CLOCK_EXTERNAL)
		{
			// Get the current tempo and compare to the previous tempo to check for significant changes
			float newTempo = uClock.getTempo();
			// Round to 1 decimal place
			newTempo = roundf(newTempo);
			if(newTempo != currentBpm)
			{
				currentBpm = newTempo;
				ESP_LOGE(MIDI_TAG, "New BPM: %.1f", currentBpm);
				newClockEvent = MIDI_CLOCK_EVENT_CHANGE;
			}
		}

		vTaskDelay(20 / portTICK_PERIOD_MS);
	}
}
