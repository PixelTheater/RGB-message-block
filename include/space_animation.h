#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include <vector>

// Forward declarations
extern CRGB leds[];
extern void set_led(uint8_t x, uint8_t y, CRGB color);

// Space object types
struct Star {
  float x, y;
  float speed;
  uint8_t brightness;
  CRGB color;
  bool active;
};

struct Comet {
  float x, y;
  float speedX, speedY;
  uint8_t trailLength;
  CRGB color;
  bool active;
  unsigned long lastUpdate;
};

struct Planet {
  float x, y;
  float speed;
  uint8_t size;
  CRGB color;
  bool active;
  float phase; // For rotation/animation
};

struct Spaceship {
  float x, y;
  float speedX, speedY;
  uint8_t frame; // For sprite animation
  CRGB color;
  bool active;
  unsigned long lastFrameUpdate;
};

// Main space animation class
class SpaceAnimation {
public:
  SpaceAnimation();
  
  // Main update and render
  void update();
  void render();
  void reset();
  
  // Configuration
  void setStarCount(int count) { maxStars = count; }
  void setCometCount(int count) { maxComets = count; }
  void setPlanetCount(int count) { maxPlanets = count; }
  void setSpaceshipCount(int count) { maxSpaceships = count; }
  
  void setParallaxSpeed(float speed) { parallaxSpeed = speed; }
  void setStarSpeedRange(float min, float max) { starSpeedMin = min; starSpeedMax = max; }
  
  // Animation control
  void pause() { paused = true; }
  void resume() { paused = false; }
  bool isPaused() const { return paused; }
  
private:
  // Object arrays
  std::vector<Star> stars;
  std::vector<Comet> comets;
  std::vector<Planet> planets;
  std::vector<Spaceship> spaceships;
  
  // Configuration
  int maxStars;
  int maxComets;
  int maxPlanets;
  int maxSpaceships;
  
  float parallaxSpeed;
  float starSpeedMin, starSpeedMax;
  
  // Animation state
  bool paused;
  unsigned long lastUpdate;
  unsigned long cometSpawnTimer;
  unsigned long planetSpawnTimer;
  unsigned long spaceshipSpawnTimer;
  
  // Background effects
  uint8_t nebularPhase;
  unsigned long nebulaTimer;
  
  // Display dimensions
  static const int DISPLAY_WIDTH = 160; // 32 chars * 5 pixels
  static const int DISPLAY_HEIGHT = 7;
  
  // Object management
  void initializeStars();
  void updateStars();
  void updateComets();
  void updatePlanets();
  void updateSpaceships();
  void updateNebula();
  
  void spawnComet();
  void spawnPlanet();
  void spawnSpaceship();
  
  // Rendering
  void renderStars();
  void renderComets();
  void renderPlanets();
  void renderSpaceships();
  void renderNebula();
  
  // Utility functions
  CRGB getStarColor(uint8_t brightness);
  CRGB getCometColor();
  CRGB getPlanetColor();
  CRGB getSpaceshipColor();
  
  void drawPixel(float x, float y, CRGB color, uint8_t brightness = 255);
  void drawLine(float x1, float y1, float x2, float y2, CRGB color, uint8_t brightness = 255);
  void drawSpaceship(float x, float y, uint8_t frame, CRGB color);
  
  bool isInBounds(float x, float y) const;
  float randomFloat(float min, float max);
};

// Space animation configuration constants
#define SPACE_STAR_COUNT 40
#define SPACE_COMET_COUNT 3
#define SPACE_PLANET_COUNT 2
#define SPACE_SPACESHIP_COUNT 1

#define SPACE_PARALLAX_SPEED 1.0f
#define SPACE_STAR_SPEED_MIN 0.1f
#define SPACE_STAR_SPEED_MAX 2.0f

#define SPACE_COMET_SPAWN_INTERVAL 3000  // milliseconds
#define SPACE_PLANET_SPAWN_INTERVAL 8000 // milliseconds
#define SPACE_SPACESHIP_SPAWN_INTERVAL 12000 // milliseconds 