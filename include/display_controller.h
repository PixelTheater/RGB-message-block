#pragma once
#include <Arduino.h>
#include <memory>
#include "content_manager.h"
#include "transition_effects.h"
#include "space_animation.h"

// Display modes
enum class DisplayMode {
  TEXT_CONTENT = 0,    // Show text content with transitions
  SPACE_ANIMATION = 1, // Show space animation
  COLOR_SHOW = 2,      // Original color show mode
  TEST_PATTERNS = 3    // Original test patterns
};

// Display controller manages content, transitions, and modes
class DisplayController {
public:
  DisplayController();
  ~DisplayController();
  
  // Initialization
  void initialize();
  
  // Main update loop
  void update();
  
  // Mode management  
  void setDisplayMode(DisplayMode mode);
  DisplayMode getDisplayMode() const { return currentMode; }
  int getDisplayModeCount() const { return 4; }
  const char* getDisplayModeName() const;
  
  // Transition management (for text content mode)
  void setTransitionType(TransitionType type);
  TransitionType getTransitionType() const;
  const char* getTransitionName() const;
  int getTransitionCount() const;
  void cycleThroughTransitions();
  
  // Content management
  ContentManager& getContentManager() { return contentManager; }
  
  // Space animation access
  SpaceAnimation& getSpaceAnimation() { return spaceAnimation; }
  
  // Configuration
  void setSmoothTransitions(bool smooth);
  bool getSmoothTransitions() const { return smoothTransitions; }
  
  // Button handling
  void handleShortPress();
  void handleLongPress();
  
  // Reset current mode
  void reset();
  
private:
  // Core components
  ContentManager contentManager;
  std::unique_ptr<TransitionEffect> currentTransition;
  SpaceAnimation spaceAnimation;
  
  // State
  DisplayMode currentMode;
  TransitionType currentTransitionType;
  bool smoothTransitions;
  
  // Transition cycling
  unsigned long lastTransitionChange;
  unsigned long transitionChangeInterval; // Auto-cycle transitions every N seconds
  bool autoTransitionCycling;
  
  // Mode-specific update functions
  void updateTextContent();
  void updateSpaceAnimation();
  void updateColorShow();
  void updateTestPatterns();
  
  // Original functions (from main.cpp)
  void color_show();
  void test_patterns();
  
  // Helper functions
  void createTransition(TransitionType type);
  const char* getDisplayModeNameFor(DisplayMode mode) const;
};

// Configuration constants
#define NUM_DISPLAY_MODES 4
#define AUTO_TRANSITION_CYCLE_INTERVAL 15000 // milliseconds (15 seconds)
#define ENABLE_AUTO_TRANSITION_CYCLING false 