#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include <vector>

// Color mode enumeration
enum class ColorMode {
  WORD_BASED = 0,     // Each word gets a different color based on position
  RAINBOW_SCROLL = 1, // Rainbow colors across character positions
  RANDOM_WORDS = 2,   // Random color per word
  SINGLE_COLOR = 3    // Single color with brightness variation
};

// Content management for stories and text display
class ContentManager {
public:
  ContentManager();
  
  // Story management
  void addStory(const String& story);
  void selectRandomStory();
  void selectStory(int index);
  String getCurrentStory() const;
  int getCurrentStoryIndex() const { return currentStoryIndex; }
  int getStoryCount() const { return stories.size(); }
  
  // Text processing for scroll modes
  char getCharacterAt(int position) const;
  bool isAtStoryEnd(int position) const;
  int getStoryLength() const;
  
  // Line-based processing for line modes
  std::vector<String> extractLines(const String& story) const;
  std::vector<String> getCurrentLines();
  void refreshCurrentLines();
  
  // Color management
  void setColorMode(ColorMode mode) { currentColorMode = mode; }
  ColorMode getColorMode() const { return currentColorMode; }
  void randomizeColorMode();
  const char* getColorModeName() const;
  
  // Color generation based on current mode
  CRGB getCharacterColor(const String& text, int position, int scrollPosition = 0) const;
  CRGB getWordColor(const String& text, int position) const; // Legacy method
  
  // Navigation
  void reset();
  bool hasNewlineAt(int position) const;
  int findNextPrintableChar(int startPos) const;
  
private:
  std::vector<String> stories;
  int currentStoryIndex;
  std::vector<String> currentLines;
  bool linesNeedRefresh;
  ColorMode currentColorMode;
  
  // Helper functions
  String smartWordWrap(const String& line, int maxWidth) const;
  String padToWidth(const String& line, int width) const;
};

// Constants for display parameters
#define NUM_CHARS 32
#define CPS_TARGET 15.0
#define LINE_TRANSITION_SMOOTH true 