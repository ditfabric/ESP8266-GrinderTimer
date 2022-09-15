#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <arduino-timer.h>

// OLED using default pins D1 (SCL) / D2 (SDA)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);

// default Grinding time
int GRINDING_TIME_SINGLE_MS = 1500;
int GRINDING_TIME_DOUBLE_MS = 3000;

// debounce for setup mode
int debounce = 200;   // the debounce time, increase if the output flickers
int btnSetupStatusPrevious = LOW;    // the previous reading from the input pin
unsigned long btnSetupLastMillis = 0;         // the last time the output pin was toggled

//timer
Timer<2> timer;

// I/O pin setup
const int grindRelay = 2; // D4

const int btnSingle = 14; // D5
int btnSingleStatus;
const int btnDouble = 12; //D6
int btnDoubleStatus;
const int btnSetup = 13; // D7
int btnSetupStatus;

// state
boolean grinding = false;
boolean editingSingle = false;
boolean editingDouble = false;



void setup() {
  Serial.begin(74880);
  Serial.println("Hello!");

  // setup I/O
  pinMode(grindRelay, OUTPUT);
  digitalWrite(grindRelay, HIGH); //Setting the grind output to high at default to open the relay when the board has power

  pinMode(btnDouble, INPUT);
  pinMode(btnSingle, INPUT);
  pinMode(btnSetup, INPUT);

  // init eeprom
  
  Serial.println("init EEPROM and read values");
  EEPROM.begin(512);  //Initialize EEPROM
  GRINDING_TIME_SINGLE_MS = readIntFromEEPROM(0);
  GRINDING_TIME_DOUBLE_MS =  readIntFromEEPROM(2);

  Serial.println("init display");
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.setRotation(2);
  displayPrintDefault();
  Serial.println("init complete!");
}

void loop() {
  // handle timers
  timer.tick();

  // read button states
  btnSingleStatus = digitalRead(btnSingle);
  btnDoubleStatus = digitalRead(btnDouble);
  btnSetupStatus = digitalRead(btnSetup);

  // handle grinding triggers if not grinding and not in edit mode
  if (!grinding && !editingSingle && !editingDouble) {
    if (btnSingleStatus == HIGH)  {
      Serial.println("Start grind single portion");
      startGrinding();
      displayPrintGrindingSingle();
      timer.in(GRINDING_TIME_SINGLE_MS, stopGrinding);
    }
    if (btnDoubleStatus == HIGH)  {
      Serial.println("Start grind double portion");
      startGrinding();
      displayPrintGrindingDouble();
      timer.in(GRINDING_TIME_DOUBLE_MS, stopGrinding);
    }
  }

 // when in editing mode, use single / double button to increase/decrease grinding time
 
  if (editingSingle) {
    if (btnSingleStatus == HIGH)  {
      Serial.println("Reduce single time");
      GRINDING_TIME_SINGLE_MS = constrain(GRINDING_TIME_SINGLE_MS - 100, 1000, 20000);
      delay(100);
      displayPrintEditSingle();
    }
    if (btnDoubleStatus == HIGH)  {
      Serial.println("Increase single time");
      GRINDING_TIME_SINGLE_MS = constrain(GRINDING_TIME_SINGLE_MS + 100, 1000, 20000);
      delay(100);
      displayPrintEditSingle();
    }
  }
  
  if (editingDouble) {
    if (btnSingleStatus == HIGH)  {
      Serial.println("Reduce double time");
      GRINDING_TIME_DOUBLE_MS = constrain(GRINDING_TIME_DOUBLE_MS - 100, 1000, 20000);
      delay(100);
      displayPrintEditDouble();
    }
    if (btnDoubleStatus == HIGH)  {
      Serial.println("Increase single time");
      GRINDING_TIME_DOUBLE_MS = constrain(GRINDING_TIME_DOUBLE_MS + 100, 1000, 20000);
      delay(100);
      displayPrintEditDouble();
    }
  }

  // debounce editing toggle and toggle through the modes: grinding, editing-single, editing-double
  if (btnSetupStatus == HIGH && btnSetupStatusPrevious == LOW && millis() - btnSetupLastMillis > debounce) {
    btnSetupLastMillis = millis();
    
    if (!editingSingle && !editingDouble) {
     
      // currently in grinding mode, change to editing single
      editingSingle = true;
      displayPrintEditSingle();
      
    } else if (editingSingle) {
      
      // finished editing the single time, store values and go to double time edit
      writeIntIntoEEPROM(0, GRINDING_TIME_SINGLE_MS);
      editingSingle = false;
      editingDouble = true;
      displayPrintEditDouble();
      
    } else {
      
      // finished editing the double time, store value and go to grdining mode
      writeIntIntoEEPROM(2, GRINDING_TIME_DOUBLE_MS);
      editingDouble = false;
      editingSingle = false;
      displayPrintDefault();
    }

  }
  btnSetupStatusPrevious = btnSetupStatus;


} // end loop



void startGrinding() {
  Serial.println("Start grinding");
  grinding = true;
  digitalWrite(grindRelay, LOW); //Turn output to low while grinding.
}


bool stopGrinding(void *arg) {

  Serial.println("Stopped grinding ");
  grinding = false;
  digitalWrite(grindRelay, HIGH);
  displayPrintDefault();
  return false; // the timer library needs this to clear the timer
}

// prints the default screen
void displayPrintDefault() {
  display.clearDisplay();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 10);     // Start at top-left corner
  display.cp437(true);
  display.write(char(3));
  display.write(" : ");
  display.print(float(float(GRINDING_TIME_SINGLE_MS) / 1000));
  display.write("s");
  display.setCursor(0, 40);

  display.write(char(3));
  display.write(char(3));
  display.write(": ");
  display.print(float(float(GRINDING_TIME_DOUBLE_MS) / 1000));
  display.write("s");
  display.display();
  // Clear the buffer
  display.clearDisplay();
}

// prints the single-grind screen
void displayPrintGrindingSingle() {
  display.setCursor(30, 20);     // Start at top-left corner
  display.setTextSize(3);      // Normal 1:1 pixel scale
  display.cp437(true);
  display.write(char(3));
  display.write("...");
  display.display();
  // Clear the buffer
  display.clearDisplay();
}

// prints the double grind screen
void displayPrintGrindingDouble() {
  display.setCursor(25, 20);     // Start at top-left corner
  display.setTextSize(3);      // Normal 1:1 pixel scale
  display.cp437(true);
  display.write(char(3));
  display.write(char(3));
  display.write("...");
  display.display();
  // Clear the buffer
  display.clearDisplay();
}

// prints the edit single screen
void displayPrintEditSingle() {
  display.clearDisplay();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 10);     // Start at top-left corner
  display.cp437(true);
  display.write(char(3));
  display.write(" : ");
  display.print(float(float(GRINDING_TIME_SINGLE_MS) / 1000));
  display.write("s");
  display.display();

  // Clear the buffer
  display.clearDisplay();
}

// prints the edit double screen
void displayPrintEditDouble() {
  display.clearDisplay();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 40);     // Start at top-left corner
  display.cp437(true);
  display.write(char(3));
  display.write(char(3));
  display.write(": ");
  display.print(float(float(GRINDING_TIME_DOUBLE_MS) / 1000));
  display.write("s");
  display.display();

  // Clear the buffer
  display.clearDisplay();
}


// hehlper function to write an int to eeprom
void writeIntIntoEEPROM(int address, int number)
{
  byte byte1 = number >> 8;
  byte byte2 = number & 0xFF;
  EEPROM.write(address, byte1);
  EEPROM.write(address + 1, byte2);
  EEPROM.commit();
}

// helper function to read an int from eeprom
int readIntFromEEPROM(int address)
{
  byte byte1 = EEPROM.read(address);
  byte byte2 = EEPROM.read(address + 1);
  return (byte1 << 8) + byte2;
}
