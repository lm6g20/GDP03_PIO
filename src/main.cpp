/*
 * Project: GDP03_MAIN
 * Description: //
 * 
 * Hardware Requirements:
 * - Microcontroller: [Arduino Board Model, e.g., Arduino Uno, Mega, ESP32]
 * - Components: [List sensors, motors, displays, etc.]
 * - Power: [Voltage and power requirements]
 * 
 * Pin Connections:
 * - [Component] -> [Pin Name] (e.g., LED -> Pin 13)
 * - [Component] -> [Pin Name]
 * 
 * Usage Instructions:
 * - [Any setup/calibration needed before running the code]
 * 
 * Author: Liam Marshall
 * Date: 03/03/2025
 * 
 * Univeristy of Southamtpon, FEEG6013(24-25)
 * GDP03 - A Prosthetic Foot Test Machine
 */

#include <Arduino.h> // Include the core Arduino functions (digitalWrite, pinMode, etc.)
#include <HX711_ADC.h> // Include the HX711_ADC library for interfacing with the HX711 load cell amplifier
#if defined(ESP8266)|| defined(ESP32) || defined(AVR) // Check if the board is ESP8266, ESP32, or AVR-based (Arduino Mega)
#include <EEPROM.h> // Include the EEPROM library for storing calibration values and settings in non-volatile memory
#endif // End of conditional compilation for EEPROM inclusion

//##### DEFINE PINOUT ####

// Define pins for Forefoot Loading Node
const int DIR_F = 35; // Direction pin for Forefoot stepper driver
const int PUL_F = 34; // Pulse pin for Forefoot stepper driver
const int ENA_F = 53; // Enable pin for Forefoot stepper driver
const int HX711_dout_F = 4; // HX711 Forefoot dout pin
const int HX711_sck_F = 5; // HX711 Forefoot sck pin

// Define pins for Heel Stepper Motor
const int DIR_H = 37; // Direction pin for Heel stepper driver
const int PUL_H = 36; // Pulse pin for Heel stepper driver
const int ENA_H = 51; // Enable pin for Heel stepper driver
const int HX711_dout_H = 6; // HX711 Heel dout pin
const int HX711_sck_H = 7; // HX711 Heel sck pin

// Microstepping Configuration
const int microstepSetting = 4; // 1/4 Microstepping
const int stepsPerRevolution = 200 * microstepSetting; // 800 steps per revolution
const int stepDelay = 500; // Speed in microseconds

// EEPROM adress for load cell calibration tare values
const int calVal_eepromAdress_F = 0; // EEPROM adress for calibration value load cell Forefoot (4 bytes)
const int calVal_eepromAdress_H = 4; // EEPROM adress for calibration value load cell Heel (4 bytes)
unsigned long t = 0;

// HX711 constructor's (dout pin, sck pin)
HX711_ADC LoadCell_F(HX711_dout_F, HX711_sck_F); //HX711 1
HX711_ADC LoadCell_H(HX711_dout_H, HX711_sck_H); //HX711 2

// Target force in grams
const float targetForce = 2000.0; 

// Set cycle count to zero
const int maxCycles = 2;

//#### DEFINE FUNCTIONS ####

void calibrate(HX711_ADC &LoadCell, int calAddr) {
  Serial.println("Performing automatic calibration...");
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");
  
  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte == 't') 
        LoadCell.tareNoDelay();
      }
      else {
        Serial.println("Invalid input. Please send 't' to tare.");
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true;
    }
  }

  Serial.println("Place a mass of know weight on the loadcell, Then send the weight of this mass in grams (i.e. 100.0 [g]) from serial monitor.");

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
        _resume = true;
      }
      else{
        Serial.println("Invalid mass input. Please enter a valid number.");
      }
    }
  }

  LoadCell.refreshDataSet(); //refresh the dataset to be sure that the known mass is measured correct
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); //get the new calibration value

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue);
  Serial.println(", use this as calibration value (calFactor) in your project sketch.");
  Serial.print("Save this value to EEPROM address ");
  Serial.print(calAddr);
  Serial.println("? (y: Yes, n: No)");

  _resume = false;
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calAddr, newCalibrationValue);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calAddr, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calAddr);
        _resume = true;

      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
      else{
        Serial.println("Invalid input. Please enter 'y' or 'n'.");
      }
    }
  }

  Serial.println("End calibration");
  Serial.println("***");
}
}

void manualCalibrationInput(HX711_ADC &LoadCell, int calAddr) {
  Serial.println("Performing manual calibration...");
  float oldCalibrationValue = LoadCell.getCalFactor();
  boolean _resume = false;
  Serial.println("***");
  Serial.print("Current value is: ");
  Serial.println(oldCalibrationValue);
  Serial.println("Now, send the new value from serial monitor.");
  float newCalibrationValue;
  while (_resume == false) {
    if (Serial.available() > 0) {
      newCalibrationValue = Serial.parseFloat();
      if (newCalibrationValue != 0) {
        Serial.print("New calibration value is: ");
        Serial.println(newCalibrationValue);
        LoadCell.setCalFactor(newCalibrationValue);
        _resume = true;
      }
    }
  }
  _resume = false;
  Serial.print("Save this value to EEPROM address ");
  Serial.print(calAddr);
  Serial.println("? (y: Yes, n: No)");
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calAddr, newCalibrationValue);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calAddr, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calAddr);
        _resume = true;
      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }
  Serial.println("End change calibration value");
  Serial.println("***");
}

void calibrateLoadCell(HX711_ADC &LoadCell, int calAddr) {
  unsigned long stabilizingtime = 2000;  // 2-second startup stabilization
  boolean _tare = true;  // Perform an automatic tare (zeroing)

  LoadCell.begin();  // Initialize the load cell
  LoadCell.start(stabilizingtime, _tare);  // Start with stabilization and tare
  while (!LoadCell.update());  // Wait until LoadCell updates

  // Serial.println("Do you want to recalibrate the load cell? (y: Yes auto (known weight), m: Manual, n: No)");

  while (true) {  // Loop to wait for user input
    if (Serial.available() > 0) {  // Check if user has typed something in Serial Monitor
      char response = Serial.read();  // Read the response character

      if (response == 'y') {  // If the user presses 'y', start the regular calibration process
        Serial.println("Starting regular calibration...");
        calibrate(LoadCell, calAddr);  // Call the calibrate function to begin the calibration process
        break;  // Break out of the loop once calibration is done
      } 
      else if (response == 'm') {  // If the user presses 'm', start the manual calibration process
        Serial.println("Starting manual calibration...");
        manualCalibrationInput(LoadCell, calAddr);  // Call the manual calibration function
        break;  // Break out of the loop once manual calibration is done
      } 
      else if (response == 'n') {  // If the user presses 'n', skip calibration
        Serial.println("Skipping calibration. Using saved value.");
        float savedValue;  // Declare a variable to hold the saved calibration value
        EEPROM.get(calAddr, savedValue);  // Retrieve the saved calibration value from EEPROM
        if (savedValue == 0) {
          Serial.println("Warning: Calibration value is invalid or not set. Using default.");
          LoadCell.setCalFactor(1.0);  // Set a default value or ask the user to calibrate
        } else {
          LoadCell.setCalFactor(savedValue);  // Use the saved calibration factor
        }
        break;  // Break out of the loop once the calibration value is set
      } 
      else {
        // If the user enters an invalid input, ask them again
        Serial.println("Invalid input. Please enter 'y' for regular calibration, 'm' for manual calibration, or 'n' to skip.");
      }
    }
  }
}

// Function to read and average the load cell values
// void readLoadCell(char loadCellID, int samples = 5, int delayBetweenSamples = 10) {
//   float total = 0.0;
//   for (int i = 0; i < samples; i++) {
//       if (loadCellID == 'F') {
//           LoadCell_F.update();
//           total += LoadCell_F.getData();
//       } else if (loadCellID == 'H') {
//           LoadCell_H.update();
//           total += LoadCell_H.getData();
//       } else {
//           Serial.println("Invalid Load Cell Selection!");
//           return; // Error return value
//       }
//       delay(delayBetweenSamples); // Short delay between samples
//   }
//   forceread = total/samples;
//   return forceread; // Return averaged force
// }
float readLoadCell(char loadCellID, int samples = 5, int delayBetweenSamples = 10) {
  float total = 0.0;

  for (int i = 0; i < samples; i++) {
      if (loadCellID == 'F') {
          LoadCell_F.update();
          total += LoadCell_F.getData();
      } else if (loadCellID == 'H') {
          LoadCell_H.update();
          total += LoadCell_H.getData();
      } else {
          Serial.println("Invalid Load Cell Selection!");
          return -1.0; // Return an error value
      }
      delay(delayBetweenSamples);
  }

  float forceread = total / samples;  // Compute the average
  return forceread; // Return the averaged force value
}

// Function to move a stepper by one microstep
void stepMotor(int speedMicroseconds, bool direction, int microsteps, char motor) {
  int dirPin, pulPin;
  
  // Select correct stepper motor
  if (motor == 'F') {
      dirPin = DIR_F;
      pulPin = PUL_F;
  } else if (motor == 'H') {
      dirPin = DIR_H;
      pulPin = PUL_H;
  } else {
      Serial.println("Invalid motor selection!");
      return;
  }

  digitalWrite(dirPin, direction); // Set direction

  // Loop for the correct number of microsteps
  for (int i = 0; i < microsteps; i++) {
      digitalWrite(pulPin, HIGH);
      delayMicroseconds(speedMicroseconds);
      digitalWrite(pulPin, LOW);
      delayMicroseconds(speedMicroseconds);
  }
}

//#### RUN ONCE SETUP ####

void setup() {
  Serial.begin(57600); // Start Serial Monitor for debugging
  Serial.println("Starting...");
  
  // Set stepper driver pins as OUTPUT's
  pinMode(DIR_F, OUTPUT);
  pinMode(PUL_F, OUTPUT);
  pinMode(DIR_H, OUTPUT);
  pinMode(PUL_H, OUTPUT);
  pinMode(ENA_F, OUTPUT);
  pinMode(ENA_H, OUTPUT);

  // Set Enable pins to low to enable the stepper driver's
  digitalWrite(ENA_F, LOW);
  digitalWrite(ENA_H, LOW);

  Serial.println("Stepper Motors & Drivers Initialized.");

  //#### LOAD CELL CALIBRATION & START-UP ####

  Serial.println("Do you want to recalibrate the Forefoot load cell? (y: Yes auto (known weight), m: Manual, n: No)");

  // Forefoot load cell calibration
  calibrateLoadCell(LoadCell_F, calVal_eepromAdress_F); 

  Serial.println("Do you want to recalibrate the Heel load cell? (y: Yes auto (known weight), m: Manual, n: No)");

  // Heel load cell calibration
  calibrateLoadCell(LoadCell_H, calVal_eepromAdress_H);

    // Ask user to start the test or not
  Serial.println("Do you want to start the test? (y: Yes, n: No)");
  
  // Wait for user input (y: Yes, n: No)
  while (true) {
    if (Serial.available() > 0) {
      char response = Serial.read(); // Read the user input
      if (response == 'y' || response == 'Y') {
        Serial.println("Starting test...");
        Serial.println("! CAUTION: ACUTATOR MOTION !");
        Serial.println("***");
        break; // Break the loop and start motor movement
      } else if (response == 'n' || response == 'N') {
        Serial.println("Test aborted.");
        Serial.println("***");
      // Exit setup, no motor movement
      } else {
        // If the input is invalid, ask the user again
        Serial.println("Invalid input. Please enter 'y' to start or 'n' to abort.");
        }
      }
    }

}

//#### INFINITE LOOP ####

void loop() {

  int stepCount_F = 0;
  int stepCount_H = 0;
  int cycleCount = 0;  // Reset cycle count
  
  // Loop to perform motor movement for the set number of cycles
  while (cycleCount < maxCycles) {

    // Move Forefoot Motor until target force is reached
    Serial.println("Moving Forefoot Motor...");
    while (true) {
        float force = readLoadCell('F'); // Get averaged force
        Serial.print("Force (F): ");
        Serial.println(force);

        if (force >= targetForce) {
            Serial.println("Target force reached!");
            break;
        }

        stepMotor(stepDelay, HIGH, microstepSetting, 'F'); // Move Forefoot Motor
        stepCount_F++;  // Increment Forefoot step count
    }

    delay(1000); // Pause before moving back

    // Move back same number of steps for Forefoot Motor
    Serial.println("Returning Forefoot Motor...");
    for (int i = 0; i < stepCount_F; i++) {
        stepMotor(stepDelay, LOW, microstepSetting, 'F');
    }

    Serial.println("Forefoot Motor Back to Start.");
    stepCount_F = 0;  // Reset step count after cycle

    delay(2000); // Pause before next cycle

    // Move Heel Motor until target force is reached
    Serial.println("Moving Heel Motor...");
    while (true) {
        float force = readLoadCell('H'); // Get averaged force
        Serial.print("Force (H): ");
        Serial.println(force);

        if (force >= targetForce) {
            Serial.println("Target force reached!");
            break;
        }

        stepMotor(stepDelay, HIGH, microstepSetting, 'H'); // Move Heel Motor
        stepCount_H++;  // Increment Heel step count
    }

    delay(1000);

    // Move back same number of steps for Heel Motor
    Serial.println("Returning Heel Motor...");
    for (int i = 0; i < stepCount_H; i++) {
        stepMotor(stepDelay, LOW, microstepSetting, 'H');
    }

    Serial.println("Heel Motor Back to Start.");
    stepCount_H = 0;  // Reset step count after cycle
    
    delay(2000); // Pause before next cycle

    // Increment the cycle count after both motors have completed the cycle
    cycleCount++; 
    Serial.print("Cycle count: ");
    Serial.println(cycleCount);

    // Check if the set number of cycles has been reached
    if (cycleCount >= maxCycles) {
        Serial.println("Test completed. Max cycles reached: ");
        Serial.print(maxCycles);
        Serial.println(" Cycles");
        Serial.println("***");
        break;  // Exit the loop after completing the desired number of cycles
    }
  }

  //#### LEGACY - FIRST TEST ####
  // digitalWrite(DIR_F,HIGH); // Enables the motor to move in a particular direction
  // for (int i = 0; i < 6; i++) {  // 48 times = one revolution
  //   for (int x = 0; x < 3200; x++) { // 3200 pulse/rev
  //     digitalWrite(PUL_F, HIGH);
  //     delayMicroseconds(60);
  //     digitalWrite(PUL_F, LOW);
  //     delayMicroseconds(60);
  // }
  // }
  // delay(500); 
  // digitalWrite(DIR_F,LOW); // Enables the motor to move in a particular direction
  // for (int i = 0; i < 6; i++) {  // 48 times = one revolution
  //   for (int x = 0; x < 3200; x++) { // 3200 pulse/rev
  //     digitalWrite(PUL_F, HIGH);
  //     delayMicroseconds(60);
  //     digitalWrite(PUL_F, LOW);
  //     delayMicroseconds(60);
  // }
  // }
  //   delay(500);
  //     digitalWrite(DIR_H,HIGH); // Enables the motor to move in a particular direction
  // for (int i = 0; i < 24; i++) {  // 48 times = one revolution
  //   for (int x = 0; x < 3200; x++) { // 3200 pulse/rev
  //     digitalWrite(PUL_H, HIGH);
  //     delayMicroseconds(60);
  //     digitalWrite(PUL_H, LOW);
  //     delayMicroseconds(60);
  // }
  // }
  // delay(500); 
  // digitalWrite(DIR_H,LOW); // Enables the motor to move in a particular direction
  // for (int i = 0; i < 24; i++) {  // 48 times = one revolution
  //   for (int x = 0; x < 3200; x++) { // 3200 pulse/rev
  //     digitalWrite(PUL_H, HIGH);
  //     delayMicroseconds(60);
  //     digitalWrite(PUL_H, LOW);
  //     delayMicroseconds(60);
  // }
  // }
  // delay(500);
}