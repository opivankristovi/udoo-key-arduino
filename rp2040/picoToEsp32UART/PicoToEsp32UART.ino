const int rxPin = 1;    //UART rx-pin (connected to ESP32 tx-pin)
const int txPin = 0;    //UART tx-pin (connected to ESP32 rx-pin)
const int ledPin =  25; //Green on-boar LED pin

void setup() {
  //Configure the pins for UART1
  Serial1.setRX(rxPin);
  Serial1.setTX(txPin);
  //Start serial connections
  Serial1.begin(9600);
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  //Print to serial monitor via USB
  Serial.println("Serial connections started");
}

void loop() {
  if (Serial1.available()) {        //Check for available serial data
    char number = Serial1.read();   //read serial data into char variable
    //Turn on/off LED based upon received input
    if (number == '0'){
      digitalWrite(ledPin, LOW);
    }
    else if (number == '1'){
      digitalWrite(ledPin, HIGH);
    }
    //Notify and print received message to debug monitor if command is not recognized
    else {
      Serial.print("Unknown serial command: ");
      Serial.println(number);
    }
  }
}
