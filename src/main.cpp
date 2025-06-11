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
const int ENA_F = 28; // Enable pin for Forefoot stepper driver
const int HX711_dout_F = 4; // HX711 Forefoot dout pin
const int HX711_sck_F = 5; // HX711 Forefoot sck pin

// Define pins for Heel Stepper Motor
const int DIR_H = 37; // Direction pin for Heel stepper driver
const int PUL_H = 36; // Pulse pin for Heel stepper driver
const int ENA_H = 29; // Enable pin for Heel stepper driver
const int HX711_dout_H = 6; // HX711 Heel dout pin
const int HX711_sck_H = 7; // HX711 Heel sck pin

// Microstepping Configuration
const int microstepSetting = 4; // 1/4 Microstepping
const int stepsPerRevolution = 200 * microstepSetting; // 800 steps per revolution
const int stepDelay_fast = 300; // Speed in microseconds 400
const int stepDelay_slow = 1000; // Speed in microseconds

// EEPROM adress for load cell calibration tare values
const int calVal_eepromAdress_F = 0; // EEPROM adress for calibration value load cell Forefoot (4 bytes)
const int calVal_eepromAdress_H = 4; // EEPROM adress for calibration value load cell Heel (4 bytes)

// HX711 constructor's (dout pin, sck pin)
HX711_ADC LoadCell_F(HX711_dout_F, HX711_sck_F); //HX711 1
HX711_ADC LoadCell_H(HX711_dout_H, HX711_sck_H); //HX711 2
unsigned long t = 0;
volatile boolean newDataReady;

// g in m/s^2
const float g = 9.81;

// Test parameters
const int maxCycles = 1000;
const int recalibrationInterval = 1001; // Recalculate steps every X cycles
const float targetForce = 1.5; 

//#### DEFINE FUNCTIONS ####

void printFloat3SF(float value) {
  // If the value is very small or very large, adjust precision accordingly
  int digitsBeforeDecimal = log10(abs(value));  // Number of digits before the decimal point
  // Calculate the number of decimal places to print
  int decimalPlaces = 3 - digitsBeforeDecimal;
  // Ensure the number is printed with 3 significant figures
  if (decimalPlaces < 0) {
    decimalPlaces = 0;  // If the number is too large, print as an integer (no decimal places)
  }
  // Print the value with the calculated decimal places
  Serial.println(value, decimalPlaces);
}

float calibrate(HX711_ADC &LoadCell, int calAddr) {
  Serial.println("Performing automatic calibration...");
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  
  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte == 't') LoadCell.tareNoDelay();
      }
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
  Serial.print(", use this as calibration value (calFactor) in your project sketch.");
  Serial.println("Save this value to EEPROM address ");
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
  Serial.println("End of auto calibration value");
  Serial.println("***");
  return(newCalibrationValue);
}

float manualCalibrationInput(HX711_ADC &LoadCell, int calAddr) {
  Serial.println("Performing manual calibration...");
  float oldCalibrationValue;  // Declare a variable to hold the saved calibration value
  EEPROM.get(calAddr, oldCalibrationValue);  // Retrieve the saved calibration value from EEPROM
  boolean _resume = false;
  Serial.println("***");
  Serial.print("Current value is: ");
  Serial.println(oldCalibrationValue);
  Serial.println("Now, send the new value from serial monitor (e.g. 14.4).");
  float newCalibrationValue;
  while (_resume == false) {
    if (Serial.available() > 0) {
      newCalibrationValue = Serial.parseFloat();
      if (newCalibrationValue != 0) {
        Serial.print("New calibration value is: ");
        Serial.println(newCalibrationValue);
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
  Serial.println("End of manual calibration value");
  Serial.println("***");
  return(newCalibrationValue);
}

float calibrateLoadCell(HX711_ADC &LoadCell, int calAddr) {
  float calibrationVal = 0;
  boolean _resume = false;
  while (_resume == false){
    if (Serial.available() > 0) {  // Check if user has typed something in Serial Monitor
      char response = Serial.read();  // Read the response character
      if (response == 'y') {  // If the user presses 'y', start the regular calibration process
        float calibrationVal = calibrate(LoadCell, calAddr);
        return(calibrationVal);
        _resume = true;
        }  // Call the calibrate function to begin the calibration process
      else if (response == 'm') {  // If the user presses 'm', start the manual calibration process
        float calibrationVal = manualCalibrationInput(LoadCell, calAddr);  // Call the manual calibration function
        return(calibrationVal);
        _resume = true; 
        } 
      else if (response == 'n') {  // If the user presses 'n', skip calibration
        Serial.println("Skipping calibration. Using saved tare offset value.");
        float savedValue;  // Declare a variable to hold the saved calibration value
        EEPROM.get(calAddr, savedValue);  // Retrieve the saved calibration value from EEPROM
        if (savedValue == 0) {
          Serial.println("Warning: Calibration value is invalid or not set. Using default tare offset value.");
          float calibrationVal = 1.0; // Set a default value or ask the user to calibrate
          Serial.print("Saved Value: ");
          Serial.println(calibrationVal);
          Serial.println("End of calibration value");
          Serial.println("***");
          return(calibrationVal);
          _resume = true;
        } 
        else {
          float calibrationVal = savedValue;
          Serial.print("Saved Value: ");
          Serial.println(calibrationVal);
          Serial.println("End of calibration.");
          Serial.println("***");
          return(calibrationVal);
          _resume = true;
        }
       }
      else {
      // If the user enters an invalid input, ask them again
      Serial.println("Invalid input. Please enter 'y' for regular calibration, 'm' for manual calibration, or 'n' to skip.");
      }
      }
    }
   }

float readLoadCell(HX711_ADC &LoadCell) {
  static boolean newDataReady = 0; // Flag to check if new data is available
  if (LoadCell.update()) newDataReady = true; // Update load cell and set flag if new data is ready
  const int serialPrintInterval = 0; // No delay between serial prints (if serial print is used)
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData(); // Issue: This declares a local `i` which shadows the static `i`
      i = abs(i); // always positive
      newDataReady = 0; // Reset the flag to indicate no new data
      // Serial.print("Load_cell output val: ");
      printFloat3SF(i); // Function to print the value of `i`
      t = millis(); // Update the timing for serial printing
      return (i / 1000) * g;
    }
  }
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

  Serial.println("Stepper Motors & Drivers Initialised.");

  // initalise load cells
  LoadCell_F.begin();
  LoadCell_H.begin();
  unsigned long stabilizingtime = 2000; // tare preciscion can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  byte loadcell_F_rdy = 0;
  byte loadcell_H_rdy = 0;
  while ((loadcell_F_rdy + loadcell_H_rdy) < 2) { //run startup, stabilization and tare, both modules simultaniously
    if (!loadcell_F_rdy) loadcell_F_rdy = LoadCell_F.startMultiple(stabilizingtime, _tare);
    if (!loadcell_H_rdy) loadcell_H_rdy = LoadCell_H.startMultiple(stabilizingtime, _tare);
  }
  if (LoadCell_F.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 no.1 wiring and pin designations");
  }
  if (LoadCell_H.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 no.2 wiring and pin designations");
  }

  Serial.println("Load Cells Initialised.");

  // //#### LOAD CELL CALIBRATION & START-UP ####

  Serial.println("Do you want to recalibrate the Forefoot load cell? (y: Yes Auto (Using a Known Mass), m: Manual, n: No)");

  float calibrationVal_F = calibrateLoadCell(LoadCell_F, calVal_eepromAdress_F);
  LoadCell_F.setCalFactor(calibrationVal_F);
  Serial.println("Forefoot Load Cell Calibrated.");

  Serial.println("Do you want to recalibrate the Heel load cell? (y: Yes auto (known weight), m: Manual, n: No)");

  float calibrationVal_H = calibrateLoadCell(LoadCell_H, calVal_eepromAdress_H);
  LoadCell_H.setCalFactor(calibrationVal_H);
  Serial.println("Heel Load Cell Calibrated.");

  // Ask user to start the test or not
  Serial.println("Do you want to start the test? (y: Yes, n: No)");
  
  // Wait for user input (y: Yes, n: No)
  while (true) {
    if (Serial.available() > 0) {
      char response = Serial.read(); // Read the user input
      if (response == 'y' || response == 'Y') {
        Serial.println("Starting test...");
        Serial.println("! CAUTION: ACUTATOR MOTION !");
        Serial.println("TEST PARAMETERS");
        Serial.println("---------------");
        Serial.println("Test Force (N) = "); 
        Serial.println(targetForce);
        Serial.println("Number of Test Cycles = "); 
        Serial.println(maxCycles);
        Serial.println("Number of Cycles Between Calibration = "); 
        Serial.println(recalibrationInterval);
        Serial.println("---------------");
        Serial.println("Test commencing in:");
        for (int i = 10; i > 0; i--) {
          Serial.print(i);  // Print the remaining time
          Serial.println("...");  // Append ellipsis for effect
          delay(1000);  // Wait for 1 second
        }
        Serial.println("Test commenced!");
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
  int cycleCount = 0;

  while (cycleCount < maxCycles) {

      // Determine if recalibration is needed
      bool recalibrate = (cycleCount == 0 || cycleCount % recalibrationInterval == 0);

      if (recalibrate) {
          Serial.println("Calibrating step counts...");

          // Forefoot Motor Calibration
          stepCount_F = 0;
          Serial.println("Moving Forefoot Motor...");
          while (true) {
              float force_F = readLoadCell(LoadCell_F);
              Serial.print("Forefoot Force (N): ");
              printFloat3SF(force_F);

              if (force_F >= targetForce) {
                  Serial.println("Target force reached!");
                  break;
              }

              stepMotor(stepDelay_slow, HIGH, stepsPerRevolution, 'F'); // Slow for calibration
              stepCount_F++;
          }

          // Read force after forward movement
          float force_F = readLoadCell(LoadCell_F);
          Serial.print("Forefoot Force After Forward Move: ");
          Serial.println(force_F);

          delay(200); // Pause before moving back

          // Move Forefoot Motor back after calibration (Fast)
          Serial.println("Returning Forefoot Motor after calibration...");
          for (int i = 0; i < stepCount_F; i++) {
              stepMotor(stepDelay_fast, LOW, stepsPerRevolution, 'F');
          }

          // Read force after backward movement
          force_F = readLoadCell(LoadCell_F);
          Serial.print("Forefoot Force After Backward Move: ");
          Serial.println(force_F);

          Serial.println("Forefoot Motor Back to Start.");

          // // Heel Motor Calibration
          // stepCount_H = 0;
          // Serial.println("Moving Heel Motor...");
          // while (true) {
          //     float force_H = readLoadCell(LoadCell_H);
          //     Serial.print("Heel Force (N): ");
          //     Serial.println(force_H);

          //     if (force_H >= targetForce) {
          //         Serial.println("Target force reached!");
          //         break;
          //     }

          //     stepMotor(stepDelay_slow, HIGH, stepsPerRevolution, 'H'); // Slow for calibration
          //     stepCount_H++;
          // }

          // Read force after forward movement
          float force_H = readLoadCell(LoadCell_H);
          Serial.print("Heel Force After Forward Move: ");
          Serial.println(force_H);

          delay(200); // Pause before moving back

          // Move Heel Motor back after calibration (Fast)
          Serial.println("Returning Heel Motor after calibration...");
          for (int i = 0; i < stepCount_H; i++) {
              stepMotor(stepDelay_fast, LOW, stepsPerRevolution, 'H');
          }

          // Read force after backward movement
          force_H = readLoadCell(LoadCell_H);
          Serial.print("Heel Force After Backward Move: ");
          Serial.println(force_H);

          Serial.println("Heel Motor Back to Start.");

          Serial.println("Calibration complete.");
      }

      // Move Forefoot Motor using stored step count (Fast)
      Serial.println("Moving Forefoot Motor using stored step count...");
      for (int i = 0; i < stepCount_F; i++) {
          stepMotor(stepDelay_fast, HIGH, stepsPerRevolution, 'F');
      }

      // Read force after forward movement
      float force_F = readLoadCell(LoadCell_F);
      Serial.print("Forefoot Force After Forward Move: ");
      Serial.println(force_F);

      delay(200); // Pause before moving back

      // Move back same number of steps for Forefoot Motor (Fast)
      Serial.println("Returning Forefoot Motor...");
      for (int i = 0; i < stepCount_F; i++) {
          stepMotor(stepDelay_fast, LOW, stepsPerRevolution, 'F');
      }

      // Read force after backward movement
      force_F = readLoadCell(LoadCell_F);
      Serial.print("Forefoot Force After Backward Move: ");
      Serial.println(force_F);

      Serial.println("Forefoot Motor Back to Start.");

      // // Move Heel Motor using stored step count (Fast)
      // Serial.println("Moving Heel Motor using stored step count...");
      // for (int i = 0; i < stepCount_H; i++) {
      //     stepMotor(stepDelay_fast, HIGH, stepsPerRevolution, 'H');
      // }

      // // Read force after forward movement
      // float force_H = readLoadCell(LoadCell_H);
      // Serial.print("Heel Force After Forward Move: ");
      // Serial.println(force_H);

      // delay(200); // Pause before moving back

      // // Move back same number of steps for Heel Motor (Fast)
      // Serial.println("Returning Heel Motor...");
      // for (int i = 0; i < stepCount_H; i++) {
      //     stepMotor(stepDelay_fast, LOW, stepsPerRevolution, 'H');
      // }

      // // Read force after backward movement
      // force_H = readLoadCell(LoadCell_H);
      // Serial.print("Heel Force After Backward Move: ");
      // Serial.println(force_H);

      // Serial.println("Heel Motor Back to Start.");

      // Increment cycle count
      cycleCount++;
      Serial.print("Cycle count: ");
      Serial.println(cycleCount);

      // Check if the set number of cycles has been reached
      if (cycleCount >= maxCycles) {
          Serial.println("Test completed. Max cycles reached: ");
          Serial.print(maxCycles);
          Serial.println(" Cycles");
          Serial.println("***");
          while (true) {} // Halt execution
      }
  }
}