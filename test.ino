#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL343.h>
#include "Adafruit_APDS9960.h"

Adafruit_APDS9960 apds;
Adafruit_ADXL343 accel = Adafruit_ADXL343(12345);

#define INPUT_PIN_INT1   (4)
#define ADXL343_SCK 13
#define ADXL343_MISO 12
#define ADXL343_MOSI 11
#define ADXL343_CS 10

uint32_t g_tap_count = 0;
int_config g_int_config_enabled = { 0 };
int_config g_int_config_map = { 0 };

const int headTouchPin = 0;
const int cheekTouchPin = 17;
const int eyeTouchPin = 29;
const int PressureInPin1 = A7; 
const int PressureInPin2 = A16;
float fear;

int pressureValue1 = 0; 
int pressureValue2 = 0;

float valence = 0;
float arousal = 0;
float prev_valence = 0; // initialize previous valence to 0
float prev_arousal = 0; // initialize previous arousal to 0

unsigned long lastChangeTime = millis();

void int1_isr(void) {
    g_tap_count++;
}

void setup() {
  Serial.begin(115200);

  apds.begin();
  apds.enableProximity(true);

  accel.begin();
  accel.setRange(ADXL343_RANGE_16_G);

  pinMode(INPUT_PIN_INT1, INPUT);
  attachInterrupt(digitalPinToInterrupt(INPUT_PIN_INT1), int1_isr, RISING);
  g_int_config_enabled.bits.single_tap = true;
  accel.enableInterrupts(g_int_config_enabled);
  g_int_config_map.bits.single_tap = ADXL343_INT1;
  accel.mapInterrupts(g_int_config_map);
  
  g_tap_count = 0;


}

void loop() {

  static uint32_t start_time = 0;
  static bool exceeded = false;
  static int previous_z_value = 0;
  int current_z_value = 0;

  sensors_event_t event;
  accel.getEvent(&event);
  delay(50);
  //Serial.print("Proximity: ");
  //Serial.println(apds.readProximity());

  delay(50);

  int proximity = apds.readProximity();
  if (proximity >= 15){
    Serial.println("Scared");
    if (valence <= -0.1 && valence >= -0.2){
      valence -= 0.0005;
    }
    else if (valence < -0.1){
      valence += 0.2;
    }
    else{
      valence -= 0.2;
    }
    arousal += 0.2;
  }

  while (g_tap_count) {
      Serial.println("Single tap detected!");
      /* Clear the interrupt as a side-effect of reading the interrupt source register.. */
      accel.checkInterrupts();
      /* Decrement the local interrupt counter. */
      g_tap_count--;
  }

  //Touch sensor

  //head_touch_sensor
  int headTouchValue = touchRead(headTouchPin);
  //Serial.print("Head Touch: ");
  //Serial.println(headTouchValue);

  //cheek_touch_sensor
  int cheekTouchValue = touchRead(cheekTouchPin);
  //Serial.print("Cheek Touch: ");
  //Serial.println(cheekTouchValue);

  //eye_touch_sensor
  int eyeTouchValue = touchRead(eyeTouchPin);
  //Serial.print("under eye Touch: ");
  //Serial.println(eyeTouchValue);

  delay(50);

  if (headTouchValue > 2000) {
    if (!exceeded) {
      start_time = millis();
      exceeded = true;
    } else if (millis() - start_time> 500) {
      Serial.println("Relaxed");
      valence += 0.2;
      arousal += -0.2;
    }
  } 
  else if(cheekTouchValue > 2000) {
    if (!exceeded) {
      start_time = millis();
      exceeded = true;
    } 
    else if (millis() - start_time> 500) {
      Serial.println("Affectionate");
      valence += 0.2;
      if (arousal >= 0 && arousal <=0.1){
        arousal -= 0.0005;
      }
      else if (arousal > 0.1) {
        arousal -= 0.2;
      }
      else{
        arousal += 0.2;
      }
    }
  } 
  else if (eyeTouchValue > 1900) {
    if (!exceeded) {
        start_time = millis();
        exceeded = true;
    } 
    else if (millis() - start_time > 500) {
        Serial.println("Upset");
        valence -= 0.2;
        if (arousal <= 0 && arousal > -0.1) {
            arousal -= 0.0005;
        } 
        else if (arousal < -0.1) {
            arousal += 0.2;
        } 
        else{
          arousal -= 0.2;
        }
    } 
  } 
  else {
    exceeded = false;
  }


  //Pressure sensor
  pressureValue1 = analogRead(PressureInPin1);
  pressureValue2 = analogRead(PressureInPin2);
  //Serial.print("Pressure: ");
  //Serial.println(pressureValue1);
  if (pressureValue1 < 10 || pressureValue2 < 90){
    Serial.println("Angry");
    valence += -0.2;
    arousal += 0.2;
  }

  //Accelerometer
  current_z_value = event.acceleration.z;
  //Serial.print("Z: "); Serial.print(current_z_value); Serial.println(" counts");

  // Check if the Z value has alternated
  if ((previous_z_value < 9 && current_z_value >= 9) || (previous_z_value >= 9 && current_z_value < 9)) {
    Serial.println("Excited");
    if (valence > 0.1){
      valence -= 0.2;
    }
    else if (valence >= 0 && valence <=0.1){
      valence += 0.0005;
    }
    else{
      valence += 0.2;
    }
    arousal += 0.2;

  }

  previous_z_value = current_z_value;


  //Arousal/Valence value
  float x_norm = valence;
  float y_norm = arousal;

  // limit x and y to the range [-1, 1]
  if (x_norm > 1.0) {
    x_norm = 1.0;
    valence = 1.0;
  } else if (x_norm < -1.0) {
    x_norm = -1.0;
    valence = -1.0;
  }
  if (y_norm > 1.0) {
    y_norm = 1.0;
    arousal = 1.0;
  } else if (y_norm < -1.0) {
    y_norm = -1.0;
    arousal = -1.0;
  }

  if (prev_valence != valence || prev_arousal != arousal) {
    // Update variables
    prev_valence = valence;
    prev_arousal = arousal;

    lastChangeTime = millis();
  }
  // print the coordinates to the serial monitor
  Serial.print("x = ");
  Serial.print(x_norm);
  Serial.print(", y = ");
  Serial.println(y_norm);
  
  if (millis() - lastChangeTime > 3000) {
    // Reset variables to 0
    valence = 0;
    arousal = 0;

    // Update last change time
    lastChangeTime = millis();

    // Print reset message to serial monitor
    Serial.print("x = ");
    Serial.print(valence);
    Serial.print(", y = ");
    Serial.println(arousal);
  }
  

  delay(500);

}
