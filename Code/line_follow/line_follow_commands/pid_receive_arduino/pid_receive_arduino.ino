#include <Servo.h>
#include <SoftwareSerial.h>

// Motor control definitions


#define FORWARD 'w'
#define LEFT 'a'
#define RIGHT 'd'
#define BACK 's'
#define STOP 'x'

// Pin definitions
// MOTOR PINS
const int leftPWM = 5;
const int rightPWM = 6;
const int leftIN1 = 7;
const int leftIN2 = 8;
const int rightIN1 = 9;
const int rightIN2 = 11;

// ULTRASONIC SENSOR PINS (CHANGED from 7,8 to avoid conflict)
const int trigPin = 10;   // Changed from 7
const int echoPin = 13;   // Changed from 8
const long OBST_IN = 5;

// SERVO PIN
const int servoPin = 12;  // Changed from 9 to avoid motor conflict
Servo MG90SLeft;
Servo MG90SRight;

// SPEAKER/BUZZER PIN
// const int buzzerPin = 4;
const int startingServoPos = 0;  // Servo starts at 0 degrees (closed/up position)
const int deployedServoPos = 105;  // Servo deploys to 90 degrees (open/down position)

// LED PINS (assuming RGB LED or separate LEDs)
const int ledRedPin = 1;
const int ledWhitePin = 2;

// State variables
bool rampDeployed = false;
bool stopSignPassed = false;
SoftwareSerial mp3(4, 3);
const uint8_t track1[] = {0x7E, 0x01, 0x00, 0x02, 0x00, 0x01, 0xEF};
const uint8_t track2[] = {0x7E, 0x01, 0x00, 0x02, 0x00, 0x02, 0xEF};
const uint8_t track3[] = {0x7E, 0x01, 0x00, 0x02, 0x00, 0x03, 0xEF};

void Set_Speed(int Left, int Right) {
  int topspeed = 255;
  if (Left > topspeed) Left = topspeed;
  if (Right > topspeed) Right = topspeed;
  
  analogWrite(leftPWM, Left);
  analogWrite(rightPWM, Right);
}

void forward() {
  digitalWrite(leftIN1, HIGH);
  digitalWrite(leftIN2, LOW);
  digitalWrite(rightIN1, LOW);
  digitalWrite(rightIN2, HIGH);
}

void back() {
  digitalWrite(leftIN1, LOW);
  digitalWrite(leftIN2, HIGH);
  digitalWrite(rightIN1, HIGH);
  digitalWrite(rightIN2, LOW);
}

void left() {
  digitalWrite(leftIN1, LOW);
  digitalWrite(leftIN2, HIGH);
  digitalWrite(rightIN1, LOW);
  digitalWrite(rightIN2, HIGH);
}

void right() {
  digitalWrite(leftIN1, HIGH);
  digitalWrite(leftIN2, LOW);
  digitalWrite(rightIN1, HIGH);
  digitalWrite(rightIN2, LOW);
}

void stopcar() {
  analogWrite(leftPWM, 0);
  analogWrite(rightPWM, 0);
}

long readUltrasonicInches() {
  digitalWrite(trigPin, LOW);
  // delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  // delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  unsigned long duration = pulseIn(echoPin, HIGH, 3000); // 30ms timeout
  if (duration == 0) return 9999; // No echo detected
  
  long inches = duration / 74 / 2;
  return inches;
}

void deployRamp() {
  Serial.println("Deploying ramp");
  if (rampDeployed) return; // Already deployed
  
  // Move servo down (deploy ramp)
    Serial.print("Moving servo to angle: ");
    Serial.println(deployedServoPos);
    for (int angle = startingServoPos; angle <= deployedServoPos; angle+=15) {
      Serial.print("Moving servo to angle: ");
      Serial.println(angle);
      MG90SLeft.write(angle);
      delay(15);
    }
  
  
  rampDeployed = true;
}

void closeRamp() {
  if (!rampDeployed) return; // Already closed
  Serial.println("Closing ramp");
  
  // Move servo back up
  Serial.print("Moving servo to angle: ");
  Serial.print(startingServoPos);

  for (int angle = deployedServoPos; angle >= startingServoPos; angle-=15) {
    MG90SLeft.write(angle);
    delay(15);
  }
  MG90SLeft.write(startingServoPos);

  rampDeployed = false;
}

void startSpeaker(const uint8_t *cmd, size_t len) {
  Serial.println("Starting speaker");
  // const int speakerPin = 8; //Pin connector for the speaker (could change)
  // const int frequency = 1000; // sound in Hz
  for(size_t i = 0; i < len; i++) {
    mp3.write(cmd[i]);
  }

  // tone(speakerPin, frequency);  //speaker continues to play
}
void setVolume(uint8_t level) {
  if (level > 30) level = 10;              // valid range 0–30
  uint8_t cmd[] = {0x7E, 0x06, 0x00, 0x02, 0x00, level, 0xEF};
  startSpeaker(cmd, sizeof(cmd));
}

void stopSpeaker() {
  Serial.println("Stopping speaker");
  // noTone(speakerPin);  //stops sound
  
}

void startLED() {
  Serial.println("Starting LED");
  digitalWrite(ledRedPin, HIGH);
  // delay(1000); 
  digitalWrite(ledWhitePin, LOW);
}

void stopLED() {
  Serial.println("Stopping LED");
  digitalWrite(ledRedPin, LOW);
  digitalWrite(ledWhitePin, LOW);
}

void process_direction(char direction) {
  switch(direction) {
    case FORWARD:
      forward();
      break;
    case LEFT:
      left();
      break;
    case RIGHT:
      right();
      break;
    case BACK:
      back();
      break;
    case STOP:
      stopcar();
      break;
  }
}

void setup() {
  // Motor pins
  pinMode(leftPWM, OUTPUT);
  pinMode(rightPWM, OUTPUT);
  pinMode(leftIN1, OUTPUT);
  pinMode(leftIN2, OUTPUT);
  pinMode(rightIN1, OUTPUT);
  pinMode(rightIN2, OUTPUT);

  // Ultrasonic pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Servo
  MG90SLeft.attach(servoPin);

  // Buzzer
  // pinMode(buzzerPin, OUTPUT);
  
  mp3.begin(9600);
  
  startSpeaker(track1, sizeof(track1));
  
  // mp3.listen();

  // LEDs
  // pinMode(ledRedPin, OUTPUT);
  // pinMode(ledWhitePin, OUTPUT);

  // Serial communication with ESP32
  Serial.begin(115200);

  // Initialize states
  stopcar();
  MG90SLeft.write(startingServoPos);  // Set servo to 0 degrees (closed position)
  // delay(1500);  // Give servo time to reach initial position
  rampDeployed = false;  // Ensure state is reset
}

void loop() {
  // SAFETY FIRST: Check ultrasonic EVERY loop iteration
  long inches = readUltrasonicInches();
  
  
  if(inches < OBST_IN) {
    stopcar();
    
    if (!rampDeployed) {
      Serial.println("PERSON DETECTED: Deploying ramp");
      deployRamp();
      startSpeaker(track1, sizeof(track1));
      // stopLED();
      
      delay(8000);
      
      closeRamp();
      
      stopSpeaker();
      
      Serial.println("Ramp sequence complete");
      startSpeaker(track3, sizeof(track2));
    }
    
    // delay(100);
    return;  // Exit early, don't process commands while obstacle detected
  }
  
  // Process incoming PID commands from ESP32
  while (Serial.available()) {
    String pid_command = Serial.readStringUntil('\n');
    pid_command.trim();

    // Handle STOP command
    if (pid_command == "STOP" && !stopSignPassed) {
      stopcar();
      delay(3000);
      stopSignPassed = true;
      continue;
    }

    // Parse PID command: "leftSpeed,rightSpeed,direction"
    int firstCommaIndex = pid_command.indexOf(',');
    int secondCommaIndex = pid_command.indexOf(',', firstCommaIndex + 1);

    if (firstCommaIndex > 0 && secondCommaIndex > firstCommaIndex) {
      int leftSpeed = pid_command.substring(0, firstCommaIndex).toInt();
      int rightSpeed = pid_command.substring(firstCommaIndex + 1, secondCommaIndex).toInt();
      char direction = pid_command.substring(secondCommaIndex + 1)[0];

      process_direction(direction);
      Set_Speed(leftSpeed, rightSpeed);
    }
  }
  
  // delay(10);
}
