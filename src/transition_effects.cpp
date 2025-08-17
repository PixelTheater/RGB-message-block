#include "transition_effects.h"
#include "performance_monitor.h"
#include "font.h"
#include <memory>

// External references from main.cpp
extern CRGB leds[];
extern PerformanceMonitor* g_perfMonitor;

// Forward declaration of utility functions that will be in main.cpp
void set_led(uint8_t x, uint8_t y, CRGB color);
void write_character(uint8_t character, uint8_t pos, CRGB color, int offset = 0);

//=============================================================================
// SmoothScrollTransition Implementation
//=============================================================================

SmoothScrollTransition::SmoothScrollTransition() 
  : TransitionEffect(true), scrollPosition(0), startPause(true), lastUpdateTime(0) {
}

void SmoothScrollTransition::reset() {
  scrollPosition = 0;
  startPause = true;
  lastUpdateTime = millis();
}

bool SmoothScrollTransition::update(ContentManager& content) {
  START_TIMER(scroll_op);
  
  if (startPause) {
    showStartPauseEffect();
    startPause = false;
  }
  
  // Handle newline transitions
  if (content.hasNewlineAt(scrollPosition)) {
    showNewlineTransition();
    scrollPosition = content.findNextPrintableChar(scrollPosition);
    startPause = true;
    FastLED.clear();
    END_TIMER(scroll_op, g_perfMonitor->getMetrics().scrollTime);
    return true;
  }
  
  // Render current scroll position with smooth steps
  renderScrollMessage(content, scrollPosition, true, 6);
  
  // Advance position
  if (scrollPosition >= 0 && scrollPosition + 21 <= content.getStoryLength()) {
    scrollPosition++;
    if (g_perfMonitor) g_perfMonitor->incrementCharactersScrolled();
    END_TIMER(scroll_op, g_perfMonitor->getMetrics().scrollTime);
    return true;
  } else {
    // End of story - trigger story change
    scrollPosition = 0;
    content.selectRandomStory();
    if (g_perfMonitor) g_perfMonitor->incrementCharactersScrolled();
    END_TIMER(scroll_op, g_perfMonitor->getMetrics().scrollTime);
    return true;
  }
}

void SmoothScrollTransition::renderScrollMessage(ContentManager& content, int spos, bool scroll, int smoothSteps) {
  // Simplified version of the original scroll_message_smooth function
  START_TIMER(calc);
  
  String story = content.getCurrentStory();
  
  for (int step = 0; step < smoothSteps; step++) {
    int offset = -step;
    
    FastLED.clear();
    for (int pos = 0; pos <= NUM_CHARS; pos++) {
      char thechar = ' ';
      if (pos + spos < story.length()) {
        thechar = story.c_str()[spos + pos];
      }
      
      switch (thechar) {
        case '\n':
          thechar = ' ';
          break;
      }
      
      // Use configurable coloring mode
      CRGB c = content.getCharacterColor(story, pos, spos);
      write_character(thechar, pos, c, offset);
    }
    
    if (!scroll) break;
    
    START_TIMER(led_show);
    FastLED.show();
    END_FASTLED_TIMER(led_show, g_perfMonitor->getMetrics().fastLEDShowTime);
  }
  
  END_TIMER(calc, g_perfMonitor->getMetrics().calculationTime);
}

void SmoothScrollTransition::showStartPauseEffect() {
  // Simplified version of the fade-in effect - just clear for now
  FastLED.clear();
  START_TIMER(led_show_pause);
  FastLED.show();
  END_FASTLED_TIMER(led_show_pause, g_perfMonitor->getMetrics().fastLEDShowTime);
}

void SmoothScrollTransition::showNewlineTransition() {
  // Simplified matrix transition effect
  for (int b = 1; b < 30; b++) {
    FastLED.clear();
    for (int x = 0; x < NUM_CHARS * 5; x++) {
      for (int y = 0; y < 7; y++) {
        set_led(x, y, CHSV(abs(sin(b / 10.0) * cos(x / 10.0)) * 255, 100 + random(b * 3, b * 4), 130 - b * 4 + random(20)));
      }
    }
    START_TIMER(led_show_transition);
    FastLED.show();
    END_FASTLED_TIMER(led_show_transition, g_perfMonitor->getMetrics().fastLEDShowTime);
    delay(20);
  }
}

//=============================================================================
// CharacterScrollTransition Implementation  
//=============================================================================

CharacterScrollTransition::CharacterScrollTransition()
  : TransitionEffect(true), scrollPosition(0), startPause(true), lastCharacterTime(0) {
}

void CharacterScrollTransition::reset() {
  scrollPosition = 0;
  startPause = true;
  lastCharacterTime = millis();
}

bool CharacterScrollTransition::update(ContentManager& content) {
  if (startPause) {
    showStartPauseEffect();
    startPause = false;
  }
  
  // Speed control for character mode
  unsigned long currentTime = millis();
  unsigned long targetDelay = 1000.0 / CPS_TARGET;
  
  if (currentTime - lastCharacterTime < targetDelay) {
    // Just maintain current display
    renderScrollMessage(content, scrollPosition);
    return false;
  }
  
  lastCharacterTime = currentTime;
  
  // Handle newlines similar to smooth scroll
  if (content.hasNewlineAt(scrollPosition)) {
    scrollPosition = content.findNextPrintableChar(scrollPosition);
    startPause = true;
    return true;
  }
  
  renderScrollMessage(content, scrollPosition);
  
  if (scrollPosition >= 0 && scrollPosition + 21 <= content.getStoryLength()) {
    scrollPosition++;
    if (g_perfMonitor) g_perfMonitor->incrementCharactersScrolled();
    return true;
  } else {
    scrollPosition = 0;
    content.selectRandomStory();
    if (g_perfMonitor) g_perfMonitor->incrementCharactersScrolled();
    return true;
  }
}

void CharacterScrollTransition::renderScrollMessage(ContentManager& content, int position) {
  // Fast single-step rendering
  String story = content.getCurrentStory();
  
  FastLED.clear();
  for (int pos = 0; pos <= NUM_CHARS; pos++) {
    char thechar = ' ';
    if (pos + position < story.length()) {
      thechar = story.c_str()[position + pos];
    }
    
    if (thechar == '\n') thechar = ' ';
    
    CRGB c = content.getCharacterColor(story, pos, position);
    write_character(thechar, pos, c, 0);
  }
  
  START_TIMER(led_show);
  FastLED.show();
  END_FASTLED_TIMER(led_show, g_perfMonitor->getMetrics().fastLEDShowTime);
}

void CharacterScrollTransition::showStartPauseEffect() {
  // Simplified version - just clear for now
  FastLED.clear();
  START_TIMER(led_show_pause);
  FastLED.show();
  END_FASTLED_TIMER(led_show_pause, g_perfMonitor->getMetrics().fastLEDShowTime);
}

//=============================================================================
// LineSlideTransition Implementation (Simplified)
//=============================================================================

LineSlideTransition::LineSlideTransition()
  : TransitionEffect(true), currentLineIndex(0), lastLineTime(0), previousLine("") {
}

void LineSlideTransition::reset() {
  currentLineIndex = 0;
  lastLineTime = millis();
  previousLine = ""; // Clear previous line on reset
}

bool LineSlideTransition::update(ContentManager& content) {
  auto lines = content.getCurrentLines();
  if (lines.empty()) {
    content.selectRandomStory();
    previousLine = ""; // Reset previous line
    return true;
  }
  
  unsigned long currentTime = millis();
  
  if (currentLineIndex < lines.size()) {
    String currentLine = lines[currentLineIndex];
    unsigned long lineDisplayTime = (currentLine.length() * 1000.0) / CPS_TARGET;
    
    if (currentTime - lastLineTime >= lineDisplayTime) {
      // Always show transition - for first line, slide from blank
      displayLineSlide(previousLine, currentLine, content);
      previousLine = currentLine;
      currentLineIndex++;
      lastLineTime = currentTime;
      if (g_perfMonitor) g_perfMonitor->incrementCharactersScrolled(currentLine.length());
      return true;
    } else {
      // Still displaying current line - show the correct one
      if (currentLineIndex > 0) {
        String displayLine = lines[currentLineIndex - 1]; // Show the line we're currently on
        maintainCurrentLine(displayLine, content);
      } else {
        // First line hasn't been revealed yet - keep display blank
        FastLED.clear();
        FastLED.show();
      }
      return false;
    }
  } else {
    // End of lines, reset
    currentLineIndex = 0;
    previousLine = ""; // Reset previous line
    content.selectRandomStory();
    return true;
  }
}

void LineSlideTransition::displayLineSlide(const String& prevLine, const String& newLine, ContentManager& content) {
  // Display line with true vertical slide transition (restored from original)
  int slideSteps = smoothTransitions ? 9 : 1; // Increased to accommodate 2-pixel gap
  
  for (int step = 0; step < slideSteps; step++) {
    FastLED.clear();
    
    // Calculate vertical positions with 2-pixel gap
    int prevY = -step; // Previous line moves up and out
    int newY = 9 - step; // New line moves up from bottom (7 + 2 pixel gap)
    
    // Draw previous line moving up
    for (int pos = 0; pos < NUM_CHARS && pos < prevLine.length(); pos++) {
      char prevChar = prevLine.c_str()[pos];
      CRGB prevColor = content.getCharacterColor(prevLine, pos, 0); // Use content manager coloring
      
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
      CRGB newColor = content.getCharacterColor(newLine, pos, 0); // Use content manager coloring
      
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
    END_FASTLED_TIMER(led_show_slide, g_perfMonitor->getMetrics().fastLEDShowTime);
    delay(40);
  }
}

void LineSlideTransition::maintainCurrentLine(const String& line, ContentManager& content) {
  FastLED.clear();
  for (int pos = 0; pos < NUM_CHARS && pos < line.length(); pos++) {
    char thechar = line.c_str()[pos];
    CRGB c = content.getCharacterColor(line, pos, 0); // Use content manager coloring
    write_character(thechar, pos, c);
  }
  START_TIMER(led_show_maintain);
  FastLED.show();
  END_FASTLED_TIMER(led_show_maintain, g_perfMonitor->getMetrics().fastLEDShowTime);
}

//=============================================================================
// CursorWipeTransition Implementation (Simplified)
//=============================================================================

CursorWipeTransition::CursorWipeTransition()
  : TransitionEffect(true), currentLineIndex(0), lastLineTime(0) {
}

void CursorWipeTransition::reset() {
  currentLineIndex = 0;
  lastLineTime = millis();
  wipeState = WIPE_IDLE;
  wipeStep = 0;
  flashStep = 0;
  lastStateTime = millis();
  currentWipeLine = "";
}

bool CursorWipeTransition::update(ContentManager& content) {
  auto lines = content.getCurrentLines();
  if (lines.empty()) {
    content.selectRandomStory();
    return true;
  }
  
  unsigned long currentTime = millis();
  
  // Start with blank display for first line
  if (currentLineIndex == 0 && wipeState == WIPE_IDLE && lines.size() > 0) {
    FastLED.clear();
    FastLED.show();
  }
  
  if (currentLineIndex < lines.size()) {
    String currentLine = lines[currentLineIndex];
    
    // Non-blocking cursor wipe animation
    if (wipeState == WIPE_IDLE) {
      // Start new line animation
      currentWipeLine = currentLine;
      currentWipeLine.trim();
      wipeStep = 0;
      flashStep = 0;
      wipeState = WIPE_REVEALING;
      lastStateTime = currentTime;
    }
    
    if (wipeState == WIPE_REVEALING) {
      // Wipe animation - reveal one character every 40ms
      if (currentTime - lastStateTime >= 40) {
        displayWipeStep(currentWipeLine, wipeStep, content);
        wipeStep++;
        lastStateTime = currentTime;
        
        if (wipeStep > currentWipeLine.length()) {
          wipeState = WIPE_FLASHING;
          flashStep = 0;
          lastStateTime = currentTime;
        }
      }
    }
    
    if (wipeState == WIPE_FLASHING) {
      // Flash cursor at end - every 200ms
      if (currentTime - lastStateTime >= 200) {
        displayFlashStep(currentWipeLine, flashStep % 2 == 0, content);
        flashStep++;
        lastStateTime = currentTime;
        
        if (flashStep >= 6) { // Flash 3 times (6 steps: on/off/on/off/on/off)
          // Move to next line
          unsigned long lineDisplayTime = (currentLine.length() * 1000.0) / CPS_TARGET + 2000;
          if (currentTime - lastLineTime >= lineDisplayTime) {
            currentLineIndex++;
            lastLineTime = currentTime;
            wipeState = WIPE_IDLE;
            if (g_perfMonitor) g_perfMonitor->incrementCharactersScrolled(currentLine.length());
          }
        }
      }
    }
    
    return false;
  } else {
    currentLineIndex = 0;
    wipeState = WIPE_IDLE;
    content.selectRandomStory();
    return true;
  }
}

void CursorWipeTransition::displayLineCursorWipe(const String& line, ContentManager& content) {
  // Display line with cursor wipe effect - stops at actual text end and flashes (restored from original)
  
  // Find actual end of text (excluding trailing spaces)
  String trimmedLine = line;
  trimmedLine.trim();
  int textLength = trimmedLine.length();
  
  int wipeSteps = smoothTransitions ? textLength : 1;
  
  // Wipe phase - reveal characters one by one
  for (int wipe = 0; wipe <= wipeSteps; wipe++) {
    FastLED.clear();
    
    for (int pos = 0; pos < NUM_CHARS; pos++) {
      CRGB c = CRGB::Black;
      char thechar = ' ';
      
      if (pos < textLength) {
        thechar = line.c_str()[pos];
        
        if (pos < wipe) {
          // Character revealed - use content manager coloring
          c = content.getCharacterColor(line, pos, 0);
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
    END_FASTLED_TIMER(led_show_wipe, g_perfMonitor->getMetrics().fastLEDShowTime);
    delay(40); // Faster character drawing (was 80ms, now 40ms)
  }
  
  // Pause phase - flash cursor at end of text
  if (smoothTransitions) {
    for (int flash = 0; flash < 3; flash++) { // Flash 3 times
      // Show text with cursor
      FastLED.clear();
      for (int pos = 0; pos < textLength; pos++) {
        char thechar = line.c_str()[pos];
        CRGB c = content.getCharacterColor(line, pos, 0);
        write_character(thechar, pos, c);
      }
      // Add flashing cursor at end
      if (textLength < NUM_CHARS) {
        write_character('_', textLength, CRGB::White);
      }
      
      START_TIMER(led_show_flash_on);
      FastLED.show();
      END_FASTLED_TIMER(led_show_flash_on, g_perfMonitor->getMetrics().fastLEDShowTime);
      delay(200);
      
      // Show text without cursor
      FastLED.clear();
      for (int pos = 0; pos < textLength; pos++) {
        char thechar = line.c_str()[pos];
        CRGB c = content.getCharacterColor(line, pos, 0);
        write_character(thechar, pos, c);
      }
      
      START_TIMER(led_show_flash_off);
      FastLED.show();
      END_FASTLED_TIMER(led_show_flash_off, g_perfMonitor->getMetrics().fastLEDShowTime);
      delay(200);
    }
  }
}

void CursorWipeTransition::maintainCurrentLine(const String& line, ContentManager& content) {
  // Just maintain the line display without animation
  FastLED.clear();
  for (int pos = 0; pos < NUM_CHARS && pos < line.length(); pos++) {
    char thechar = line.c_str()[pos];
    CRGB c = content.getCharacterColor(line, pos, 0); // Use content manager coloring
    write_character(thechar, pos, c);
  }
  START_TIMER(led_show_maintain);
  FastLED.show();
  END_FASTLED_TIMER(led_show_maintain, g_perfMonitor->getMetrics().fastLEDShowTime);
}

void CursorWipeTransition::displayWipeStep(const String& line, int step, ContentManager& content) {
  // Display one step of the wipe animation (non-blocking)
  FastLED.clear();
  
  int textLength = line.length();
  
  for (int pos = 0; pos < NUM_CHARS; pos++) {
    CRGB c = CRGB::Black;
    char thechar = ' ';
    
    if (pos < textLength) {
      thechar = line.c_str()[pos];
      
      if (pos < step) {
        // Character revealed - use content manager coloring
        c = content.getCharacterColor(line, pos, 0);
      } else if (pos == step) {
        // Cursor position during wipe
        c = CRGB::White;
        thechar = '_';
      }
    }
    
    write_character(thechar, pos, c);
  }
  
  START_TIMER(led_show_wipe);
  FastLED.show();
  END_FASTLED_TIMER(led_show_wipe, g_perfMonitor->getMetrics().fastLEDShowTime);
}

void CursorWipeTransition::displayFlashStep(const String& line, bool showCursor, ContentManager& content) {
  // Display one step of the flash animation (non-blocking)
  FastLED.clear();
  
  int textLength = line.length();
  
  // Show revealed text
  for (int pos = 0; pos < textLength && pos < NUM_CHARS; pos++) {
    char thechar = line.c_str()[pos];
    CRGB c = content.getCharacterColor(line, pos, 0);
    write_character(thechar, pos, c);
  }
  
  // Add flashing cursor at end if requested and space available
  if (showCursor && textLength < NUM_CHARS) {
    write_character('_', textLength, CRGB::White);
  }
  
  START_TIMER(led_show_flash);
  FastLED.show();
  END_FASTLED_TIMER(led_show_flash, g_perfMonitor->getMetrics().fastLEDShowTime);
}

//=============================================================================
// TransitionFactory Implementation
//=============================================================================

TransitionEffect* TransitionFactory::createTransition(TransitionType type, bool smoothTransitions) {
  switch (type) {
    case TransitionType::SMOOTH_SCROLL:
      return new SmoothScrollTransition();
    case TransitionType::CHARACTER_SCROLL:
      return new CharacterScrollTransition();
    case TransitionType::LINE_SLIDE:
      return new LineSlideTransition();
    case TransitionType::CURSOR_WIPE:
      return new CursorWipeTransition();
    default:
      return new SmoothScrollTransition();
  }
}

const char* TransitionFactory::getTransitionName(TransitionType type) {
  switch (type) {
    case TransitionType::SMOOTH_SCROLL: return "Smooth Scroll";
    case TransitionType::CHARACTER_SCROLL: return "Character Scroll";
    case TransitionType::LINE_SLIDE: return "Line Slide";
    case TransitionType::CURSOR_WIPE: return "Cursor Wipe";
    case TransitionType::FADE_IN_OUT: return "Fade In/Out";
    case TransitionType::RAINBOW_CYCLE: return "Rainbow Cycle";
    default: return "Unknown";
  }
} 