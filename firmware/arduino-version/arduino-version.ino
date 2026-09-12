#include <math.h>

#define DIR_PIN 2
#define STEP_PIN 3
#define SLEEP_RESET_PIN 4

#define MS1_PIN 5
#define MS2_PIN 6
#define MS3_PIN 7

// Heater control pins
const int R_EN = 10;
const int L_EN = 11;
const int LPWM = 12;
const int RPWM = 13;
const int thermistorPin = A7;

// Thermistor constants
const float seriesResistor = 100000.0;
const float nominalResistance = 100000.0;
const float nominalTemperature = 25.0;
const float betaCoefficient = 3950.0;
const int adcMax = 1023;
const float vcc = 5.0;

// Target temperature
const float targetTemp = 245.0;

// PID Constants
float Kp = 5.0;
float Ki = 0.1;
float Kd = 30.0;

// PID Variables
float previousError = 0;
float integral = 0;
unsigned long lastTime = 0;

// Stepper Speed
int stepsPerRevolution = 400; // Updated by microstepping
int motorSpeed = 1000;        // Initial speed (microseconds)
const int MIN_MOTOR_DELAY = 200; // Fastest allowed step time (200µs = 5k steps/sec)
const int ACCELERATION_STEP = 5; // How much speed increases every loop

void setup() {
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(SLEEP_RESET_PIN, OUTPUT);

  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  pinMode(MS3_PIN, OUTPUT);

  Serial.begin(9600);

  setMicrostepping(2);      // Change microstepping as needed
  digitalWrite(SLEEP_RESET_PIN, HIGH); // Wake driver
  
  // Reverse motor direction
  digitalWrite(DIR_PIN, HIGH);         // Changed from HIGH to LOW to reverse direction

  Serial.println("Setup complete. Driver awake, motor starting...");

  heaterOff();
  lastTime = millis();
}

void loop() {
  float temperature = readingTemperature();

  // Accelerate the motor
  if (motorSpeed > MIN_MOTOR_DELAY) {
    motorSpeed -= ACCELERATION_STEP;
  }

  // Step the motor
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(motorSpeed);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(motorSpeed);

  // Temperature feedback and control
  pidControl(temperature);
}

// ---------------- HEATER CONTROL ----------------

void pidControl(float temp) {
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - lastTime) / 1000.0;

  if (temp == -273.15 || temp > 330) {
    heaterOff();
    lastTime = currentTime;
    return;
  }

  float error = targetTemp - temp;

  if (abs(error) < 1.0) {
    Serial.print("Temp stable: ");
    Serial.print(temp);
    Serial.println(" °C");
  } else {
    integral += error * deltaTime;
    integral = constrain(integral, -100, 100);

    float derivative = (error - previousError) / deltaTime;
    float output = Kp * error + Ki * integral + Kd * derivative;

    output = constrain(output, 0, 235);
    setHeaterPower((int)output);

    previousError = error;
  }

  lastTime = currentTime;
}

float readingTemperature() {
  int adcValue = analogRead(thermistorPin);
  float voltage = adcValue * vcc / adcMax;

  if (voltage <= 0) return -273.15;

  float resistance = seriesResistor * (vcc / voltage - 1.0);
  float temp = resistance / nominalResistance;
  temp = log(temp);
  temp /= betaCoefficient;
  temp += 1.0 / (nominalTemperature + 273.15);
  temp = 1.0 / temp;
  temp -= 273.15;

  return temp;
}

void setHeaterPower(int pwmValue) {
  digitalWrite(R_EN, HIGH);
  digitalWrite(L_EN, HIGH);
  analogWrite(RPWM, 0);
  analogWrite(LPWM, pwmValue);
}

void heaterOff() {
  digitalWrite(R_EN, LOW);
  digitalWrite(L_EN, LOW);
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);
}

// ---------------- STEPPER DRIVER CONTROL ----------------

void sleepDriver() {
  digitalWrite(SLEEP_RESET_PIN, LOW);
}

void wakeDriver() {
  digitalWrite(SLEEP_RESET_PIN, HIGH);
}

void setMicrostepping(int mode) {
  Serial.print("Setting microstepping to 1/");
  Serial.println(mode);

  switch (mode) {
    case 1:
      digitalWrite(MS1_PIN, LOW);
      digitalWrite(MS2_PIN, LOW);
      digitalWrite(MS3_PIN, LOW);
      stepsPerRevolution = 200;
      break;
    case 2:
      digitalWrite(MS1_PIN, HIGH);
      digitalWrite(MS2_PIN, LOW);
      digitalWrite(MS3_PIN, LOW);
      stepsPerRevolution = 400;
      break;
    case 4:
      digitalWrite(MS1_PIN, LOW);
      digitalWrite(MS2_PIN, HIGH);
      digitalWrite(MS3_PIN, LOW);
      stepsPerRevolution = 800;
      break;
    case 8:
      digitalWrite(MS1_PIN, HIGH);
      digitalWrite(MS2_PIN, HIGH);
      digitalWrite(MS3_PIN, LOW);
      stepsPerRevolution = 1600;
      break;
    case 16:
      digitalWrite(MS1_PIN, HIGH);
      digitalWrite(MS2_PIN, HIGH);
      digitalWrite(MS3_PIN, HIGH);
      stepsPerRevolution = 3200;
      break;
    default:
      Serial.println("Invalid microstepping mode, using full step.");
      digitalWrite(MS1_PIN, LOW);
      digitalWrite(MS2_PIN, LOW);
      digitalWrite(MS3_PIN, LOW);
      stepsPerRevolution = 200;
      break;
  }
}
