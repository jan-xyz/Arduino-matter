#include <Matter.h>
#include <MatterIlluminance.h>

// If there's no built-in button set a pin where a button is connected
#ifndef BTN_BUILTIN
#define BTN_BUILTIN PA0
#endif

MatterIlluminance matter_illuminance_sensor;

int ambientLightPin = PA4;

const float voltage = 3.3;
const float adcMax = 4095.0;

// the setup function runs once when you press reset or power the board
void setup() {
  // Start serial output
  Serial.begin(115200);

  // Start Matter
  Matter.begin();
  matter_illuminance_sensor.begin();
  pinMode(BTN_BUILTIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // Check commissioning
  if (!Matter.isDeviceCommissioned()) {
    Serial.println("Matter device is not commissioned");
    Serial.println("Commission it to your Matter hub with the manual pairing code or QR code");
    Serial.printf("Manual pairing code: %s\n", Matter.getManualPairingCode().c_str());
    Serial.printf("QR code URL: %s\n", Matter.getOnboardingQRCodeUrl().c_str());
  }
  while (!Matter.isDeviceCommissioned()) {
    delay(200);
  }

  // Connect to thread network
  Serial.println("Waiting for Thread network...");
  while (!Matter.isDeviceThreadConnected()) {
    decommission_handler();
    delay(200);
  }
  Serial.println("Connected to Thread network");

  // Wait for device to come online
  Serial.println("Waiting for Matter device discovery...");
  while (!matter_illuminance_sensor.is_online()) {
    decommission_handler();
    delay(200);
  }

  // Indicate that the setup is done
  Serial.println("Device is now online");
  digitalWrite(LED_BUILTIN, HIGH);
}

// the loop function runs over and over again forever
void loop() {
  static float current_illuminance_lux = 0.0f;
  static uint32_t last_action = 0;
  // Wait 10 seconds
  if ((last_action + 10000) < millis()) {
    last_action = millis();
    float analogLight = analogRead(ambientLightPin);
    current_illuminance_lux = analogToLux(analogLight);
    // Publish the illuminance value - you can also use 'matter_illuminance_sensor = current_illuminance_lux'
    matter_illuminance_sensor.set_measured_value_lux(current_illuminance_lux);
    Serial.printf("Current light level: %.02f lx\n", current_illuminance_lux);
  }

  // Handle the decommissioning process if requested
  decommission_handler();
}

int analogToLux(int analogValue) {
  float volts = analogValue * voltage / adcMax;
  float amps = volts / 10000.0;  // across 10,000 Ohms
  float microamps = amps * 1000000;
  float lux = microamps * 2.0;
  return lux;
}

void decommission_handler() {
  // If the button is not pressed or the device is not commissioned - return
  if (digitalRead(BTN_BUILTIN) != LOW || !Matter.isDeviceCommissioned()) {
    return;
  }
  Serial.println("button press detected");


  // Store the time when the button was first pressed
  uint32_t start_time = millis();
  // While the button is being pressed
  while (digitalRead(BTN_BUILTIN) == LOW) {
    // Calculate the elapsed time
    uint32_t elapsed_time = millis() - start_time;
    // If the button has been pressed for less than 10 seconds, continue
    if (elapsed_time < 10000u) {
      yield();
      continue;
    }

    // Blink the LED to indicate the start of the decommissioning process
    for (uint8_t i = 0u; i < 10u; i++) {
      digitalWrite(LED_BUILTIN, !(digitalRead(LED_BUILTIN)));
      delay(100);
    }

    Serial.println("Starting decommissioning process, device will reboot...");
    Serial.println();
    digitalWrite(LED_BUILTIN, LED_BUILTIN_INACTIVE);
    // This function will not return
    // The device will restart once decommissioning has finished
    Matter.decommission();
  }
}
