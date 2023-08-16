const int ledPin =  25; //Green on-boar LED pin

void setup() {
  //Set led pin as output pin
  pinMode(ledPin, OUTPUT);
}

void loop() {
      //Turn the LED Off for 0,5s
      digitalWrite(ledPin, LOW);
      delay(500);
      //Turn the LED On for 0,5s
      digitalWrite(ledPin, HIGH);
      delay(500);
}
