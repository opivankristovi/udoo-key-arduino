void setup() {
  Serial.begin(115200);
  delay(750);
}

void loop() {
  Serial.printf("Core temperature: %2.1fC\n", analogReadTemp());
  delay(1000);
}
