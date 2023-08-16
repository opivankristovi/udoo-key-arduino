#include <I2S.h>

const int DOpin = 25;     //the GPIO pin connected to Data Out
const int CLKpin = 27;    //GPIO pin connected to Clock Pin

void setup() {
  // Open serial communications and wait for port to open:
  // A baud rate of 115200 is used instead of 9600 for a faster data rate
  // on non-native USB ports
  I2S.setDataPin(DOpin);
  I2S.setSckPin(CLKpin);
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // start I2S at 8 kHz with 32-bits per sample
  if (!I2S.begin(esp_i2s::I2S_MODE_MASTER, 8000, 32)) {
    Serial.println("Failed to initialize I2S!");
    while (1); // do nothing
  }
}

void loop() {
  // read a sample
  int sample = I2S.read();

  //Print sample value to serial monitor
  Serial.print("Val: ");
  Serial.println(sample);
  delay(10);
}
