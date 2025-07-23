#include <Arduino.h>
#include <FastLED.h>
#include "font.h"
#include "led_art.h"
#include "led_history.h"
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
#define MAX_BRIGHTNESS 30

CRGB leds[NUM_LEDS];

// Global static variables for story and mode management
static int spos = 0;
static int currentStoryIndex = 0;

// Declare stories vector globally
std::vector<String> stories;

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
    leds[y*5+x+offset] = color;
  }
}

void fade_led(uint8_t x, uint8_t y, uint8_t fade) {
  if (x < NUM_CHARS*5 && y < 7) {
    int offset = x/5*30;
    leds[y*5+x+offset].fadeToBlackBy(fade);
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


void scroll_message_smooth(String s, int spos, bool scroll=true){
  String substring = s.substring(spos, spos+21);
  int skip_from = 0;
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
      if (skip_from > 0 && pos > skip_from){
        thechar = ' ';
      }
      switch (thechar){
        case '\n':
          thechar = ' ';
          skip_from = pos;
          break;
      }
      write_character(thechar, pos, c, offset);
    }
    if (!scroll){
      return;
    } else {
      FastLED.show();
    }
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


void story_time(){
    static bool start_pause = true;
    // static int currentStoryIndex = 0; // Remove local static declaration

    if (stories[currentStoryIndex].c_str()[spos] == '\n'){
      // animate matrix with a nice transition
      for (int b=1; b<30; b++){
        FastLED.clear();
        for (int x=0; x<NUM_LEDS/5; x++){
          for (int y=0; y<7; y++){
            set_led(x, y, CHSV(abs(sin(b/10.0)*cos(x/10.0))*255, 100+random(b*3,b*4), 130-b*4+random(20)));
          }
        }
        FastLED.show();
        delay(20);
      }
      // reset cursor to just past the newline
      bool printable = false;
      while (!printable && spos < stories[currentStoryIndex].length()){
        char c = stories[currentStoryIndex].c_str()[spos+1];
        if (c == '\n' || c == ' '){
          printable = false;
        } else {
          printable = true;
        }
        spos++;
      }
      start_pause = true;
      FastLED.clear();
    } else {      
      if (start_pause){
        start_pause = false;
        for (int b=30; b>0; b--){
          scroll_message_smooth(stories[currentStoryIndex], spos, false);
          for (int i=0; i<NUM_LEDS; i++){
            leds[i].fadeToBlackBy(random(b*6,b*10));
          }
          FastLED.show();
          delay(20);
        }
        // delay(1000);
      }
      scroll_message_smooth(stories[currentStoryIndex], spos, true);
      if (spos >= 0 && spos + 21 <= stories[currentStoryIndex].length()) {
        spos++;
      } else {
        spos = 0;
        currentStoryIndex = (currentStoryIndex + 1) % stories.size();
      }
    }
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
  FastLED.addLeds<WS2812Controller800Khz, 5, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHTNESS);

  pinMode(0, INPUT_PULLUP);

  // Populate stories vector
  stories.clear();
  stories.push_back(led_art_story);
  stories.push_back(led_history_story);

  delay(500);
  Serial.printf("Number of LEDs: %d\n", NUM_LEDS);
  Serial.println("Start");
}

#define NUM_MODES 3
unsigned long buttonPressTime = 0;
bool longPressActive = false;

void loop() {
  static int mode = 0;

  // Check if button is pressed
  if (digitalRead(0) == LOW) {
    if (buttonPressTime == 0) {
      buttonPressTime = millis(); // Mark the time button was first pressed
    }
    // If button held for more than 1 second, toggle mode
    if (millis() - buttonPressTime > 1000 && !longPressActive) {
      mode = (mode + 1) % NUM_MODES;
      Serial.printf("Button held, changing mode to %d\n", mode);
      longPressActive = true;
      // Visual feedback for long press
      FastLED.setBrightness(MAX_BRIGHTNESS/2);
      for (int i = 0; i < 5; i++) { // Flash 5 times
        FastLED.showColor(CRGB::Blue);
        delay(100);
        FastLED.clear();
        delay(100);
      }
    }
  } else { // Button is released
    if (buttonPressTime > 0) { // Button was pressed
      if (!longPressActive) { // It was a short press
        if (mode == 0) { // Only change story if in story mode
          currentStoryIndex = (currentStoryIndex + 1) % stories.size(); // Use stories.size()
          // Reset story scroll position when changing story
          spos = 0;
          Serial.printf("Button tapped, changing story to %d\n", currentStoryIndex);
        } else {
          mode = 0; // If not in story mode, a short tap reverts to story mode
          Serial.println("Button tapped, reverting to story mode");
        }
        // Visual feedback for short press
        FastLED.setBrightness(MAX_BRIGHTNESS/2);
        FastLED.showColor(CRGB::Green);
        FastLED.clear();
      }
      buttonPressTime = 0; // Reset button press time
      longPressActive = false; // Reset long press flag
    }
  }

  switch(mode){
    case 0:
      story_time();
      break;
    case 1:
      test_patterns();
      break;
    case 2:
      color_show();
      break;
  }




}