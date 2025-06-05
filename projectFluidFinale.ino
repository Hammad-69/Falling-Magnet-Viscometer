#include <math.h>
#include <LiquidCrystal.h> // Include the LiquidCrystal library

// --- Keypad Code ---
const int rowPins[] = {7,6, 5, 4};
const int colPins[] = {3, 2, A2, A3};
const char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', ' B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', '.'} 
};
String enteredValue = "";
float densityBall = 0.0;
float densityOfFluid = 0.0;
float radiusBall = 0.00;
int inputState = 0; // 0: massOfBall, 1: densityOfFluid, 2: diameterOfBall
bool valueEntered = false;
bool inputVariablesSet = false; // Flag for input variables
const char NO_KEY = 0;

char getKey() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(rowPins[i], LOW);
    for (int j = 0; j < 4; j++) {
      if (digitalRead(colPins[j]) == LOW) {
        delay(50);
        while (digitalRead(colPins[j]) == LOW);
        digitalWrite(rowPins[i], HIGH);
        return keys[i][j];
      }
    }
    digitalWrite(rowPins[i], HIGH);
  }
  return NO_KEY;
}
// --- End of Keypad Code ---

float dynamicViscosity = 0;
float kinematicViscosity = 0;

// --- Your Sensor Code Variables ---
int hallSensor1Pin = A0;
int hallSensor2Pin = A1;
int thresholdSensor1High = 530;
int thresholdSensor1Low = 505;
int thresholdSensor2High = 530;
int thresholdSensor2Low = 520;
float sensor1Value;
float sensor2Value;
unsigned long triggerTimeSensor1 = 0;
unsigned long triggerTimeSensor2 = 0;
float timeDiffSeconds = 0.0;
bool sensor1Triggered = false;
bool sensor2Triggered = false;
float velocity = 0.0;
const float distanceBetweenSensors = 0.305;
const float pi = M_PI;
float faxen=0.0;
float z=0.0;

// --- LCD Pin Definitions
const int rs = 8, en =9, d4 = 10 , d5 = 11, d6 = 12, d7 = 13;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
byte etaChar[8] = {  // Custom character for viscosity
  B10000,
  B10110,
  B11001,
  B10001,
  B10001,
  B00001,
  B00001,
  B00001
};
byte rhoChar[8] = {// Custom character for density (ρ)
  B00110,
  B01001,
  B01001,
  B01110,
  B01000,
  B11000,
  B10000,
  B00000
};

void setup() {
  Serial.begin(9600); // Keep serial for debugging if needed
  lcd.begin(16, 2);   // Initialize the LCD with 16 columns and 2 rows
  lcd.print("Keypad Ready");
  lcd.setCursor(0, 1);
  lcd.print("*=Select, #=OK");

  // --- Keypad setup ---
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
  }
  for (int i = 0; i < 4; i++) {
    pinMode(colPins[i], INPUT_PULLUP);
  }

  // --- Your Sensor setup ---
  // Create custom characters
  lcd.createChar(0, etaChar);  // Viscosity (η) = ASCII 0
  lcd.createChar(1, rhoChar);  // Density (ρ) = ASCII 1

  Serial.println("Sensor system initialized.");
  lcd.clear();
  lcd.print("Enter ");
  lcd.write((uint8_t)1);
  lcd.print(" Ball: ");
}

void loop() {
  char key = getKey();

  if (key != NO_KEY) {
    if (isdigit(key) || key == '.') {
      enteredValue += key;
      lcd.setCursor(0, 1);
      lcd.print("Input: ");
      lcd.print(enteredValue);
      for (int i = enteredValue.length() < 16 ? enteredValue.length() : 16; i < 16; i++) {
        lcd.print(" "); // Clear the rest of the line
      }
    } else if (key == 'C') {
      lcd.clear();
      if (enteredValue.length() > 0) {
        // Replace 'D' with '.' before converting to float
        enteredValue.replace('D', '.');
        float enteredFloat = enteredValue.toFloat();
        valueEntered = true;
        if (inputState == 0) {
          densityBall = enteredFloat;
          lcd.print(": ");
          lcd.print(enteredFloat);
          inputState = 1; // Move to the next input
          lcd.setCursor(0, 1);
          lcd.write((uint8_t)1);
          lcd.print(" (kg/m^3),C=OK");
        } else if (inputState == 1) {
          densityOfFluid = enteredFloat;
          lcd.write((uint8_t)1);
          lcd.print(" fluid: ");
          lcd.print(densityOfFluid);
          lcd.print("kg/m^3");
          inputState = 2; // Move to the next input
          Serial.print("densityOfFluid (kg/m³): "); Serial.println(densityOfFluid);
          lcd.setCursor(0, 1);
          lcd.print("*=Radius, C=OK");
        } else if (inputState == 2) {
          radiusBall = enteredFloat/1000.0;
          lcd.print("Radius(mm):");
          lcd.print(enteredFloat);
          Serial.print("radius of ball (m): "); Serial.println(radiusBall);
          inputVariablesSet = true; // All inputs done
          lcd.setCursor(0, 1);
          lcd.print("Ready to Drop");
        }
        enteredValue = "";
      } else {
        lcd.print("No input");
        lcd.setCursor(0, 1);
        lcd.print("*=Select, C=OK");
      }
    } else if (key == '*') {
      enteredValue = ""; // Always clear the entered value when '*' is pressed
      inputState = (inputState + 1) % 3;
      lcd.clear();
      if (inputState == 0) {
        lcd.print("Enter ");
        lcd.write((uint8_t)1);
        lcd.print(" ball");
      } else if (inputState == 1) {
        lcd.print("Enter ");
        lcd.write((uint8_t)1);
        lcd.print(" fluid");
        lcd.setCursor(0, 1);
        lcd.print("(kg/m^3)");
      } else if (inputState == 2) {
        lcd.print("Enter radius");
        lcd.setCursor(0, 1);
        lcd.print("(mm)");
       
      }
      enteredValue = "";
    }
  else if (key == '#') { // Use '#' as the backspace key
      if (enteredValue.length() > 0) {
        enteredValue.remove(enteredValue.length() - 1); // Remove the last character
        lcd.setCursor(0, 1);
        lcd.print("Input: ");
        lcd.print(enteredValue);
        for (int i = enteredValue.length() < 16 ? enteredValue.length() : 16; i < 16; i++) {
          lcd.print(" "); // Clear the rest of the line
        }
      }
    }
  }

  // Your sensor reading and logic
  sensor1Value = analogRead(hallSensor1Pin);
  sensor2Value = analogRead(hallSensor2Pin);

  if (sensor1Value < thresholdSensor1Low || sensor1Value > thresholdSensor1High && !sensor1Triggered) {
    triggerTimeSensor1 = micros();
    sensor1Triggered = true;
    lcd.clear();
    lcd.print("Sensor 1 Trig");
  }
  if (sensor1Triggered && (sensor2Value < thresholdSensor2Low || sensor2Value > thresholdSensor2High) && !sensor2Triggered) {
    triggerTimeSensor2 = micros();
    sensor2Triggered = true;
    timeDiffSeconds = (triggerTimeSensor2 - triggerTimeSensor1) / 1000000.00;
    lcd.print("Sensor 2 Trig");
    delay(2000);
    lcd.clear();
    lcd.print("Time: ");
    lcd.print(timeDiffSeconds);
    Serial.print("timeDiff (s): "); Serial.println(timeDiffSeconds);
    lcd.print(" s");
    delay(2000);

    if (timeDiffSeconds > 0 && radiusBall > 0 && densityOfFluid > 0 && inputVariablesSet) {
    velocity = distanceBetweenSensors / timeDiffSeconds;
    lcd.setCursor(0, 0); // Ensure we start at the first line
    lcd.print("V: ");
    lcd.print(velocity);
    Serial.print("velocity (m/s): "); Serial.println(velocity);
    lcd.print(" m/s");
    delay(5000);
    lcd.clear();

    if(densityOfFluid==1000){
        faxen=0.6520056366;
        z=52.0;
        }    
    else{
      faxen=1.0;
      z=1.0;}
      //else if(densityOfFluid==1261)
    dynamicViscosity = (2.0/9.0) * pow(radiusBall, 2) * (9.81 / velocity) * (densityBall - densityOfFluid);
    // Display dynamic viscosity in cP (1 Pa·s = 1000 cP)
    lcd.setCursor(0, 0);
    lcd.write((uint8_t)0);
    lcd.print(": ");
    lcd.print(dynamicViscosity * 1000 * faxen/z);  // Convert to cP
    lcd.print(" cP");
    delay(5000);
    lcd.clear();

    lcd.clear();
    lcd.print("Ready"); // Example: Return to a ready state
      // Reset the sensor trigger flags to detect the next ball drop
    sensor1Triggered = false;
    sensor2Triggered = false;
    timeDiffSeconds = 0.0; // Also reset the time difference for the new measurement

  } else if (sensor1Triggered && sensor2Triggered) {
    lcd.setCursor(0, 1);
    lcd.print("Err: No Ball Data");
  }
  }
}