#include "space_animation.h"
#include "performance_monitor.h"
#include "content_manager.h"

// External references
extern PerformanceMonitor* g_perfMonitor;
extern CRGB leds[];

SpaceAnimation::SpaceAnimation() 
  : maxStars(SPACE_STAR_COUNT), maxComets(SPACE_COMET_COUNT), 
    maxPlanets(SPACE_PLANET_COUNT), maxSpaceships(SPACE_SPACESHIP_COUNT),
    parallaxSpeed(SPACE_PARALLAX_SPEED), starSpeedMin(SPACE_STAR_SPEED_MIN), starSpeedMax(SPACE_STAR_SPEED_MAX),
    paused(false), lastUpdate(0), cometSpawnTimer(0), planetSpawnTimer(0), spaceshipSpawnTimer(0),
    nebularPhase(0), nebulaTimer(0) {
  
  // Initialize object arrays
  stars.resize(maxStars);
  comets.resize(maxComets);
  planets.resize(maxPlanets);
  spaceships.resize(maxSpaceships);
  
  // Initialize stars
  initializeStars();
  
  // Set initial timers
  lastUpdate = millis();
  cometSpawnTimer = millis();
  planetSpawnTimer = millis();
  spaceshipSpawnTimer = millis();
  nebulaTimer = millis();
}

void SpaceAnimation::update() {
  if (paused) return;
  
  unsigned long currentTime = millis();
  if (currentTime - lastUpdate < 16) return; // ~60 FPS limit
  
  lastUpdate = currentTime;
  
  updateStars();
  updateComets();
  updatePlanets();
  updateSpaceships();
  updateNebula();
}

void SpaceAnimation::render() {
  if (paused) return;
  
  START_TIMER(space_render);
  
  FastLED.clear();
  
  // Render in back-to-front order
  renderNebula();    // Background nebula
  renderStars();     // Starfield
  renderPlanets();   // Planets
  renderComets();    // Comets with trails
  renderSpaceships(); // Foreground spaceships
  
  START_TIMER(led_show_space);
  FastLED.show();
  END_FASTLED_TIMER(led_show_space, g_perfMonitor->getMetrics().fastLEDShowTime);
  
  END_TIMER(space_render, g_perfMonitor->getMetrics().calculationTime);
}

void SpaceAnimation::reset() {
  // Reset all objects
  for (auto& star : stars) star.active = false;
  for (auto& comet : comets) comet.active = false;
  for (auto& planet : planets) planet.active = false;
  for (auto& spaceship : spaceships) spaceship.active = false;
  
  // Reinitialize
  initializeStars();
  nebularPhase = 0;
  
  unsigned long currentTime = millis();
  lastUpdate = currentTime;
  cometSpawnTimer = currentTime;
  planetSpawnTimer = currentTime;
  spaceshipSpawnTimer = currentTime;
  nebulaTimer = currentTime;
}

void SpaceAnimation::initializeStars() {
  for (int i = 0; i < maxStars; i++) {
    stars[i].x = randomFloat(0, DISPLAY_WIDTH);
    stars[i].y = randomFloat(0, DISPLAY_HEIGHT);
    stars[i].speed = randomFloat(starSpeedMin, starSpeedMax);
    stars[i].brightness = random(50, 255);
    stars[i].color = getStarColor(stars[i].brightness);
    stars[i].active = true;
  }
}

void SpaceAnimation::updateStars() {
  for (auto& star : stars) {
    if (!star.active) continue;
    
    // Move star left based on its speed
    star.x -= star.speed * parallaxSpeed;
    
    // Wrap around when star goes off screen
    if (star.x < 0) {
      star.x = DISPLAY_WIDTH;
      star.y = randomFloat(0, DISPLAY_HEIGHT);
      star.speed = randomFloat(starSpeedMin, starSpeedMax);
      star.brightness = random(50, 255);
      star.color = getStarColor(star.brightness);
    }
    
    // Slight twinkle effect
    if (random(100) < 2) { // 2% chance per frame
      star.brightness = random(50, 255);
      star.color = getStarColor(star.brightness);
    }
  }
}

void SpaceAnimation::updateComets() {
  unsigned long currentTime = millis();
  
  // Spawn new comets periodically
  if (currentTime - cometSpawnTimer > SPACE_COMET_SPAWN_INTERVAL) {
    spawnComet();
    cometSpawnTimer = currentTime;
  }
  
  for (auto& comet : comets) {
    if (!comet.active) continue;
    
    // Move comet
    comet.x += comet.speedX;
    comet.y += comet.speedY;
    
    // Remove comet if it goes off screen
    if (comet.x < -10 || comet.x > DISPLAY_WIDTH + 10 || 
        comet.y < -10 || comet.y > DISPLAY_HEIGHT + 10) {
      comet.active = false;
    }
  }
}

void SpaceAnimation::updatePlanets() {
  unsigned long currentTime = millis();
  
  // Spawn new planets periodically
  if (currentTime - planetSpawnTimer > SPACE_PLANET_SPAWN_INTERVAL) {
    spawnPlanet();
    planetSpawnTimer = currentTime;
  }
  
  for (auto& planet : planets) {
    if (!planet.active) continue;
    
    // Move planet left
    planet.x -= planet.speed;
    planet.phase += 0.05f; // Slow rotation
    
    // Remove planet if it goes off screen
    if (planet.x < -planet.size) {
      planet.active = false;
    }
  }
}

void SpaceAnimation::updateSpaceships() {
  unsigned long currentTime = millis();
  
  // Spawn new spaceships periodically
  if (currentTime - spaceshipSpawnTimer > SPACE_SPACESHIP_SPAWN_INTERVAL) {
    spawnSpaceship();
    spaceshipSpawnTimer = currentTime;
  }
  
  for (auto& spaceship : spaceships) {
    if (!spaceship.active) continue;
    
    // Move spaceship
    spaceship.x += spaceship.speedX;
    spaceship.y += spaceship.speedY;
    
    // Animate spaceship frame
    if (currentTime - spaceship.lastFrameUpdate > 200) {
      spaceship.frame = (spaceship.frame + 1) % 4;
      spaceship.lastFrameUpdate = currentTime;
    }
    
    // Remove spaceship if it goes off screen
    if (spaceship.x < -10 || spaceship.x > DISPLAY_WIDTH + 10 || 
        spaceship.y < -5 || spaceship.y > DISPLAY_HEIGHT + 5) {
      spaceship.active = false;
    }
  }
}

void SpaceAnimation::updateNebula() {
  unsigned long currentTime = millis();
  if (currentTime - nebulaTimer > 100) {
    nebularPhase = (nebularPhase + 1) % 255;
    nebulaTimer = currentTime;
  }
}

void SpaceAnimation::spawnComet() {
  for (auto& comet : comets) {
    if (!comet.active) {
      // All comets spawn from right edge and move horizontally left at different speeds
      comet.x = DISPLAY_WIDTH + 5;
      comet.y = randomFloat(0, DISPLAY_HEIGHT);
      comet.speedX = randomFloat(-4.0f, -1.5f); // Faster horizontal speeds
      comet.speedY = 0; // Pure horizontal movement
      
      comet.trailLength = random(3, 8);
      comet.color = getCometColor();
      comet.active = true;
      break; // Only spawn one at a time
    }
  }
}

void SpaceAnimation::spawnPlanet() {
  for (auto& planet : planets) {
    if (!planet.active) {
      planet.x = DISPLAY_WIDTH + 10;
      planet.y = randomFloat(1, DISPLAY_HEIGHT - 3);
      planet.speed = randomFloat(0.3f, 1.0f);
      planet.size = random(2, 5);
      planet.color = getPlanetColor();
      planet.phase = 0;
      planet.active = true;
      break; // Only spawn one at a time
    }
  }
}

void SpaceAnimation::spawnSpaceship() {
  for (auto& spaceship : spaceships) {
    if (!spaceship.active) {
      // Spawn from left or right edge, but move purely horizontally
      if (random(2)) {
        spaceship.x = -5;
        spaceship.speedX = randomFloat(2.0f, 4.0f); // Faster speeds
      } else {
        spaceship.x = DISPLAY_WIDTH + 5;
        spaceship.speedX = randomFloat(-4.0f, -2.0f); // Faster speeds
      }
      
      spaceship.y = randomFloat(1, DISPLAY_HEIGHT - 2);
      spaceship.speedY = 0; // Pure horizontal movement
      spaceship.frame = 0;
      spaceship.color = getSpaceshipColor();
      spaceship.lastFrameUpdate = millis();
      spaceship.active = true;
      break; // Only spawn one at a time
    }
  }
}

void SpaceAnimation::renderStars() {
  for (const auto& star : stars) {
    if (!star.active) continue;
    drawPixel(star.x, star.y, star.color, star.brightness);
  }
}

void SpaceAnimation::renderComets() {
  for (const auto& comet : comets) {
    if (!comet.active) continue;
    
    // Draw comet head
    drawPixel(comet.x, comet.y, comet.color, 255);
    
    // Draw comet tail
    for (int i = 1; i <= comet.trailLength; i++) {
      float tailX = comet.x - (comet.speedX * i * 0.3f);
      float tailY = comet.y - (comet.speedY * i * 0.3f);
      uint8_t tailBrightness = 255 * (comet.trailLength - i) / comet.trailLength;
      
      if (tailX >= 0 && tailX < DISPLAY_WIDTH && tailY >= 0 && tailY < DISPLAY_HEIGHT) {
        drawPixel(tailX, tailY, comet.color, tailBrightness);
      }
    }
  }
}

void SpaceAnimation::renderPlanets() {
  for (const auto& planet : planets) {
    if (!planet.active) continue;
    
    // Draw planet as a colored blob
    for (int dx = 0; dx < planet.size; dx++) {
      for (int dy = 0; dy < planet.size; dy++) {
        float distance = sqrt(dx*dx + dy*dy);
        if (distance < planet.size / 2.0f) {
          uint8_t brightness = 255 * (1.0f - distance / (planet.size / 2.0f));
          // Add rotation effect
          brightness = brightness * (0.7f + 0.3f * sin(planet.phase + dx * 0.5f));
          drawPixel(planet.x + dx, planet.y + dy, planet.color, brightness);
        }
      }
    }
  }
}

void SpaceAnimation::renderSpaceships() {
  for (const auto& spaceship : spaceships) {
    if (!spaceship.active) continue;
    drawSpaceship(spaceship.x, spaceship.y, spaceship.frame, spaceship.color);
  }
}

void SpaceAnimation::renderNebula() {
  // Subtle background nebula effect
  for (int x = 0; x < DISPLAY_WIDTH; x += 4) {
    for (int y = 0; y < DISPLAY_HEIGHT; y += 2) {
      uint8_t intensity = abs(sin((x + nebularPhase) * 0.1f) * cos((y + nebularPhase) * 0.15f)) * 30;
      if (intensity > 15) {
        CRGB nebulaColor = CHSV(160 + (nebularPhase % 60), 200, intensity);
        drawPixel(x, y, nebulaColor, intensity);
      }
    }
  }
}

CRGB SpaceAnimation::getStarColor(uint8_t brightness) {
  // Different star colors based on brightness (like real stars)
  if (brightness > 200) return CHSV(0, 0, brightness);      // White (hot)
  if (brightness > 150) return CHSV(40, 100, brightness);   // Yellow
  if (brightness > 100) return CHSV(20, 150, brightness);   // Orange
  return CHSV(0, 200, brightness);                           // Red (cool)
}

CRGB SpaceAnimation::getCometColor() {
  // Comets are typically blue-white with some variation
  return CHSV(random(150, 200), random(100, 200), 255);
}

CRGB SpaceAnimation::getPlanetColor() {
  // Various planet colors
  uint8_t hue = random(4) * 60 + random(30); // 0-30, 60-90, 120-150, 180-210
  return CHSV(hue, random(150, 255), random(120, 200));
}

CRGB SpaceAnimation::getSpaceshipColor() {
  // Metallic colors for spaceships
  return CHSV(random(200, 240), 100, random(150, 255));
}

void SpaceAnimation::drawPixel(float x, float y, CRGB color, uint8_t brightness) {
  int pixelX = (int)x;
  int pixelY = (int)y;
  
  // Use the same LED addressing as the main set_led function
  if (pixelX >= 0 && pixelX < NUM_CHARS * 5 && pixelY >= 0 && pixelY < 7) {
    int offset = pixelX / 5 * 30;
    int ledIndex = pixelY * 5 + pixelX + offset;
    int totalLeds = 5 * 7 * NUM_CHARS; // Calculate total LEDs
    if (ledIndex >= 0 && ledIndex < totalLeds) {
      CRGB scaledColor = color;
      scaledColor.fadeToBlackBy(255 - brightness);
      leds[ledIndex] = scaledColor;
    }
  }
}

void SpaceAnimation::drawLine(float x1, float y1, float x2, float y2, CRGB color, uint8_t brightness) {
  // Simple line drawing for trails
  float dx = x2 - x1;
  float dy = y2 - y1;
  float distance = sqrt(dx*dx + dy*dy);
  
  if (distance > 0) {
    int steps = (int)distance + 1;
    for (int i = 0; i <= steps; i++) {
      float t = (float)i / steps;
      float x = x1 + dx * t;
      float y = y1 + dy * t;
      drawPixel(x, y, color, brightness);
    }
  }
}

void SpaceAnimation::drawSpaceship(float x, float y, uint8_t frame, CRGB color) {
  // Simple spaceship sprite (3x2 pixels with animation)
  int baseX = (int)x;
  int baseY = (int)y;
  
  // Different frames for engine glow animation
  switch (frame % 4) {
    case 0:
      drawPixel(baseX, baseY, color, 255);         // Main body
      drawPixel(baseX + 1, baseY, color, 200);     // Body
      drawPixel(baseX - 1, baseY, CRGB::Red, 100); // Engine dim
      break;
    case 1:
      drawPixel(baseX, baseY, color, 255);         // Main body
      drawPixel(baseX + 1, baseY, color, 200);     // Body
      drawPixel(baseX - 1, baseY, CRGB::Red, 200); // Engine bright
      break;
    case 2:
      drawPixel(baseX, baseY, color, 255);         // Main body
      drawPixel(baseX + 1, baseY, color, 200);     // Body
      drawPixel(baseX - 1, baseY, CRGB::Orange, 255); // Engine very bright
      break;
    case 3:
      drawPixel(baseX, baseY, color, 255);         // Main body
      drawPixel(baseX + 1, baseY, color, 200);     // Body
      drawPixel(baseX - 1, baseY, CRGB::Red, 150); // Engine medium
      break;
  }
}

bool SpaceAnimation::isInBounds(float x, float y) const {
  return x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT;
}

float SpaceAnimation::randomFloat(float min, float max) {
  return min + (max - min) * (random(10000) / 10000.0f);
} 