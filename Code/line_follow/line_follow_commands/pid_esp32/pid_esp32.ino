#include "esp_camera.h"
#include <WiFi.h>

// Select camera model
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"
#define port 5005

const char *ssid_STA = "definitelynotarouter"; //Enter the router name
const char *password_STA = "notapwd777"; //Enter the router password

IPAddress local_IP(192, 168, 50, 111); //Set the IP address of ESP32 itself
IPAddress gateway(192, 168, 50, 1); //gateway for this device is the AP/Object Detection ESP32's IP Address
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(port);

void startCameraServer();

// define PID control constants
#define K_P 0.05 //TODO: Lower this value
#define K_I 0.00
#define K_D 0.002 //10x
#define BASE_SPEED 100
#define INFLECTION_POINT 30.0

void cameraServerSetup() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);
  s->set_vflip(s, 0);        //1-Upside down, 0-No operation
  s->set_hmirror(s, 0);      //1-Reverse left and right, 0-No operation

  startCameraServer();
}

void WiFiSetup() {
  Serial.println("Connecting as station with static IP...");

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  Serial.println(WiFi.config(local_IP, gateway, subnet) ? "Ready" : "Failed!");

  boolean result = WiFi.begin(ssid_STA, password_STA);
  if(result){
    Serial.println("Ready");
    Serial.println(String("Station IP address = ") + WiFi.localIP().toString());
    Serial.println(String("MAC address = ") + WiFi.macAddress().c_str());
  }

  else{
    Serial.println("Failed!");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print(WiFi.localIP().toString());
  Serial.println("' to connect");

  server.begin(port);
  WiFi.setAutoReconnect(true);
}

float pid_control(float error, float dt) {
  static float integral = 0;
  static float lastError = 0;

  // compute error integral (error accumulation scaled by time)
  integral += error * dt;

  float derivative = (error - lastError) / dt; 

  // compute output (sum of scaled P, I, D error values)
  float output = K_P * error + K_I * integral + K_D * derivative;


  // keep last value, return computed output
  lastError = error;
  return output;
}

char determine_direction(float cte) {
  if (cte >= INFLECTION_POINT) {
    return 'a'; // turn left
  }

  else if (cte <= -INFLECTION_POINT) {
    return 'd'; // turn right
  }

  else {
    return 'w'; // go forward
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFiSetup();
  cameraServerSetup();
}

void loop() {
  // put your main code here, to run repeatedly:
  WiFiClient client = server.available();            // listen for incoming clients
  if (client) {                                     // if you get a client,
    while (client.connected()) {                    // loop while the client's connected
      if (client.available()) {                     // if there's bytes to read from the client, get input
        String raw_input = client.readStringUntil('\n'); // read in until newline character
        raw_input.trim();

        // STOP if no line is detected
        if (raw_input == "STOP") {
          Serial.println(raw_input); // send STOP command to the Arduino Nano
          continue;
        }

        float cte = raw_input.toFloat(); // convert raw_input into the actual cte float
        float dt = 0.2; // set to 0.2 for stability

        float left_factor = 1.0; // if robot leans right, increase left factor
        float right_factor = 1.1; // if robot leans left, increase right factor

        float speed_correction = pid_control(cte, 0.2); // perform PID control on received cte BEST VALUE: 0.25
        float left_side_speed = constrain(BASE_SPEED - speed_correction * left_factor, 0, 150);
        float right_side_speed = constrain(BASE_SPEED + speed_correction * right_factor, 0, 150);

        char direction = determine_direction(cte); // determine how to shift the car's movement based on CTE

        String pid_cmd = String(left_side_speed) + "," + String(right_side_speed) + "," + String(direction);
        Serial.println(pid_cmd); // send PID values to Arduino Nano
        client.println(pid_cmd); // send PID values back to laptop for debugging
      }
    }
    client.stop();                                  // stop the client connecting.
    Serial.println("Client Disconnected.");
  }
}
