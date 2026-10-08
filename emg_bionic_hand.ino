#include <Servo.h>

Servo myServo;

const int emgPin = A0;
const int servoPin = 9;

const unsigned long calibrationTime = 10000;
const unsigned long logInterval = 50;

int relaxedMin = 1023;
int relaxedMax = 0;
int flexMax = 0;
int adaptiveThreshold;

bool controlMode = false;
float smoothedValue = 0;
const float alpha = 0.1;

int currentServoPos = 180;
int targetServoPos = 180;

unsigned long lastLogTime = 0;
String currentLevel = "RELAXED";

void setup() {

Serial.begin(9600);
myServo.attach(servoPin);
myServo.write(180);

delay(2000);

Serial.println("=================================");
Serial.println("STARTING CALIBRATION");
Serial.println("=================================");

servoDoubleClose();
Serial.println(">>> RELAX for 10 seconds");
recordRelaxedPhase();

servoDoubleClose();
Serial.println(">>> FLEX MAX for 10 seconds");
recordFlexPhase();

servoDoubleClose();
calculateThreshold();

Serial.println("=================================");
Serial.println("CALIBRATION COMPLETE");
Serial.println("Beginning Live Output...");
Serial.println("=================================");

Serial.println("LEVEL,EMG,SERVO_ANGLE");

 controlMode = true;
}

void loop() {

if (!controlMode) return;

int rawValue = readAveragedEMG();

smoothedValue = (alpha * rawValue) + ((1 - alpha) * smoothedValue);
int emgValue = (int)smoothedValue;

int range = flexMax - adaptiveThreshold;
int lightCutoff = adaptiveThreshold + (range * 0.33);
int mediumCutoff = adaptiveThreshold + (range * 0.66);

if (emgValue < adaptiveThreshold) {
 currentLevel = "RELAXED";
 targetServoPos = 180;
}
else if (emgValue < lightCutoff) {
 currentLevel = "LIGHT";
}
else if (emgValue < mediumCutoff) {
 currentLevel = "MEDIUM";
}
else {
 currentLevel = "STRONG";
}

if (emgValue >= adaptiveThreshold) {
 targetServoPos = map(emgValue,
 adaptiveThreshold,
 flexMax,
 180,
 0);
 targetServoPos = constrain(targetServoPos, 0, 180);
}

if (currentServoPos < targetServoPos) currentServoPos++;
if (currentServoPos > targetServoPos) currentServoPos--;

myServo.write(currentServoPos);

if (millis() - lastLogTime >= logInterval) {
 lastLogTime = millis();

 Serial.print(currentLevel);
 Serial.print(",");
 Serial.print(emgValue);
 Serial.print(",");
 Serial.println(currentServoPos);
}
}

void servoDoubleClose() {
for (int index = 0; index < 2; index++) {
 myServo.write(0);
 delay(500);
 myServo.write(180);
 delay(500);
}
}

void recordRelaxedPhase() {

unsigned long startTime = millis();
unsigned long lastPrint = 0;
int lastSecond = -1;

while (millis() - startTime <= calibrationTime) {

 int value = analogRead(emgPin);

 if (value < relaxedMin) relaxedMin = value;
 if (value > relaxedMax) relaxedMax = value;

 int secondsLeft = 10 - ((millis() - startTime) / 1000);
 if (secondsLeft != lastSecond) {
  lastSecond = secondsLeft;
  Serial.print("Relax Time Left: ");
  Serial.println(secondsLeft);
 }

 if (millis() - lastPrint >= 200) {
  lastPrint = millis();
  Serial.print("Relax EMG: ");
  Serial.println(value);
 }
}

Serial.print("Relaxed Min: ");
Serial.println(relaxedMin);
Serial.print("Relaxed Max: ");
Serial.println(relaxedMax);
}

void recordFlexPhase() {

unsigned long startTime = millis();
unsigned long lastPrint = 0;
int lastSecond = -1;

while (millis() - startTime <= calibrationTime) {

 int value = analogRead(emgPin);

 if (value > flexMax) flexMax = value;

 int secondsLeft = 10 - ((millis() - startTime) / 1000);
 if (secondsLeft != lastSecond) {
  lastSecond = secondsLeft;
  Serial.print("Flex Time Left: ");
  Serial.println(secondsLeft);
 }

 if (millis() - lastPrint >= 200) {
  lastPrint = millis();
  Serial.print("Flex EMG: ");
  Serial.println(value);
 }
}

Serial.print("Flex Max: ");
Serial.println(flexMax);
}

void calculateThreshold() {

int noiseRange = relaxedMax - relaxedMin;
adaptiveThreshold = relaxedMax + (noiseRange * 2);

Serial.println("---- Calibration Results ----");
Serial.print("Noise Range: ");
Serial.println(noiseRange);
Serial.print("Adaptive Threshold: ");
Serial.println(adaptiveThreshold);
Serial.print("Flex Max: ");
Serial.println(flexMax);
}

int readAveragedEMG() {
int aggregate = 0;
for (int index = 0; index < 5; index++) {
 aggregate += analogRead(emgPin);
}
return aggregate / 5;
}
