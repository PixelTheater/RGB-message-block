#include <Arduino.h>
#include <FastLED.h>
#include "font.h"
#include "led_art.h"
#include "led_history.h"
#include <vector>

// ===================== CONFIGURATION =====================
#define ENABLE_BENCHMARKING true  // Set to false to disable performance monitoring
                                  // If you get crashes, try setting this to false first

// SCROLL MODE SETTINGS
#define SMOOTH_SCROLLING true     // Enable/disable smooth character transitions
#define CPS_TARGET 15.0            // Characters per second target (when smooth scrolling off)
                                  // Smooth scrolling ignores this and runs at max speed
                                  
// MODE COMPARISON:
// SMOOTH_SCROLLING = true:  Beautiful 6-step transitions, max speed, more FastLED calls
// SMOOTH_SCROLLING = false: Clean 1-step updates, configurable CPS speed, fewer FastLED calls


// Performance benchmarking structures
struct PerformanceMetrics {
  unsigned long totalFrameTime = 0;
  unsigned long fastLEDShowTime = 0;
  unsigned long characterWriteTime = 0;
  unsigned long scrollTime = 0;
  unsigned long calculationTime = 0;
  unsigned long frameCount = 0;
  unsigned long visualUpdateCount = 0; // Track actual FastLED.show() calls
  unsigned long charactersScrolled = 0; // Track character position changes for CPS
  unsigned long lastReportTime = 0;
  unsigned long maxFrameTime = 0;
  unsigned long minFrameTime = ULONG_MAX;
};

PerformanceMetrics perf;

// Macro for easy timing
#if ENABLE_BENCHMARKING
  #define START_TIMER(var) unsigned long var##_start = micros()
  #define END_TIMER(var, accumulator) accumulator += (micros() - var##_start)
  #define END_FASTLED_TIMER(var, accumulator) do { \
    accumulator += (micros() - var##_start); \
    perf.visualUpdateCount++; \
  } while(0)
#else
  #define START_TIMER(var) 
  #define END_TIMER(var, accumulator)
  #define END_FASTLED_TIMER(var, accumulator) perf.visualUpdateCount++
#endif

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

#define NUM_CHARS 32
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
      START_TIMER(led_show_seq);
      FastLED.show();
      END_FASTLED_TIMER(led_show_seq, perf.fastLEDShowTime);
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
      START_TIMER(led_show_fade);
      FastLED.show();
      END_FASTLED_TIMER(led_show_fade, perf.fastLEDShowTime);
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
  START_TIMER(led_show1);
  FastLED.show();
  END_FASTLED_TIMER(led_show1, perf.fastLEDShowTime);
  delay(30);
  set_led(x, y, CRGB::Black);
  START_TIMER(led_show2);
  FastLED.show();
  END_FASTLED_TIMER(led_show2, perf.fastLEDShowTime);
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
  START_TIMER(char_write);
  
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
  
  END_TIMER(char_write, perf.characterWriteTime);
}


void scroll_message_smooth(String s, int spos, bool scroll=true){
  START_TIMER(scroll_op);
  
  String substring = s.substring(spos, spos+21);
  int skip_from = 0;
  // Use smooth scrolling setting: 6 steps for smooth, 1 step for fast
  int smoothSteps = (scroll && SMOOTH_SCROLLING) ? 6 : 1;
  
  for (int step = 0; step < smoothSteps; step++){  
    // Calculate offset based on smooth steps (0 to -(smoothSteps-1))
    int offset = -step;
    
    //Serial.printf("offset: %d\n", offset);
    START_TIMER(calc);
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
    END_TIMER(calc, perf.calculationTime);
    
    if (!scroll){
      END_TIMER(scroll_op, perf.scrollTime);
      return;
    } else {
      START_TIMER(led_show);
      FastLED.show();
      END_FASTLED_TIMER(led_show, perf.fastLEDShowTime);
      // Removed timing delays - they were making performance worse
    }
  }
  
  END_TIMER(scroll_op, perf.scrollTime);
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
        START_TIMER(led_show_write);
        FastLED.show();
        END_FASTLED_TIMER(led_show_write, perf.fastLEDShowTime);
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
        START_TIMER(led_show_transition);
        FastLED.show();
        END_FASTLED_TIMER(led_show_transition, perf.fastLEDShowTime);
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
          START_TIMER(led_show_pause);
          FastLED.show();
          END_FASTLED_TIMER(led_show_pause, perf.fastLEDShowTime);
          delay(20);
        }
        // delay(1000);
      }
      scroll_message_smooth(stories[currentStoryIndex], spos, true);
      
      // Speed control and CPS tracking
      static unsigned long lastCharacterTime = 0;
      static int lastSpos = -1;
      
      bool shouldAdvance = true;
      if (!SMOOTH_SCROLLING) {
        // When smooth scrolling is off, control speed to hit CPS target
        unsigned long currentTime = millis();
        unsigned long targetDelay = 1000.0 / CPS_TARGET; // ms per character
        
        if (currentTime - lastCharacterTime < targetDelay) {
          shouldAdvance = false; // Skip advancing this frame
        } else {
          lastCharacterTime = currentTime;
        }
      }
      
      if (shouldAdvance && spos >= 0 && spos + 21 <= stories[currentStoryIndex].length()) {
        spos++;
        // Track character scrolling for CPS measurement
        if (spos != lastSpos) {
          perf.charactersScrolled++;
          lastSpos = spos;
        }
      } else if (shouldAdvance) {
        spos = 0;
        currentStoryIndex = (currentStoryIndex + 1) % stories.size();
        perf.charactersScrolled++; // Count story transitions
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

#if ENABLE_BENCHMARKING
void reportPerformance() {
  unsigned long currentTime = millis();
  if (currentTime - perf.lastReportTime >= 2000 && perf.frameCount > 0) { // Report every 2 seconds
    unsigned long reportInterval = currentTime - perf.lastReportTime;
    float avgFrameTime = (float)perf.totalFrameTime / perf.frameCount / 1000.0; // Convert to milliseconds
    float loopFPS = 1000.0 / avgFrameTime;
    float visualFPS = (float)perf.visualUpdateCount * 1000.0 / reportInterval; // Visual updates per second
    float avgFastLEDTime = (float)perf.fastLEDShowTime / perf.visualUpdateCount / 1000.0; // Per visual update
    float avgCharWriteTime = (float)perf.characterWriteTime / perf.frameCount / 1000.0;
    float avgScrollTime = (float)perf.scrollTime / perf.frameCount / 1000.0;
    float avgCalcTime = (float)perf.calculationTime / perf.frameCount / 1000.0;
    float actualCPS = (float)perf.charactersScrolled * 1000.0 / reportInterval; // Characters per second
    
    float cpuUsagePercent = ((avgFrameTime - (avgFastLEDTime * perf.visualUpdateCount / perf.frameCount)) / avgFrameTime) * 100.0;
    float hardwareWaitPercent = ((avgFastLEDTime * perf.visualUpdateCount / perf.frameCount) / avgFrameTime) * 100.0;
    
    Serial.println("=== PERFORMANCE REPORT ===");
    Serial.printf("Visual FPS: %.1f | Loop FPS: %.1f | Avg Loop: %.1fms\n", 
                  visualFPS, loopFPS, avgFrameTime);
    Serial.printf("Visual Updates/Loop: %.1f | FastLED.show(): %.2fms each\n", 
                  (float)perf.visualUpdateCount / perf.frameCount, avgFastLEDTime);
    Serial.printf("Character Write: %.2fms | Scroll: %.2fms | Calc: %.2fms\n", 
                  avgCharWriteTime, avgScrollTime, avgCalcTime);
    Serial.printf("Actual CPS: %.1f | Target: %.1f | Smooth: %s\n", 
                  actualCPS, CPS_TARGET, SMOOTH_SCROLLING ? "ON" : "OFF");
    Serial.printf("CPU Usage: %.1f%% | Hardware Wait: %.1f%%\n", cpuUsagePercent, hardwareWaitPercent);
    Serial.printf("Loops: %lu | Visual Updates: %lu | Characters: %lu | LEDs: %d\n", 
                  perf.frameCount, perf.visualUpdateCount, perf.charactersScrolled, NUM_LEDS);
    Serial.println("========================");
    
    // Reset metrics
    perf.totalFrameTime = 0;
    perf.fastLEDShowTime = 0;
    perf.characterWriteTime = 0;
    perf.scrollTime = 0;
    perf.calculationTime = 0;
    perf.frameCount = 0;
    perf.visualUpdateCount = 0;
    perf.charactersScrolled = 0;
    perf.maxFrameTime = 0;
    perf.minFrameTime = ULONG_MAX;
    perf.lastReportTime = currentTime;
  }
}
#endif

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

  // Initialize performance tracking (always initialize, even if benchmarking disabled)
  perf.lastReportTime = millis();

  delay(500);

  // Print ESP SDK version if available
  #if defined(ESP32)
    Serial.printf("ESP SDK version: %s\n", ESP.getSdkVersion());
  #endif

  // Parse FastLED version and print as X.Y.Z (e.g., 3.10.1 for 301001)
  int major = FASTLED_VERSION / 100000;
  int minor = (FASTLED_VERSION / 100) % 1000;
  int patch = FASTLED_VERSION % 100;
  Serial.printf("FastLED version: %d.%d.%d\n", major, minor, patch);

  // Other info
  Serial.printf("Number of LEDs: %d\n", NUM_LEDS);
  #if ENABLE_BENCHMARKING
    Serial.println("Performance benchmarking: ENABLED");
  #else
    Serial.println("Performance benchmarking: DISABLED");
  #endif
  Serial.println("Start");
}

#define NUM_MODES 3
unsigned long buttonPressTime = 0;
bool longPressActive = false;

void loop() {
  START_TIMER(frame);
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
      // Single blue flash instead of 5 separate flashes
      START_TIMER(led_show_long);
      FastLED.showColor(CRGB::Blue);
      END_FASTLED_TIMER(led_show_long, perf.fastLEDShowTime);
      delay(300); // Single longer flash
      FastLED.clear();
      FastLED.setBrightness(MAX_BRIGHTNESS); // Restore brightness
    }
  } else { // Button is released
    if (buttonPressTime > 0) { // Button was pressed
      if (!longPressActive) { // It was a short press
        if (mode == 0) { // Only change story if in story mode
          currentStoryIndex = (currentStoryIndex + 1) % stories.size(); // Use stories.size()
          // Reset story scroll position when changing story
          spos = 0;
          Serial.printf("Button tapped, changing story to %d\n", currentStoryIndex);
          // Visual feedback for short press - removed (too brief to see, wastes FastLED calls)
        } else {
          mode = 0; // If not in story mode, a short tap reverts to story mode
          Serial.println("Button tapped, reverting to story mode");
        }
        // Visual feedback for short press - removed (too brief to see, wastes FastLED calls)
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

  // Frame timing and performance reporting
  END_TIMER(frame, perf.totalFrameTime);
  #if ENABLE_BENCHMARKING
  unsigned long currentFrameTime = frame_start != 0 ? micros() - frame_start : 0;
  if (currentFrameTime > perf.maxFrameTime) perf.maxFrameTime = currentFrameTime;
  if (currentFrameTime < perf.minFrameTime) perf.minFrameTime = currentFrameTime;
  #endif
  perf.frameCount++;
  
  #if ENABLE_BENCHMARKING
    reportPerformance();
  #endif
}