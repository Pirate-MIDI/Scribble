#include "Arduino.h"
#include	"hardware_Def.h"
#include "Adafruit_ST7789.h"
#include "display.h"
#include "main.h"

Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN);

void display_Init()
{
	SPI.begin(SPI_SCK_PIN, -1, SPI_MOSI_PIN, -1);
	lcd.init(172, 320);           // Init ST7789 172x320
	 lcd.setRotation(3); // rotates the screen
  display_DrawMainScreen();
}

void display_DrawMainScreen()
{
	// Draw main colour boxes
	lcd.fillRect(0, LCD_HEIGHT-MAIN_FILL_HEIGHT, 320, MAIN_FILL_HEIGHT, GEN_LOSS_BLUE);
	lcd.fillRect(0, 0, 320, LCD_HEIGHT-MAIN_FILL_HEIGHT, ST77XX_BLACK);

	// Draw info bars
	lcd.setFont(&PRESET_NUM_FONT);
	lcd.setTextColor(ST77XX_WHITE);
	lcd.setCursor(PRESET_NUM_X_OFFSET, PRESET_NUM_Y_OFFSET);

	lcd.print("Scribble MIDI");

}
