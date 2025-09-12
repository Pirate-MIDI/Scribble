#include "buttons.h"
#include "Arduino.h"
#include "hardware_def.h"
#include "main.h"
#include "Button2.h"


Button2 switch1;
Button2 switch2;

void switch1Press(Button2& button);
void switch2Press(Button2& button);

void switch1Hold(Button2& button);
void switch2Hold(Button2& button);

void switchPressHandler(uint8_t switchIndex);
void switchHoldHandler(uint8_t switchIndex);

void buttons_Init()
{
	// Configure the pins and interrupts
	pinMode(SWITCH1_PIN, INPUT);
	pinMode(SWITCH2_PIN, INPUT);
	switch1.begin(SWITCH1_PIN, INPUT_PULLUP, true);
	switch2.begin(SWITCH2_PIN, INPUT, true);
	switch1.setClickHandler(switch1Press);
	switch2.setClickHandler(switch2Press);
	//switch1.setDoubleClickHandler(switch1Press);
	//switch2.setDoubleClickHandler(switch2Press);
	switch1.setLongClickDetectedHandler(switch1Hold);
	switch2.setLongClickDetectedHandler(switch2Hold);

	// Configure the event times
	switch1.setDoubleClickTime(5);
	switch2.setDoubleClickTime(5);
}

void buttons_Process()
{
	switch1.loop();
	switch2.loop();
}

// Button callbacks and handlers
void switch1Press(Button2& button)
{
	switchPressHandler(0);
}

void switch2Press(Button2& button)
{
	switchPressHandler(1);
}



void switchPressHandler(uint8_t switchIndex)
{
	// Trigger any available message stacks before changing presets (if applicable)
	// Global
	for(uint8_t i=0; i<NUM_SWITCH_MESSAGES; i++)
	{
		if(globalSettings.switchPressMessages[switchIndex][i].status == 0)
			break;

		sendMidiMessage(globalSettings.switchPressMessages[switchIndex][i]);
	}
	// Preset
	for(uint8_t i=0; i<NUM_SWITCH_MESSAGES; i++)
	{
		if(presets[globalSettings.currentPreset].switchPressMessages[switchIndex][i].status == 0)
			break;

		sendMidiMessage(presets[globalSettings.currentPreset].switchPressMessages[switchIndex][i]);
	}

	// Triger any navigation actions
	if(globalSettings.switchMode[switchIndex] == SwitchPressPresetUp)
	{
		presetUp();
	}
	else if(globalSettings.switchMode[switchIndex] == SwitchPressPresetDown)
	{
		presetDown();
	}
}

void switch1Hold(Button2& button)
{
	switchHoldHandler(0);
}

void switch2Hold(Button2& button)
{
	switchHoldHandler(1);
}

void switchHoldHandler(uint8_t switchIndex)
{
	// Trigger any available message stacks before changing presets (if applicable)
	// Global
	for(uint8_t i=0; i<NUM_SWITCH_MESSAGES; i++)
	{
		if(globalSettings.switchHoldMessages[switchIndex][i].status == 0)
			break;

		sendMidiMessage(globalSettings.switchHoldMessages[switchIndex][i]);
	}
	// Preset
	for(uint8_t i=0; i<NUM_SWITCH_MESSAGES; i++)
	{
		if(presets[globalSettings.currentPreset].switchHoldMessages[switchIndex][i].status == 0)
			break;

		sendMidiMessage(presets[globalSettings.currentPreset].switchHoldMessages[switchIndex][i]);
	}

	// Triger any navigation actions
	if(globalSettings.switchMode[switchIndex] == SwitchHoldPresetUp)
	{
		presetUp();
	}
	else if(globalSettings.switchMode[switchIndex] == SwitchHoldPresetDown)
	{
		presetDown();
	}
}