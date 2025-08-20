#include "buttons.h"
#include "Arduino.h"
#include "hardware_def.h"
#include "main.h"
#include "Button2.h"


Button2 switch1;
Button2 switch2;

void switch1Press(Button2& button);
void switch2Press(Button2& button);
void switchPressHandler(uint8_t switchIndex);
void switch1Hold(Button2& button);
void switch2Hold(Button2& button);

void buttons_Init()
{
	// Configure the pins and interrupts
	pinMode(SWITCH1_PIN, INPUT);
	pinMode(SWITCH2_PIN, INPUT);
	switch1.begin(SWITCH1_PIN, INPUT, true);
	switch2.begin(SWITCH2_PIN, INPUT, true);
	switch1.setClickHandler(switch1Press);
	switch2.setClickHandler(switch2Press);
	switch1.setLongClickDetectedHandler(switch1Hold);
	switch2.setLongClickDetectedHandler(switch2Hold);
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
	if(globalSettings.switchMode[switchIndex] == SwitchPresetUp)
	{
		presetUp();
	}
	else if(globalSettings.switchMode[switchIndex] == SwitchPresetDown)
	{
		presetDown();
	}
	else if(globalSettings.switchMode[switchIndex] == SwitchCustom)
	{

	}
}

void switch1Hold(Button2& button)
{

}

void switch2Hold(Button2& button)
{

}