#include "midi_clock.h"
#include "main.h"
#include <uClock.h>
#include "esp_log.h"
#include "task_priorities.h"
#include "Arduino.h"

static const char* CLOCK_TAG = "MIDI Clock";

uint8_t midiReceived = 0;
uint8_t newClockEvent = 0;

void clock_Init()
{
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
			clock_SetTempo();
		}

		uClock.init();
		uClock.start();
	}
	
	uClock.setOnSync24(clock_OnSync24Callback );
	uClock.setOnClockStart(clock_OnClockStart);
	uClock.setOnClockStop(clock_OnClockStop);

	// MIDI clock task
	BaseType_t taskResult = xTaskCreatePinnedToCore(
		clock_Task, // Task function. 
		"MIDI Clock Task", // name of task. 
		5000, // Stack size of task 
		NULL, // parameter of the task 
		MIDI_CLOCK_TASK_PRIORITY, // priority of the task 
		NULL, // Task handle to keep track of created task 
		1); // pin task to core 1 
	ESP_LOGI(CLOCK_TAG, "MIDI Clock task created: %d", taskResult);
}

void clock_Task(void* parameter)
{
	ESP_LOGI(CLOCK_TAG, "MIDI Clock task started");
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
				ESP_LOGE(CLOCK_TAG, "New BPM: %.1f", currentBpm);
				newClockEvent = MIDI_CLOCK_EVENT_CHANGE;
			}
		}

		vTaskDelay(20 / portTICK_PERIOD_MS);
	}
}

void clock_ExternalClockHandler()
{
	uClock.clockMe();
}

void clock_ExternalClockStart()
{
  uClock.start();
  midiReceived = 1;
  newClockEvent = MIDI_CLOCK_EVENT_START;
}

void clock_ExternalClockStop()
{
  uClock.stop();
  midiReceived = 1;
  newClockEvent = MIDI_CLOCK_EVENT_STOP;
}

void clock_OnSync24Callback(uint32_t tick)
{
	static uint8_t bpm_blink_timer = 1;
  	if(globalSettings.clockMode == MIDI_CLOCK_PRESET ||
		globalSettings.clockMode == MIDI_CLOCK_GLOBAL)
	{
		if(globalSettings.midiClockOutHandles[MidiUSBD])
			midi_SendMessage(MidiUSBD, midi::Clock, 0, 0, 0);

		if(globalSettings.midiClockOutHandles[MidiBLE])
			midi_SendMessage(MidiBLE, midi::Clock, 0, 0, 0);

#ifdef USE_WIFI_RTP_MIDI
		if(globalSettings.midiClockOutHandles[MidiWiFiRTP])
			midi_SendMessage(MidiWiFiRTP, midi::Clock, 0, 0, 0);
#endif

		if(globalSettings.midiClockOutHandles[MidiSerial1])
			midi_SendMessage(MidiSerial1, midi::Clock, 0, 0, 0);
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
void clock_OnClockStart()
{
	if(globalSettings.clockMode == MIDI_CLOCK_PRESET ||
		globalSettings.clockMode == MIDI_CLOCK_GLOBAL)
	{
		if(globalSettings.midiClockOutHandles[MidiUSBD])
			midi_SendMessage(MidiUSBD, midi::Clock, 0, 0, 0);

		if(globalSettings.midiClockOutHandles[MidiBLE])
			midi_SendMessage(MidiBLE, midi::Clock, 0, 0, 0);
			
#ifdef USE_WIFI_RTP_MIDI
		if(globalSettings.midiClockOutHandles[MidiWiFiRTP])
			midi_SendMessage(MidiWiFiRTP, midi::Clock, 0, 0, 0);
#endif		

		if(globalSettings.midiClockOutHandles[MidiSerial1])
			midi_SendMessage(MidiSerial1, midi::Clock, 0, 0, 0);
	}
}

// The callback function wich will be called when clock stops by using Clock.stop() method.
void clock_OnClockStop()
{
	if(globalSettings.clockMode == MIDI_CLOCK_PRESET ||
		globalSettings.clockMode == MIDI_CLOCK_GLOBAL)
	{
		if(globalSettings.midiClockOutHandles[MidiUSBD])
			midi_SendMessage(MidiUSBD, midi::Clock, 0, 0, 0);

		if(globalSettings.midiClockOutHandles[MidiBLE])
			midi_SendMessage(MidiBLE, midi::Clock, 0, 0, 0);

#ifdef USE_WIFI_RTP_MIDI			
		if(globalSettings.midiClockOutHandles[MidiWiFiRTP])
			midi_SendMessage(MidiWiFiRTP, midi::Clock, 0, 0, 0);
#endif
		
		if(globalSettings.midiClockOutHandles[MidiSerial1])
			midi_SendMessage(MidiSerial1, midi::Clock, 0, 0, 0);
	}
}
 
void clock_SetTempo()
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
