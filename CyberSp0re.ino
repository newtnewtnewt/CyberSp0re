#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <string>
#include <vector>

SPIClass SDSPI(HSPI);

// You need these to have the SD card read like Noah has it configured
#define MY_CS    27 // Blue 
#define MY_SCLK  26 // Green 
#define MY_MOSI  25 // Orange
#define MY_MISO  33 // White

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  21 // Blue
#define PN532_MISO 22 // White
#define PN532_MOSI 17 // Orange
#define PN532_SS   15 // Yellow

// Breakout with SPI connection
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);



void setupNFC() {
  Serial.begin(115200);
  while (!Serial) delay(1); // for Leonardo/Micro/Zero

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);

  Serial.println("Waiting for an ISO14443A card");

}


void setupSDCard() {
    SDSPI.begin(MY_SCLK, MY_MISO, MY_MOSI, MY_CS);
    //Assuming use of SPI SD card
    if (!SD.begin(MY_CS, SDSPI)) {
        Serial.println("Card Mount Failed");
    } 
    else {
        Serial.println("SDCard Mount PASS");
    }
}

String checkNFCScan() {
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };	// Buffer to store the returned UID
  uint8_t uidLength;				// Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  // 10 for 10 MS delay so it doesn't infinitely block
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength, 100);

  String uidOfScannedCard = "";

  if (success) {
    int offset = 0;
    char uidStr[32];

    // The absurd process for creating a string from a uint8_t in Arduino C++
    for (uint8_t i = 0; i < uidLength; i++) {
        offset += snprintf(uidStr + offset,
                           sizeof(uidStr) - offset,
                           "%02X%s",
                           uid[i],
                           (i < uidLength - 1) ? ":" : "");
    }

    String uidOfScannedCard(uidStr);
    Serial.println("Found a card! UID Length: " + String(uidLength) + ", UID: " + uidOfScannedCard);
    // Wait 1 second before continuing
    return uidOfScannedCard;
  }
  else {
    // PN532 probably timed out waiting for a card
    return uidOfScannedCard;
  }
}

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
// TODO: Check against the existing database of saved Mutations we've saved
std::vector<String> alreadyMutatedWith;

uint8_t generation = 0;
unsigned long lastMutationTime = 0;
const unsigned long MUTATION_INTERVAL = 1000; // 1 second per generation
float creatureX = 120; // Center of screen
float creatureY = 120;
float moveDirection = 0;

// Display dimensions
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 135;

void resetCreatureData() {
  if (SD.exists("/creature.dat")) {
    SD.remove("/creature.dat");
    Serial.println("Reset creature data, creature.dat deleted.");
  } else {
    Serial.println("Attempted to reset creature data, creature.dat did not exist.");
  }
}

void saveCreatureData() {
  File file = SD.open("/creature.dat", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open creature.dat for writing!");
    return;
  }

  // --- Save CreatureGenes struct ---
  file.write((uint8_t*)&currentCreature, sizeof(CreatureGenes));

  // --- Save generation (int or uint16_t or whatever type you use) ---
  file.write((uint8_t*)&generation, sizeof(generation));

  // --- Save vector size ---
  uint8_t count = alreadyMutatedWith.size();
  file.write(count);

  // --- Save each string as null-terminated UTF-8 ---
  for (auto &s : alreadyMutatedWith) {
    file.write((const uint8_t*)s.c_str(), s.length());
    file.write('\0');  // null terminator
  }

  file.close();
  Serial.println("Saved creature.dat successfully.");
}

String loadCreatureData() {
  File file = SD.open("/creature.dat", FILE_READ);
  if (!file) {
    Serial.println("Failed to open creature.dat for reading!");
    return "Empty";
  }

  // --- Load CreatureGenes struct ---
  if (file.read((uint8_t*)&currentCreature, sizeof(CreatureGenes)) != sizeof(CreatureGenes)) {
    Serial.println("Error: creature.dat is corrupted (struct).");
    file.close();
    return "Fail";
  }

  // --- Load generation ---
  if (file.read((uint8_t*)&generation, sizeof(generation)) != sizeof(generation)) {
    Serial.println("Error: creature.dat is corrupted (generation).");
    file.close();
    return "Fail";
  }

  // --- Load vector count ---
  int count = file.read();
  if (count < 0) {
    Serial.println("Error: creature.dat is corrupted (count).");
    file.close();
    return "Fail";
  }

  alreadyMutatedWith.clear();
  alreadyMutatedWith.reserve(count);

  // --- Load null-terminated strings ---
  String temp = "";
  while (alreadyMutatedWith.size() < count && file.available()) {
    char c = file.read();
    if (c == '\0') {
      alreadyMutatedWith.push_back(temp);
      Serial.println("Added a mutated value: " + temp);
      temp = "";
    } else {
      temp += c;
    }
  }

  file.close();
  Serial.println("Loaded creature.dat successfully.");

  // Debug print
  Serial.println("=== Loaded Genes ===");
  Serial.println("Body size: " + String(currentCreature.bodySize));
  Serial.println("Body color: " + 
                 String(currentCreature.bodyColorR) + "," +
                 String(currentCreature.bodyColorG) + "," +
                 String(currentCreature.bodyColorB));
  Serial.println("Appendage count: " + String(currentCreature.appendageCount));

  Serial.println("Generation: " + String(generation));

  Serial.println("=== Mutated With ===");
  for (auto &s : alreadyMutatedWith) Serial.println(s);

  return "Success";
}

void setup() {
  Serial.begin(115200);
  Serial.println("Cyb3rSp0r3 - Genetic Evolution Simulator");

  // Start the SD card
  setupSDCard();

  // We want to see the 'SD card message' 
  delay(1000);

  // Start the NFC 
  setupNFC();
  

  // Initialize display
  tft.init();
  tft.setRotation(1); // Landscape orientation
  tft.fillScreen(TFT_BLACK);
  
  // Initialize random seed
  randomSeed(analogRead(34)); // Use ADC pin for random seed
  
  String loadStatus = loadCreatureData();
  if (loadStatus == "Empty" || loadStatus == "Fail") {
    // Initialize starting creature with basic genes
    Serial.println("File was not able to be opened, status: " + loadStatus + ", recreating creature");
    initializeCreature();
    saveCreatureData();
  }
  else {
    Serial.println("Successfully loaded creature data from file!");
  }

  // Display welcome message
  displayWelcome();
  delay(2000);
  tft.fillScreen(TFT_BLACK);
  
  Serial.println("Evolution simulation started!");
}

bool inVector(const String &value, const std::vector<String> &vec) {
    for (const auto &s : vec) {
        if (s == value) {
            return true;
        }
    }
    return false;
}

void loop() {
  String mutateUID = checkNFCScan();
  if (mutateUID.length() == 0) {
    // We didn't detect anything from NFC chip, continue as normal
  }
  else {
    bool alreadyMutated = inVector(mutateUID, alreadyMutatedWith);
    if (alreadyMutated) {
      Serial.println("Already mutated with that one!");
      // TODO: Display message on screen if not new mutation
    }
    else {
      Serial.println("Yay! A new mutation! UID: " + mutateUID);
      generation++;
      mutateCreature();
      alreadyMutatedWith.push_back(mutateUID);
      // Clear and redraw
      tft.fillScreen(TFT_BLACK);
      // TODO: Display message if new mutation (maybe stat sheet)
      saveCreatureData();
    }
  }
  
  drawCreature();
  drawUI();
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
  
  Serial.print("Generation "); 
  Serial.print(generation);
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
    
  // Creature stats
  tft.setCursor(5, 15);
  tft.setTextSize(1);
  tft.print("Size:");
  tft.print(currentCreature.bodySize);
  
  tft.setCursor(5, 25);
  tft.print("Apps:");
  tft.print(currentCreature.appendageCount);
  
  tft.setCursor(5, 35);
  tft.print("IQ:");
  tft.print(currentCreature.intelligence);
  
  tft.setCursor(5, 45);
  tft.print("Spd:");
  tft.print(currentCreature.speed);
  
  // Title
  tft.setCursor(150, 5);
  tft.setTextColor(TFT_CYAN);
  tft.print("Cyb3rSp0r3");
  
  tft.setCursor(150, 15);
  tft.setTextColor(TFT_YELLOW);
  tft.print("DEFCON 2025");
}

void displayWelcome() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(50, 40);
  tft.print("Cyb3rSp0r3");
  
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(1);
  tft.setCursor(30, 70);
  tft.print("Genetic Evolution Simulator");
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(70, 90);
  tft.print("DEFCON 2026");
  
  //TODO: Display current number of acquired mutations

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(60, 110);
  tft.print("Initializing...");
}
