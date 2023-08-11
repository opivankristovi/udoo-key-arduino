const int rxPin = 22;    // UART0 rx-pin (connected to RP2040 tx-Pin)
const int txPin = 19;    // UART0 tx-pin (connected to RP2040 rx-Pin)
const int ledPin =  32;  //Blue on-board LED pin (yellow is pin 33)

void setup() {
  //Setup the serial channel to RP2040 (Serial1 is UART0)
  //(Baudrate, Serial_mode, rx-pin, tx-pin)
  Serial1.begin(9600, SERIAL_8N1, rxPin, txPin);
  //Setup serial channel for monitor on PC via USB
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  Serial.println("Serial connections started");
}

void loop() {
  //Send serial command to RP2040 while turning the LED on/off
  Serial1.print(0);
  digitalWrite(ledPin, LOW);
  delay(500);
  Serial1.print(1);
  digitalWrite(ledPin, HIGH);
  delay(500);
}
