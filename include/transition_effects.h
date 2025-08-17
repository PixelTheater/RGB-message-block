#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include <vector>
#include "content_manager.h"

// Forward declarations
extern CRGB leds[];
extern void set_led(uint8_t x, uint8_t y, CRGB color);
extern void write_character(uint8_t character, uint8_t pos, CRGB color, int offset);

// Transition effect types
enum class TransitionType {
  SMOOTH_SCROLL = 0,
  CHARACTER_SCROLL = 1,  
  LINE_SLIDE = 2,
  CURSOR_WIPE = 3,
  FADE_IN_OUT = 4,
  RAINBOW_CYCLE = 5
};

// Base class for all transition effects
class TransitionEffect {
public:
  TransitionEffect(bool smoothTransitions = true) : smoothTransitions(smoothTransitions) {}
  virtual ~TransitionEffect() = default;
  
  virtual void reset() = 0;
  virtual bool update(ContentManager& content) = 0; // Returns true if content advanced
  virtual TransitionType getType() const = 0;
  
  void setSmoothTransitions(bool smooth) { smoothTransitions = smooth; }
  bool getSmoothTransitions() const { return smoothTransitions; }
  
protected:
  bool smoothTransitions;
};

// Smooth scroll transition (6-step character transitions)  
class SmoothScrollTransition : public TransitionEffect {
public:
  SmoothScrollTransition();
  void reset() override;
  bool update(ContentManager& content) override;
  TransitionType getType() const override { return TransitionType::SMOOTH_SCROLL; }
  
private:
  int scrollPosition;
  bool startPause;
  unsigned long lastUpdateTime;
  
  void renderScrollMessage(ContentManager& content, int position, bool scroll, int smoothSteps);
  void showStartPauseEffect();
  void showNewlineTransition();
};

// Character scroll transition (fast 1-step transitions)
class CharacterScrollTransition : public TransitionEffect {
public:
  CharacterScrollTransition();
  void reset() override;
  bool update(ContentManager& content) override;
  TransitionType getType() const override { return TransitionType::CHARACTER_SCROLL; }
  
private:
  int scrollPosition;
  bool startPause;
  unsigned long lastCharacterTime;
  
  void renderScrollMessage(ContentManager& content, int position);
  void showStartPauseEffect();
};

// Line slide transition (vertical sliding animation)
class LineSlideTransition : public TransitionEffect {
public:
  LineSlideTransition();
  void reset() override;
  bool update(ContentManager& content) override;
  TransitionType getType() const override { return TransitionType::LINE_SLIDE; }
  
private:
  int currentLineIndex;
  unsigned long lastLineTime;
  String previousLine;
  
  void displayLineSlide(const String& prevLine, const String& newLine, ContentManager& content);
  void maintainCurrentLine(const String& line, ContentManager& content);
};

// Cursor wipe transition (typing effect with cursor)
class CursorWipeTransition : public TransitionEffect {
public:
  CursorWipeTransition();
  void reset() override;
  bool update(ContentManager& content) override;
  TransitionType getType() const override { return TransitionType::CURSOR_WIPE; }
  
private:
  int currentLineIndex;
  unsigned long lastLineTime;
  
  // Non-blocking animation state
  enum WipeState { WIPE_IDLE, WIPE_REVEALING, WIPE_FLASHING };
  WipeState wipeState = WIPE_IDLE;
  int wipeStep = 0;
  int flashStep = 0;
  unsigned long lastStateTime = 0;
  String currentWipeLine = "";
  
  void displayLineCursorWipe(const String& line, ContentManager& content);
  void maintainCurrentLine(const String& line, ContentManager& content);
  void displayWipeStep(const String& line, int step, ContentManager& content);
  void displayFlashStep(const String& line, bool showCursor, ContentManager& content);
};

// Factory class for creating transitions
class TransitionFactory {
public:
  static TransitionEffect* createTransition(TransitionType type, bool smoothTransitions = true);
  static const char* getTransitionName(TransitionType type);
  static int getTransitionCount() { return 6; } // Update as we add more transitions
};

// Configuration constants
#define LINE_TRANSITION_SMOOTH true
#define NUM_LEDS (5*7*NUM_CHARS) 