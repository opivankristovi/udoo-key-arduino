const int blueLedPin  = 32;  // Blue on-board LED
const int yellowLedPin = 33; // Yellow on-board LED

void setup() {
  pinMode(blueLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
}

void loop() {
  // Alternate blue and yellow LEDs
  digitalWrite(blueLedPin, HIGH);
  digitalWrite(yellowLedPin, LOW);
  delay(500);
  digitalWrite(blueLedPin, LOW);
  digitalWrite(yellowLedPin, HIGH);
  delay(500);
}
