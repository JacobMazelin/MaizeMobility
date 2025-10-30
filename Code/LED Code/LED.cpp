
// Defines the pin that the LED is connected to on the Arduino
const in ledPin = 13 // (Number might change depending on what pin                       is used. (13 was reccommended for LED input.)

// Defines the digital pin as an output
void setup() {
    pinMode(ledPin, OUTPUT);
}

// Turns the LED on and off using HIGH and LOW voltage level
void loop() {
    digitalWrite(ledPin, HIGH);
    delay(1000);                  // Delays 1 second
    
    digitalWrite(ledPin, LOW);
    delay(1000);
}

