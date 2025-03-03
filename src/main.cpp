/*
 * Project: GDP03_MAIN
 * Description: This code initalises both stepper motors and driver boards,
 * then turns the forefoot one to list and lower the force loading carriage,
 * and roates the heel one by half a roation (180 deg).
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

#include <Arduino.h>

//##### DEFINE PINOUT ####

// Define pins for Forefoot Stepper Motor
const int DIR_F = 35; // Direction pin for Forefoot stepper driver
const int PUL_F = 34; // Pulse pin for Forefoot stepper driver
const int ENA_F = 53; // Enable pin for Forefoot stepper driver

// Define pins for Heel Stepper Motor
const int DIR_H = 37; // Direction pin for Heel stepper driver
const int PUL_H = 36; // Pulse pin for Heel stepper driver
const int ENA_H = 51; // Enable pin for Heel stepper driver

//#### INITALISE PINS AND HARDWARE ####

void setup() {
  Serial.begin(9600); // Start Serial Monitor for debugging
  
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

  Serial.println("Stepper motors & drivers initialized.");
}

//#### INFINITE LOOP ####

void loop() {

  digitalWrite(DIR_F,HIGH); // Enables the motor to move in a particular direction
  for (int i = 0; i < 6; i++) {  // 48 times = one revolution
    for (int x = 0; x < 3200; x++) { // 3200 pulse/rev
      digitalWrite(PUL_F, HIGH);
      delayMicroseconds(60);
      digitalWrite(PUL_F, LOW);
      delayMicroseconds(60);
  }
  }
  delay(500); 
  digitalWrite(DIR_F,LOW); // Enables the motor to move in a particular direction
  for (int i = 0; i < 6; i++) {  // 48 times = one revolution
    for (int x = 0; x < 3200; x++) { // 3200 pulse/rev
      digitalWrite(PUL_F, HIGH);
      delayMicroseconds(60);
      digitalWrite(PUL_F, LOW);
      delayMicroseconds(60);
  }
  }
    delay(500);
      digitalWrite(DIR_H,HIGH); // Enables the motor to move in a particular direction
  for (int i = 0; i < 24; i++) {  // 48 times = one revolution
    for (int x = 0; x < 3200; x++) { // 3200 pulse/rev
      digitalWrite(PUL_H, HIGH);
      delayMicroseconds(60);
      digitalWrite(PUL_H, LOW);
      delayMicroseconds(60);
  }
  }
  delay(500); 
  digitalWrite(DIR_H,LOW); // Enables the motor to move in a particular direction
  for (int i = 0; i < 24; i++) {  // 48 times = one revolution
    for (int x = 0; x < 3200; x++) { // 3200 pulse/rev
      digitalWrite(PUL_H, HIGH);
      delayMicroseconds(60);
      digitalWrite(PUL_H, LOW);
      delayMicroseconds(60);
  }
  }
  delay(500);
  
}