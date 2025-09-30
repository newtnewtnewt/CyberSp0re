#include <TFT_eSPI.h>
#include <WiFi.h>

// Display setup
TFT_eSPI tft = TFT_eSPI();

// Creature genetic structure
struct CreatureGenes {
  uint8_t bodySize;        // 0-255: affects overall size
  uint8_t bodyColorR;      // Red component
  uint8_t bodyColorG;      // Green component  
  uint8_t bodyColorB;      // Blue component
  uint8_t appendageCount;  // 0-8: number of appendages
  uint8_t appendageLength; // 0-255: length of appendages
  uint8_t eyeSize;         // 0-255: size of eyes
  uint8_t speed;           // 0-255: movement speed
  uint8_t aggression;      // 0-255: affects behavior
  uint8_t intelligence;    // 0-255: affects patterns
};

// Global variables
CreatureGenes currentCreature;
int generation = 0;
unsigned long lastMutationTime = 0;
const unsigned long MUTATION_INTERVAL = 1000; // 1 second per generation
float creatureX = 120; // Center of screen
float creatureY = 120;
float moveDirection = 0;

// Display dimensions
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 135;

void setup() {
  Serial.begin(115200);
  Serial.println("CyberSp0re - Genetic Evolution Simulator");
  
  // Initialize display
  tft.init();
  tft.setRotation(1); // Landscape orientation
  tft.fillScreen(TFT_BLACK);
  
  // Initialize random seed
  randomSeed(analogRead(34)); // Use ADC pin for random seed
  
  // Initialize starting creature with basic genes
  initializeCreature();
  
  // Display welcome message
  displayWelcome();
  delay(2000);
  
  Serial.println("Evolution simulation started!");
}

void loop() {
  // Check if it's time for next generation
  if (millis() - lastMutationTime >= MUTATION_INTERVAL) {
    mutateCreature();
    generation++;
    
    // Reset at generation 50
    if (generation >= 50) {
      generation = 0;
      initializeCreature();
      Serial.println("Creature reset to generation 0");
    }
    
    lastMutationTime = millis();
  }
  
  // Update creature movement (but slower for less spazzing)
  static unsigned long lastMovementUpdate = 0;
  if (millis() - lastMovementUpdate > 100) { // Update movement every 100ms
    updateCreatureMovement();
    lastMovementUpdate = millis();
  }
  
  // Clear and redraw
  tft.fillScreen(TFT_BLACK);
  drawCreature();
  drawUI();
  
  delay(150); // Slower, smoother animation
}

void initializeCreature() {
  // Start with basic "primordial" genes
  currentCreature.bodySize = 80;
  currentCreature.bodyColorR = 100;
  currentCreature.bodyColorG = 255;
  currentCreature.bodyColorB = 100;
  currentCreature.appendageCount = 2;
  currentCreature.appendageLength = 30;
  currentCreature.eyeSize = 20;
  currentCreature.speed = 50;
  currentCreature.aggression = 30;
  currentCreature.intelligence = 40;
  
  Serial.println("Creature initialized with primordial genes");
}

void mutateCreature() {
  // Each gene has a chance to mutate - WILDER MUTATIONS!
  const uint8_t MUTATION_RATE = 35; // 35% chance per gene - more frequent
  
  // Occasional "mega mutations" - rare but dramatic changes
  bool megaMutation = (random(100) < 5); // 5% chance of mega mutation
  
  if (random(100) < MUTATION_RATE) {
    int change = megaMutation ? random(-50, 51) : random(-35, 36);
    currentCreature.bodySize = constrain(currentCreature.bodySize + change, 15, 140);
  }
  
  if (random(100) < MUTATION_RATE) {
    int change = megaMutation ? random(-80, 81) : random(-50, 51);
    currentCreature.bodyColorR = constrain(currentCreature.bodyColorR + change, 0, 255);
  }
  
  if (random(100) < MUTATION_RATE) {
    int change = megaMutation ? random(-80, 81) : random(-50, 51);
    currentCreature.bodyColorG = constrain(currentCreature.bodyColorG + change, 0, 255);
  }
  
  if (random(100) < MUTATION_RATE) {
    int change = megaMutation ? random(-80, 81) : random(-50, 51);
    currentCreature.bodyColorB = constrain(currentCreature.bodyColorB + change, 0, 255);
  }
  
  if (random(100) < MUTATION_RATE) {
    int change = megaMutation ? random(-4, 5) : random(-3, 4);
    currentCreature.appendageCount = constrain(currentCreature.appendageCount + change, 0, 12);
  }
  
  if (random(100) < MUTATION_RATE) {
    int change = megaMutation ? random(-25, 26) : random(-18, 19);
    currentCreature.appendageLength = constrain(currentCreature.appendageLength + change, 5, 80);
  }
  
  if (random(100) < MUTATION_RATE) {
    int change = megaMutation ? random(-20, 21) : random(-12, 13);
    currentCreature.eyeSize = constrain(currentCreature.eyeSize + change, 3, 50);
  }
  
  if (random(100) < MUTATION_RATE) {
    int change = megaMutation ? random(-40, 41) : random(-25, 26);
    currentCreature.speed = constrain(currentCreature.speed + change, 5, 120);
  }
  
  if (random(100) < MUTATION_RATE) {
    int change = megaMutation ? random(-60, 61) : random(-35, 36);
    currentCreature.aggression = constrain(currentCreature.aggression + change, 0, 255);
  }
  
  if (random(100) < MUTATION_RATE) {
    int change = megaMutation ? random(-50, 51) : random(-30, 31);
    currentCreature.intelligence = constrain(currentCreature.intelligence + change, 0, 255);
  }
  
  // Rare "chaos mutation" - completely randomize one trait
  if (random(1000) < 3) { // 0.3% chance
    uint8_t trait = random(10);
    switch(trait) {
      case 0: currentCreature.bodySize = random(15, 141); break;
      case 1: currentCreature.bodyColorR = random(256); break;
      case 2: currentCreature.bodyColorG = random(256); break;
      case 3: currentCreature.bodyColorB = random(256); break;
      case 4: currentCreature.appendageCount = random(13); break;
      case 5: currentCreature.appendageLength = random(5, 81); break;
      case 6: currentCreature.eyeSize = random(3, 51); break;
      case 7: currentCreature.speed = random(5, 121); break;
      case 8: currentCreature.aggression = random(256); break;
      case 9: currentCreature.intelligence = random(256); break;
    }
    Serial.println("CHAOS MUTATION OCCURRED!");
  }
  
  Serial.print("Generation "); Serial.print(generation);
  Serial.print(" - Mutations complete. Body size: "); Serial.println(currentCreature.bodySize);
}

void updateCreatureMovement() {
  // Movement based on speed and intelligence genes
  float speedFactor = currentCreature.speed / 255.0;
  float intelligenceFactor = currentCreature.intelligence / 255.0;
  
  // More intelligent creatures have more purposeful movement
  if (intelligenceFactor > 0.5) {
    // Purposeful movement towards screen edges and back
    moveDirection += (random(-10, 11) * (1.0 - intelligenceFactor)) / 10.0;
  } else {
    // More random movement for less intelligent creatures
    moveDirection += random(-30, 31) / 10.0;
  }
  
  creatureX += cos(moveDirection) * speedFactor * 1.5;
  creatureY += sin(moveDirection) * speedFactor * 1.5;
  
  // Bounce off screen edges
  if (creatureX < 30 || creatureX > SCREEN_WIDTH - 30) {
    moveDirection = PI - moveDirection;
    creatureX = constrain(creatureX, 30, SCREEN_WIDTH - 30);
  }
  if (creatureY < 30 || creatureY > SCREEN_HEIGHT - 30) {
    moveDirection = -moveDirection;
    creatureY = constrain(creatureY, 30, SCREEN_HEIGHT - 30);
  }
}

void drawCreature() {
  // Calculate creature color
  uint16_t bodyColor = tft.color565(currentCreature.bodyColorR, 
                                   currentCreature.bodyColorG, 
                                   currentCreature.bodyColorB);
  
  // Draw appendages first (behind body)
  for (int i = 0; i < currentCreature.appendageCount; i++) {
    float angle = (2 * PI * i) / currentCreature.appendageCount;
    int appendageX = creatureX + cos(angle) * (currentCreature.bodySize / 4);
    int appendageY = creatureY + sin(angle) * (currentCreature.bodySize / 4);
    int endX = appendageX + cos(angle) * currentCreature.appendageLength;
    int endY = appendageY + sin(angle) * currentCreature.appendageLength;
    
    // Appendage color slightly darker than body
    uint16_t appendageColor = tft.color565(currentCreature.bodyColorR * 0.7, 
                                          currentCreature.bodyColorG * 0.7, 
                                          currentCreature.bodyColorB * 0.7);
    
    tft.drawLine(appendageX, appendageY, endX, endY, appendageColor);
    tft.fillCircle(endX, endY, 3, appendageColor); // Appendage tip
  }
  
  // Draw main body
  tft.fillCircle(creatureX, creatureY, currentCreature.bodySize / 2, bodyColor);
  tft.drawCircle(creatureX, creatureY, currentCreature.bodySize / 2, TFT_WHITE);
  
  // Draw eyes
  uint16_t eyeColor = TFT_WHITE;
  if (currentCreature.aggression > 128) {
    eyeColor = TFT_RED; // Aggressive creatures have red eyes
  }
  
  int eyeOffset = currentCreature.bodySize / 6;
  tft.fillCircle(creatureX - eyeOffset, creatureY - eyeOffset, 
                currentCreature.eyeSize / 4, eyeColor);
  tft.fillCircle(creatureX + eyeOffset, creatureY - eyeOffset, 
                currentCreature.eyeSize / 4, eyeColor);
  
  // Eye pupils
  tft.fillCircle(creatureX - eyeOffset, creatureY - eyeOffset, 
                currentCreature.eyeSize / 8, TFT_BLACK);
  tft.fillCircle(creatureX + eyeOffset, creatureY - eyeOffset, 
                currentCreature.eyeSize / 8, TFT_BLACK);
}

void drawUI() {
  // Generation counter
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.print("Gen: ");
  tft.print(generation);
  
  // Progress bar for generation cycle
  int progress = ((millis() - lastMutationTime) * 50) / MUTATION_INTERVAL;
  tft.drawRect(5, 15, 52, 8, TFT_WHITE);
  tft.fillRect(6, 16, progress, 6, TFT_GREEN);
  
  // Creature stats
  tft.setCursor(5, 30);
  tft.setTextSize(1);
  tft.print("Size:");
  tft.print(currentCreature.bodySize);
  
  tft.setCursor(5, 40);
  tft.print("Apps:");
  tft.print(currentCreature.appendageCount);
  
  tft.setCursor(5, 50);
  tft.print("IQ:");
  tft.print(currentCreature.intelligence);
  
  tft.setCursor(5, 60);
  tft.print("Spd:");
  tft.print(currentCreature.speed);
  
  // Title
  tft.setCursor(150, 5);
  tft.setTextColor(TFT_CYAN);
  tft.print("CyberSp0re");
  
  tft.setCursor(150, 15);
  tft.setTextColor(TFT_YELLOW);
  tft.print("DEFCON 2025");
}

void displayWelcome() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(50, 40);
  tft.print("CyberSp0re");
  
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(1);
  tft.setCursor(30, 70);
  tft.print("Genetic Evolution Simulator");
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(70, 90);
  tft.print("DEFCON 2025");
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(60, 110);
  tft.print("Initializing...");
}
