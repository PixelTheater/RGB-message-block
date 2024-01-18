#include <Arduino.h>
#include <FastLED.h>
#include "font.h"

//const char *phrases[] = {
  // "12345678901234567890",
  // "ABCDEFGHIJKLMNOPQRST",
  // "UVWXYZ-+*/=<>!@#$%^&",
  // "Forest green scent",
  // "Rich purple reigns",
  // "Soft pink is sweet",
  // "Deep aqua seaweeds",
  // "Creamy white beams",
  // "Warm brown feeling",
  // "Cool gray mountain"
//};

#define NUM_PHRASES 30
const char *phrases[] = {
    "A wizard, old&wise,",
    "cast a spell:'\xAB\xCD'",
    "to save`the kingdom",
    "from dragons@ night.",
    "He said,\"Beware of ",
    "the dark!\"$%&'()*+,-",
    "/;<=>?@[\\]^_`{|}~.",
    "His apprentice, Zed",
    "listened closely...",
    "Armed with^courage,",
    "Zed journeyed_far.",
    "Mountains, rivers; ",
    "forests_enchanted.",
    "Each step: a new`",
    "challenge. His path",
    "twisted&turned~<>",
    "facing fears, hope,",
    "and despair. 'Till",
    "at last, the final",
    "battle_loomed.`",
    "With magic words:",
    "\"Fiat lux!\"{he roared",
    "and light shone out",
    "Darkness fled away",
    "Peace returned, and",
    "Zed became a hero.",
    "His tale: timeless.",
    "End of tale... (32-",
    "127 ASCII chars)~",
    "The End. ~Wizardry"
};

CRGB phrase_colors[] = {
  CHSV(60, 255, 128),    // Bright yellow
  CHSV(32, 200, 110),    // Radiant orange
  CHSV(160, 179, 100),   // Sky blue
  CHSV(96, 160, 70),     // Forest green
  CHSV(213, 250, 80),    // Rich purple
  CHSV(240, 80, 90),    // Soft pink
  CHSV(125, 190, 90),    // Deep aqua
  CHSV(64, 50, 90),     // Creamy white
  CHSV(31, 140, 90),    // Warm brown
  CHSV(180, 10, 65)       // Cool gray
};

#define NUM_CHARS 20
#define NUM_LEDS 5*7*NUM_CHARS  // 5x7 LEDs per character, 2 characters per display

CRGB leds[NUM_LEDS];

void color_show(){
  // light up all LEDs in sequence
  CRGB color = CRGB::White;
  int c = random(1,255);
  for (int i = 0; i < NUM_LEDS; i++) {
    color.setHSV(c+i/2, 255 - (i%2==0 ? 50 : 0), 70);
    leds[i] = color;
    if (digitalRead(0) == LOW){
      return;
    } else {
      delay(20);
      FastLED.show();
    }
  }
  delay(1000);
  // turn off all LEDs in a dissolving pattern
  for (int x=0; x<50; x++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(3+random(5));
    }
    if (digitalRead(0) == LOW){
      return;
    } else {
      FastLED.show();
      delay(50);
    }
  }
}

void set_led(uint8_t x, uint8_t y, CRGB color) {
  if (x < NUM_CHARS*5 && y < 7) {
    int offset = x/5*30;
    // flip each character horizontally
    // x = (x / 4) + 3 - (x % 4);
    leds[x+y*5+offset] = color;
  }
}

void write_character(uint8_t character, uint8_t i, CRGB color) {
	// Loop through 7 high 5 wide monochrome font
	for (int py = 0; py < 7; py++) { // rows
		for (int px = 0; px < 5; px++) {  // columns
      int col = (py % 2 == 0) ? px : 4 - px;  
			if (FONT_BIT(character - 16, px, py)) {        
				set_led(i*5 + col, py, color);
			} else {
        set_led(i*5 + col, py, CRGB::Black);
      }
		}
	}
}


void scroll_message(String s, CRGB color){
  for (int p=0; p <= (s.length()); p++){
    for (int i = 0; i < NUM_CHARS; i++) {
      char thechar = ' ';
      if ((i+p) < s.length()){ // access limit by end of string
        thechar = s.c_str()[i+p];
      }
      write_character(thechar, i, color);
    }
    if (digitalRead(0) == LOW){
      return;
    } else {
      FastLED.show();
      if (p==0){  
        delay(3000);
      } else {
        delay(10);
      }
    }
  }
  delay(1000);
}




// --------------------------------------------------------------------



void setup() {
  // set up fastled
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, 5, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

  pinMode(0, INPUT_PULLUP);

  // ConnectToWifi();
  //news = fetchHeadlines();
  delay(500);
  Serial.printf("Number of LEDs: %d\n", NUM_LEDS);
  Serial.println("Start");
}


int mode = 0;

void loop() {
  static uint8_t character = 1;
  static uint8_t phrase = 0;

  if (digitalRead(0) == LOW){
    Serial.print("Button pressed, changing mode to ");
    mode = (mode + 1) % 2;
    Serial.println(mode);

 
    while (digitalRead(0) == LOW){
      CRGB c = CRGB::White;
      FastLED.setBrightness(50);
      c.setHSV(millis()/20 % 255, 255, 50);
      FastLED.showColor(c);
      FastLED.show(); 
      delay(10); 
    }
    Serial.println("Button released");
  }

  if (mode == 0){
    CRGB color = phrase_colors[phrase % 10];
    //String s = news[phrase]["title"];
    String s = phrases[phrase];
    scroll_message(s, color);
    // String a = news[phrase]["abstract"];
    // scroll_message(a, color);

    FastLED.clear();
    FastLED.show();
    phrase++;
    phrase = phrase % NUM_PHRASES;

    // // update time
    // time(&now);
    // localtime_r(&now, &timeinfo);
    // hour = timeinfo.tm_hour;
    // minute = timeinfo.tm_min;
    // second = timeinfo.tm_sec;
    // t=0;

    // // check if we need to update the news
    // if (hour != last_hour){
    //   Serial.println(" -> new hour");        
    //   last_hour = hour;
    //   Serial.println(getFormattedTime());
    //   millis_offset = millis();  
    //   //news = fetchHeadlines();
    // }  
  }
  if (mode == 1){
    color_show();
  }
}