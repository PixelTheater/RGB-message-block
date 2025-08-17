#include "content_manager.h"
#include <FastLED.h>

ContentManager::ContentManager() 
  : currentStoryIndex(0), linesNeedRefresh(true), currentColorMode(ColorMode::WORD_BASED) {
}

void ContentManager::addStory(const String& story) {
  stories.push_back(story);
  linesNeedRefresh = true;
}

void ContentManager::selectRandomStory() {
  if (!stories.empty()) {
    currentStoryIndex = random(stories.size());
    linesNeedRefresh = true;
  }
}

void ContentManager::selectStory(int index) {
  if (index >= 0 && index < stories.size()) {
    currentStoryIndex = index;
    linesNeedRefresh = true;
  }
}

String ContentManager::getCurrentStory() const {
  if (currentStoryIndex >= 0 && currentStoryIndex < stories.size()) {
    return stories[currentStoryIndex];
  }
  return "";
}

char ContentManager::getCharacterAt(int position) const {
  String story = getCurrentStory();
  if (position >= 0 && position < story.length()) {
    return story.c_str()[position];
  }
  return ' ';
}

bool ContentManager::isAtStoryEnd(int position) const {
  String story = getCurrentStory();
  return position >= story.length();
}

int ContentManager::getStoryLength() const {
  return getCurrentStory().length();
}

std::vector<String> ContentManager::extractLines(const String& story) const {
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

std::vector<String> ContentManager::getCurrentLines() {
  if (linesNeedRefresh) {
    refreshCurrentLines();
  }
  return currentLines;
}

void ContentManager::refreshCurrentLines() {
  currentLines = extractLines(getCurrentStory());
  linesNeedRefresh = false;
}

CRGB ContentManager::getWordColor(const String& text, int position) const {
  int prev_space = 0;
  // search backwards from current position in string for a space
  for (int i = position; i >= 0; i--){
    if (text.c_str()[i] == ' '){
      prev_space = i;
      break;
    }
  }
  // set color based on prev_space (same logic as original scroll modes)
  CRGB color;
  color = CHSV((prev_space % 25)*(10), 255, 180);
  return color;
}

void ContentManager::reset() {
  linesNeedRefresh = true;
}

bool ContentManager::hasNewlineAt(int position) const {
  return getCharacterAt(position) == '\n';
}

int ContentManager::findNextPrintableChar(int startPos) const {
  String story = getCurrentStory();
  bool printable = false;
  int pos = startPos;
  
  while (!printable && pos < story.length()){
    char c = story.c_str()[pos + 1];
    if (c == '\n' || c == ' '){
      printable = false;
    } else {
      printable = true;
    }
    pos++;
  }
  
  return pos;
}

String ContentManager::smartWordWrap(const String& line, int maxWidth) const {
  // Helper function for word wrapping (already implemented in extractLines)
  return line.substring(0, maxWidth);
}

String ContentManager::padToWidth(const String& line, int width) const {
  String padded = line;
  while (padded.length() < width) {
    padded += " ";
  }
  return padded;
}

void ContentManager::randomizeColorMode() {
  int modeCount = 4; // Number of ColorMode enum values
  currentColorMode = static_cast<ColorMode>(random(modeCount));
  Serial.printf("Color mode changed to: %s\n", getColorModeName());
}

const char* ContentManager::getColorModeName() const {
  switch (currentColorMode) {
    case ColorMode::WORD_BASED: return "Word-Based";
    case ColorMode::RAINBOW_SCROLL: return "Rainbow Scroll";
    case ColorMode::RANDOM_WORDS: return "Random Words";
    case ColorMode::SINGLE_COLOR: return "Single Color";
    default: return "Unknown";
  }
}

CRGB ContentManager::getCharacterColor(const String& text, int position, int scrollPosition) const {
  switch (currentColorMode) {
    case ColorMode::WORD_BASED:
      return getWordColor(text, position + scrollPosition);
      
    case ColorMode::RAINBOW_SCROLL:
      return CHSV((position * 10) % 255, 255, 180);
      
    case ColorMode::RANDOM_WORDS: {
      // Find the start of the current word
      int wordStart = position + scrollPosition;
      while (wordStart > 0 && text.c_str()[wordStart - 1] != ' ') {
        wordStart--;
      }
      // Use word start position as seed for consistent color per word
      return CHSV((wordStart * 73) % 255, 255, 180);
    }
    
    case ColorMode::SINGLE_COLOR: {
      // Single hue with brightness variation based on position
      uint8_t baseHue = (millis() / 1000) % 255; // Slowly changing base hue
      uint8_t brightness = 120 + (position * 20) % 135; // Brightness variation
      return CHSV(baseHue, 255, brightness);
    }
    
    default:
      return getWordColor(text, position + scrollPosition);
  }
} 