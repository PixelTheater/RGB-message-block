#pragma once
#include <Arduino.h>

// Performance benchmarking structures and utilities
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

class PerformanceMonitor {
public:
  PerformanceMonitor(bool enabled = true);
  
  void startTimer(unsigned long* startTime);
  void endTimer(unsigned long startTime, unsigned long& accumulator);
  void endFastLEDTimer(unsigned long startTime, unsigned long& accumulator);
  void reportPerformance();
  void incrementFrame();
  void incrementCharactersScrolled(int count = 1);
  
  PerformanceMetrics& getMetrics() { return metrics; }
  bool isEnabled() const { return enabled; }
  
private:
  PerformanceMetrics metrics;
  bool enabled;
};

// Macro definitions for easy timing
extern PerformanceMonitor* g_perfMonitor;

#define ENABLE_BENCHMARKING true  // Global setting

#if ENABLE_BENCHMARKING
  #define START_TIMER(var) unsigned long var##_start = 0; if(g_perfMonitor) g_perfMonitor->startTimer(&var##_start)
  #define END_TIMER(var, accumulator) if(g_perfMonitor) g_perfMonitor->endTimer(var##_start, accumulator)
  #define END_FASTLED_TIMER(var, accumulator) if(g_perfMonitor) g_perfMonitor->endFastLEDTimer(var##_start, accumulator)
#else
  #define START_TIMER(var) 
  #define END_TIMER(var, accumulator)
  #define END_FASTLED_TIMER(var, accumulator) if(g_perfMonitor) g_perfMonitor->getMetrics().visualUpdateCount++
#endif 