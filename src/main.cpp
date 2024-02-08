#include <Arduino.h>
#include <FastLED.h>
#include "font.h"
#include "story.h"
#include <vector>

const char *phrases[] = {
  "12345678901234567890",
  "ABCDEFGHIJKLMNOPQRST",
  "UVWXYZ-+*/=<>!@#$%^&",
  "",
  "Holy shit, Batman!!!",
  "<------------------>",
  "Deep aqua seaweeds",
  "Creamy white beams",
  "Warm brown feeling",
  "Cool gray mountain"
};

#define NUM_PHRASES 10
// const char *phrases[] = {
//     "A wizard, old&wise,",
//     "cast a spell:'\xAB\xCD'",
//     "to save`the kingdom",
//     "from dragons@ night.",
//     "He said,\"Beware of ",
//     "the dark!\"$%&'()*+,-",
//     "/;<=>?@[\\]^_`{|}~.",
//     "His apprentice, Zed",
//     "listened closely...",
//     "Armed with^courage,",
//     "Zed journeyed_far.",
//     "Mountains, rivers; ",
//     "forests_enchanted.",
//     "Each step: a new`",
//     "challenge. His path",
//     "twisted&turned~<>",
//     "facing fears, hope,",
//     "and despair. 'Till",
//     "at last, the final",
//     "battle_loomed.`",
//     "With magic words:",
//     "\"Fiat lux!\"{he roared",
//     "and light shone out",
//     "Darkness fled away",
//     "Peace returned, and",
//     "Zed became a hero.",
//     "His tale: timeless.",
//     "End of tale... (32-",
//     "127 ASCII chars)~",
//     "The End. ~Wizardry"
// };

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
    // as it's a zig-zag pattern, we use the row to adjust x
    int x2 = x;
    if (y % 2 == 1){
      int x_start = floor(x/5.0)*5;
      x2 = x_start + 4-(x % 5);
    }
    leds[y*5+x2+offset] = color;
  }
}

void fade_led(uint8_t x, uint8_t y, uint8_t fade) {
  if (x < NUM_CHARS*5 && y < 7) {
    int offset = x/5*30;
    // as it's a zig-zag pattern, we use the row to adjust x
    int x2 = x;
    if (y % 2 == 1){
      int x_start = floor(x/5.0)*5;
      x2 = x_start + 4-(x % 5);
    }
    leds[y*5+x2+offset].fadeToBlackBy(fade);
  }
}

void test_patterns(){
  static int x=0;
  static int y=0;
  set_led(x, y, CRGB::White);
  FastLED.show();
  delay(30);
  set_led(x, y, CRGB::Black);
  FastLED.show();
  delay(2);

  x++;
  if (x >= NUM_CHARS*5){
    x = 0;
    y++;
    if (y >= 7){
      y = 0;
    }
  }
}

void write_character(uint8_t character, uint8_t pos, CRGB color, int offset=0) {
	// Loop through 7 high 5 wide monochrome font  
	for (int py = 0; py < 7; py++) { // rows
    int adjusted_offset = 0;
		for (int px = 0; px < 5; px++) {  // columns
      if (px == abs(offset)-1) continue;
      if (offset < -1){
        adjusted_offset = (px < abs(offset)-1) ? 1 : 0;
      }
			if (FONT_BIT(character - 16, px, py)) {  
				set_led(pos*5 + px + offset + adjusted_offset, py, color);
			} else {
        set_led(pos*5 + px + offset + adjusted_offset, py, CRGB::Black);
      }
		}
	}
}


void scroll_message_smooth(String s, int spos, int scroll_delay){
  String substring = s.substring(spos, spos+21);
  for (int offset=0; offset>=-5; offset--){  
    //Serial.printf("offset: %d\n", offset);
    FastLED.clear();
    for (int pos = 0; pos <= NUM_CHARS; pos++) {
      int prev_space = 0;
      // search backwards from current pos+spos in string for a space
      for (int i = pos+spos; i >= 0; i--){
        if (s.c_str()[i] == ' '){
          prev_space = i;
          break;
        }
      }
      // assign c to the color of the word
      // CRGB c = phrase_colors[(prev_space+1) % NUM_PHRASES];

      // set color based on prev_space
      CRGB c = CHSV((prev_space % 25)*(10), 255, 180);
      char thechar = ' ';
      if (pos < s.length()){ // access limit by end of string
        thechar = s.c_str()[spos+pos];
      }
      write_character(thechar, pos, c, offset);
    }
    FastLED.show();
    delay(scroll_delay);
  }
}

void write_message(String s, int mode=0){
  for (int i = 0; i < NUM_CHARS; i++) {
    char thechar = ' ';
    if (i < s.length()){ // access limit by end of string
      thechar = s.c_str()[i];
    }
    CRGB c = CRGB(50,50,50);
    switch (mode){
      case 0:
        c = CHSV((millis()/100+i*3) % 255, 255, 150);
        break;
      case 1:
      case 2:
        c = phrase_colors[i % 10];
        break;
    }
    if (mode == 2){
      for (int j=0; j<10; j++){
        write_character(random(26)+1, i, nblend(c, CRGB(random(255),random(255),random(255)), 5));
        delay(2+j);
        FastLED.show();
      }  
    }
    write_character(thechar, i, c);
  }
  
}


void story_time(int scroll_delay){
    static int spos = 0;

    if (spos >= 0 && spos + 21 <= story.length()) {
      spos++;
    } else {
      spos = 0;
    }
    scroll_message_smooth(story, spos, scroll_delay);
}

void headlines(){
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

// --------------------------------------------------------------------



void setup() {
  // set up fastled
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, 5, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(10);

  pinMode(0, INPUT_PULLUP);

  // ConnectToWifi();
  //news = fetchHeadlines();
  delay(500);
  Serial.printf("Number of LEDs: %d\n", NUM_LEDS);
  Serial.println("Start");
}

#define NUM_MODES 3
void loop() {
  static int mode = 0;

  // check if button is pressed, if so, change mode
  if (digitalRead(0) == LOW){
    mode = (mode + 1) % NUM_MODES;
    Serial.printf("Button pressed, changing mode to %\n", mode);
    // show visual feedback
    while (digitalRead(0) == LOW){
      CRGB c = CRGB::White;
      FastLED.setBrightness(50);
      c.setHSV(millis()/100 % 255, 255, 50);
      FastLED.showColor(c);
      FastLED.show(); 
      delay(20); 
    }
    Serial.println("Button released");
  }

  switch(mode){
    case 0:
      story_time(1);
      break;
    case 1:
      test_patterns();
      break;
    case 2:
      color_show();
      break;
    case 3:
      headlines();
      break;
  }




}