#include "performance_monitor.h"
#include "content_manager.h"  // For CPS_TARGET and LINE_TRANSITION_SMOOTH

// Global performance monitor instance
PerformanceMonitor* g_perfMonitor = nullptr;

PerformanceMonitor::PerformanceMonitor(bool enabled) : enabled(enabled) {
  metrics.lastReportTime = millis();
}

void PerformanceMonitor::startTimer(unsigned long* startTime) {
  if (enabled && startTime) {
    *startTime = micros();
  }
}

void PerformanceMonitor::endTimer(unsigned long startTime, unsigned long& accumulator) {
  if (enabled && startTime != 0) {
    accumulator += (micros() - startTime);
  }
}

void PerformanceMonitor::endFastLEDTimer(unsigned long startTime, unsigned long& accumulator) {
  if (enabled && startTime != 0) {
    accumulator += (micros() - startTime);
  }
  metrics.visualUpdateCount++;
}

void PerformanceMonitor::incrementFrame() {
  metrics.frameCount++;
}

void PerformanceMonitor::incrementCharactersScrolled(int count) {
  metrics.charactersScrolled += count;
}

void PerformanceMonitor::reportPerformance() {
  if (!enabled) return;
  
  unsigned long currentTime = millis();
  if (currentTime - metrics.lastReportTime >= 2000 && metrics.frameCount > 0) { // Report every 2 seconds
    unsigned long reportInterval = currentTime - metrics.lastReportTime;
    float avgFrameTime = (float)metrics.totalFrameTime / metrics.frameCount / 1000.0; // Convert to milliseconds
    float loopFPS = 1000.0 / avgFrameTime;
    float visualFPS = (float)metrics.visualUpdateCount * 1000.0 / reportInterval; // Visual updates per second
    float avgFastLEDTime = metrics.visualUpdateCount > 0 ? (float)metrics.fastLEDShowTime / metrics.visualUpdateCount / 1000.0 : 0; // Per visual update
    float avgCharWriteTime = (float)metrics.characterWriteTime / metrics.frameCount / 1000.0;
    float avgScrollTime = (float)metrics.scrollTime / metrics.frameCount / 1000.0;
    float avgCalcTime = (float)metrics.calculationTime / metrics.frameCount / 1000.0;
    float actualCPS = (float)metrics.charactersScrolled * 1000.0 / reportInterval; // Characters per second
    
    float cpuUsagePercent = metrics.visualUpdateCount > 0 ? 
      ((avgFrameTime - (avgFastLEDTime * metrics.visualUpdateCount / metrics.frameCount)) / avgFrameTime) * 100.0 : 0;
    float hardwareWaitPercent = metrics.visualUpdateCount > 0 ?
      ((avgFastLEDTime * metrics.visualUpdateCount / metrics.frameCount) / avgFrameTime) * 100.0 : 0;
    
    Serial.println("=== PERFORMANCE REPORT ===");
    Serial.printf("Visual FPS: %.1f | Loop FPS: %.1f | Avg Loop: %.1fms\n", 
                  visualFPS, loopFPS, avgFrameTime);
    Serial.printf("Visual Updates/Loop: %.1f | FastLED.show(): %.2fms each\n", 
                  metrics.frameCount > 0 ? (float)metrics.visualUpdateCount / metrics.frameCount : 0, avgFastLEDTime);
    Serial.printf("Character Write: %.2fms | Scroll: %.2fms | Calc: %.2fms\n", 
                  avgCharWriteTime, avgScrollTime, avgCalcTime);
    Serial.printf("Actual CPS: %.1f | Target: %.1f | Transitions: %s\n", 
                  actualCPS, CPS_TARGET, LINE_TRANSITION_SMOOTH ? "Smooth" : "Fast");
    Serial.printf("CPU Usage: %.1f%% | Hardware Wait: %.1f%%\n", cpuUsagePercent, hardwareWaitPercent);
    Serial.printf("Loops: %lu | Visual Updates: %lu | Characters: %lu\n", 
                  metrics.frameCount, metrics.visualUpdateCount, metrics.charactersScrolled);
    Serial.println("========================");
    
    // Reset metrics
    metrics.totalFrameTime = 0;
    metrics.fastLEDShowTime = 0;
    metrics.characterWriteTime = 0;
    metrics.scrollTime = 0;
    metrics.calculationTime = 0;
    metrics.frameCount = 0;
    metrics.visualUpdateCount = 0;
    metrics.charactersScrolled = 0;
    metrics.maxFrameTime = 0;
    metrics.minFrameTime = ULONG_MAX;
    metrics.lastReportTime = currentTime;
  }
} 