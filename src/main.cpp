#include <Arduino.h>
#include <FastLED.h>
#include "font.h"
#include "led_art.h"
#include "led_history.h"
#include <vector>

// ===================== CONFIGURATION =====================
#define ENABLE_BENCHMARKING true  // Set to false to disable performance monitoring
                                  // If you get crashes, try setting this to false first

// DISPLAY MODE SETTINGS  
#define NUM_DISPLAY_MODES 4       // Total number of display modes (reduced from 5)
#define CPS_TARGET 15.0           // Characters per second for line reading speed calculation
#define LINE_TRANSITION_SMOOTH true // Enable smooth transitions between lines (vs instant)

// DISPLAY MODES (Button press cycles through all modes, loops back to 0):
// Mode 0: Smooth scroll (6-step character transitions)
// Mode 1: Character scroll (fast 1-step transitions) 
// Mode 2: Line-by-line with vertical slide transitions (old line slides up, new slides in from bottom)
// Mode 3: Line-by-line with cursor wipe transitions

// IMPROVEMENTS:
// - Button cycling now properly loops back to mode 0
// - Line slide mode now has true vertical sliding animation with 2-pixel spacing
// - Smart word wrapping prevents breaking words across lines
// - Removed redundant line fade mode
// - Cursor mode stops at text end and flashes (no typing spaces)
// - All line modes use word-based coloring (consistent with scroll modes)

// MODE COMPARISON:
// Scroll modes: Continuous character-by-character movement
// Line modes: Show complete lines with transitions, calculated timing for readability
// =========================================================


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


void scroll_message_smooth(String s, int spos, bool scroll=true, int smoothSteps=1){
  START_TIMER(scroll_op);
  
  String substring = s.substring(spos, spos+21);
  int skip_from = 0;
  // Use passed smoothness parameter
  if (!scroll) smoothSteps = 1; // Force single step if not scrolling
  
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

// Helper function for word-based coloring (consistent with scroll modes)
CRGB getWordColor(String text, int pos) {
  int prev_space = 0;
  // search backwards from current pos in string for a space
  for (int i = pos; i >= 0; i--){
    if (text.c_str()[i] == ' '){
      prev_space = i;
      break;
    }
  }
  // set color based on prev_space (same logic as scroll modes)
  return CHSV((prev_space % 25)*(10), 255, 180);
}

// Line-by-line display functions
void display_line_slide(String prevLine, String newLine, bool slideUp=true) {
  // Display line with true vertical slide transition
  int slideSteps = LINE_TRANSITION_SMOOTH ? 9 : 1; // Increased to accommodate 2-pixel gap
  
  for (int step = 0; step < slideSteps; step++) {
    FastLED.clear();
    
    // Calculate vertical positions with 2-pixel gap
    int prevY = -step; // Previous line moves up and out
    int newY = 9 - step; // New line moves up from bottom (7 + 2 pixel gap)
    
    // Draw previous line moving up
    for (int pos = 0; pos < NUM_CHARS && pos < prevLine.length(); pos++) {
      char prevChar = prevLine.c_str()[pos];
      CRGB prevColor = getWordColor(prevLine, pos); // Use word-based coloring
      
      // Draw character at shifted vertical position
      for (int py = 0; py < 7; py++) {
        int actualY = py + prevY;
        if (actualY >= 0 && actualY < 7) { // Only draw if within bounds
          for (int px = 0; px < 5; px++) {
            if (FONT_BIT(prevChar - 16, px, py)) {
              set_led(pos*5 + px, actualY, prevColor);
            }
          }
        }
      }
    }
    
    // Draw new line moving up from bottom
    for (int pos = 0; pos < NUM_CHARS && pos < newLine.length(); pos++) {
      char newChar = newLine.c_str()[pos];
      CRGB newColor = getWordColor(newLine, pos); // Use word-based coloring
      
      // Draw character at shifted vertical position
      for (int py = 0; py < 7; py++) {
        int actualY = py + newY;
        if (actualY >= 0 && actualY < 7) { // Only draw if within bounds
          for (int px = 0; px < 5; px++) {
            if (FONT_BIT(newChar - 16, px, py)) {
              set_led(pos*5 + px, actualY, newColor);
            }
          }
        }
      }
    }
    
    START_TIMER(led_show_slide);
    FastLED.show();
    END_FASTLED_TIMER(led_show_slide, perf.fastLEDShowTime);
    delay(40);
  }
}

void display_line_cursor_wipe(String line) {
  // Display line with cursor wipe effect - stops at actual text end and flashes
  
  // Find actual end of text (excluding trailing spaces)
  String trimmedLine = line;
  trimmedLine.trim();
  int textLength = trimmedLine.length();
  
  int wipeSteps = LINE_TRANSITION_SMOOTH ? textLength : 1;
  
  // Wipe phase - reveal characters one by one
  for (int wipe = 0; wipe <= wipeSteps; wipe++) {
    FastLED.clear();
    
    for (int pos = 0; pos < NUM_CHARS; pos++) {
      CRGB c = CRGB::Black;
      char thechar = ' ';
      
      if (pos < textLength) {
        thechar = line.c_str()[pos];
        
        if (pos < wipe) {
          // Character revealed - use word-based coloring
          c = getWordColor(line, pos);
        } else if (pos == wipe) {
          // Cursor position during wipe
          c = CRGB::White;
          thechar = '_';
        }
      }
      
      write_character(thechar, pos, c);
    }
    
    START_TIMER(led_show_wipe);
    FastLED.show();
    END_FASTLED_TIMER(led_show_wipe, perf.fastLEDShowTime);
    delay(80);
  }
  
  // Pause phase - flash cursor at end of text
  if (LINE_TRANSITION_SMOOTH) {
    for (int flash = 0; flash < 3; flash++) { // Flash 3 times
      // Show text with cursor
      FastLED.clear();
      for (int pos = 0; pos < textLength; pos++) {
        char thechar = line.c_str()[pos];
        CRGB c = getWordColor(line, pos);
        write_character(thechar, pos, c);
      }
      // Add flashing cursor at end
      if (textLength < NUM_CHARS) {
        write_character('_', textLength, CRGB::White);
      }
      
      START_TIMER(led_show_flash_on);
      FastLED.show();
      END_FASTLED_TIMER(led_show_flash_on, perf.fastLEDShowTime);
      delay(200);
      
      // Show text without cursor
      FastLED.clear();
      for (int pos = 0; pos < textLength; pos++) {
        char thechar = line.c_str()[pos];
        CRGB c = getWordColor(line, pos);
        write_character(thechar, pos, c);
      }
      
      START_TIMER(led_show_flash_off);
      FastLED.show();
      END_FASTLED_TIMER(led_show_flash_off, perf.fastLEDShowTime);
      delay(200);
    }
  }
}

// Extract lines from story text with smart word wrapping
std::vector<String> extractLines(String story) {
  std::vector<String> lines;
  int start = 0;
  
  while (start < story.length()) {
    int end = story.indexOf('\n', start);
    if (end == -1) end = story.length();
    
    String line = story.substring(start, end);
    line.trim();
    
    if (line.length() > 0) {
      // Handle lines that are too long with smart word wrapping
      while (line.length() > NUM_CHARS) {
        String currentLine;
        int lastSpace = -1;
        
        // Find the best break point (last space within display width)
        for (int i = 0; i < NUM_CHARS && i < line.length(); i++) {
          if (line.c_str()[i] == ' ') {
            lastSpace = i;
          }
        }
        
        if (lastSpace > 0 && lastSpace < NUM_CHARS) {
          // Break at the last space within display width
          currentLine = line.substring(0, lastSpace);
          // Pad with spaces to full width for clean display
          while (currentLine.length() < NUM_CHARS) {
            currentLine += " ";
          }
          lines.push_back(currentLine);
          line = line.substring(lastSpace + 1); // Skip the space
        } else {
          // No good break point found, force break (rare case)
          currentLine = line.substring(0, NUM_CHARS);
          lines.push_back(currentLine);
          line = line.substring(NUM_CHARS);
        }
      }
      
      if (line.length() > 0) {
        // Pad short lines with spaces for consistent display
        while (line.length() < NUM_CHARS) {
          line += " ";
        }
        lines.push_back(line);
      }
    }
    
    start = end + 1;
  }
  
  return lines;
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


void display_story(int mode){
  static bool start_pause = true;
  static unsigned long lastCharacterTime = 0;
  static unsigned long lastLineTime = 0;
  static int lastSpos = -1;
  static int currentLineIndex = 0;
  static std::vector<String> currentLines;
  static String previousLine = "";
  
  // Randomly select story on first run or story end
  if (currentLines.empty() || (mode >= 2 && currentLineIndex >= currentLines.size())) {
    currentStoryIndex = random(stories.size());
    currentLines = extractLines(stories[currentStoryIndex]);
    currentLineIndex = 0;
    spos = 0;
    Serial.printf("New story selected: %d (%d lines)\n", currentStoryIndex, currentLines.size());
  }

  // Handle newline transitions for scroll modes
  if (mode <= 1 && stories[currentStoryIndex].c_str()[spos] == '\n') {
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
  }

  switch (mode) {
    case 0: // Smooth scroll (6-step transitions)
      if (start_pause) {
        start_pause = false;
        for (int b=30; b>0; b--){
          scroll_message_smooth(stories[currentStoryIndex], spos, false, 6);
          for (int i=0; i<NUM_LEDS; i++){
            leds[i].fadeToBlackBy(random(b*6,b*10));
          }
          START_TIMER(led_show_pause);
          FastLED.show();
          END_FASTLED_TIMER(led_show_pause, perf.fastLEDShowTime);
          delay(20);
        }
      }
      scroll_message_smooth(stories[currentStoryIndex], spos, true, 6);
      if (spos >= 0 && spos + 21 <= stories[currentStoryIndex].length()) {
        spos++;
        if (spos != lastSpos) {
          perf.charactersScrolled++;
          lastSpos = spos;
        }
      } else {
        spos = 0;
        currentLines.clear(); // Force story change
        perf.charactersScrolled++;
      }
      break;
      
    case 1: // Character scroll (fast 1-step transitions)
      {
        if (start_pause) {
          start_pause = false;
          for (int b=30; b>0; b--){
            scroll_message_smooth(stories[currentStoryIndex], spos, false, 1);
            for (int i=0; i<NUM_LEDS; i++){
              leds[i].fadeToBlackBy(random(b*6,b*10));
            }
            START_TIMER(led_show_pause);
            FastLED.show();
            END_FASTLED_TIMER(led_show_pause, perf.fastLEDShowTime);
            delay(20);
          }
        }
        
        // Speed control for character mode
        bool shouldAdvance = true;
        unsigned long currentTime = millis();
        unsigned long targetDelay = 1000.0 / CPS_TARGET;
        
        if (currentTime - lastCharacterTime < targetDelay) {
          shouldAdvance = false;
        } else {
          lastCharacterTime = currentTime;
        }
        
        scroll_message_smooth(stories[currentStoryIndex], spos, true, 1);
        if (shouldAdvance && spos >= 0 && spos + 21 <= stories[currentStoryIndex].length()) {
          spos++;
          if (spos != lastSpos) {
            perf.charactersScrolled++;
            lastSpos = spos;
          }
        } else if (shouldAdvance) {
          spos = 0;
          currentLines.clear(); // Force story change
          perf.charactersScrolled++;
        }
      }
      break;
      
    case 2: // Line-by-line with vertical slide transitions
    case 3: // Line-by-line with cursor wipe transitions
      {
        unsigned long currentTime = millis();
        // Calculate line display time based on CPS and line length
        if (currentLineIndex < currentLines.size()) {
          String currentLine = currentLines[currentLineIndex];
          unsigned long lineDisplayTime = (currentLine.length() * 1000.0) / CPS_TARGET;
          
          if (currentTime - lastLineTime >= lineDisplayTime) {
            // Time to show next line
            if (currentLineIndex < currentLines.size()) {
              String lineToShow = currentLines[currentLineIndex];
              
              switch (mode) {
                case 2: // Slide transition
                  display_line_slide(previousLine, lineToShow, true);
                  break;
                case 3: // Cursor wipe transition
                  display_line_cursor_wipe(lineToShow);
                  break;
              }
              
              previousLine = lineToShow;
              currentLineIndex++;
              lastLineTime = currentTime;
              perf.charactersScrolled += lineToShow.length();
            }
          } else {
            // Still displaying current line - just maintain display
            if (currentLineIndex > 0 && currentLineIndex <= currentLines.size()) {
              String currentLine = currentLines[currentLineIndex - 1];
              FastLED.clear();
              for (int pos = 0; pos < NUM_CHARS && pos < currentLine.length(); pos++) {
                char thechar = currentLine.c_str()[pos];
                CRGB c = getWordColor(currentLine, pos); // Use word-based coloring
                write_character(thechar, pos, c);
              }
              START_TIMER(led_show_maintain);
              FastLED.show();
              END_FASTLED_TIMER(led_show_maintain, perf.fastLEDShowTime);
            }
          }
        }
      }
      break;
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
    Serial.printf("Actual CPS: %.1f | Target: %.1f | Transitions: %s\n", 
                  actualCPS, CPS_TARGET, LINE_TRANSITION_SMOOTH ? "Smooth" : "Fast");
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

  // Initialize random seed for story selection
  randomSeed(analogRead(A0) + millis());

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

#define NUM_MODES 4
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
      const char* modeNames[] = {"Smooth Scroll", "Character Scroll", "Line Slide", "Line Cursor"};
      Serial.printf("Button held, changing to mode %d: %s\n", mode, modeNames[mode]);
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
        // Short press also cycles modes
        mode = (mode + 1) % NUM_MODES;
        const char* modeNames[] = {"Smooth Scroll", "Character Scroll", "Line Slide", "Line Cursor"};
        Serial.printf("Button tapped, changing to mode %d: %s\n", mode, modeNames[mode]);
        // Visual feedback for short press - removed (too brief to see, wastes FastLED calls)
      }
      buttonPressTime = 0; // Reset button press time
      longPressActive = false; // Reset long press flag
    }
  }

  switch(mode){
    case 0: // Smooth scroll
    case 1: // Character scroll  
    case 2: // Line slide
    case 3: // Line cursor wipe
      display_story(mode);
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