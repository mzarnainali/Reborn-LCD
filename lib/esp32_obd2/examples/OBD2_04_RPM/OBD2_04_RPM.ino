#include <esp32_can.h>  // https://github.com/collin80/esp32_can and https://github.com/collin80/can_common CAN libraries
#include <esp32_obd2.h>

#define CAN_RX_PIN  33
#define CAN_TX_PIN  32
const bool useStandardAddressing = true;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.print("Starting OBD2 RPM... ");

  CAN0.setCANPins((gpio_num_t)CAN_RX_PIN, (gpio_num_t)CAN_TX_PIN);
 
  if (!OBD2.begin()) {
    Serial.println("failed!");
  }
  else {
    Serial.println("succeded");
  }

   // loop through PIDs 0 to 95, reading and printing the names of the supported PIDs
  for (int pid = 0; pid < 96; pid++) {
   // if (OBD2.pidSupported(pid)) {
      Serial.print(OBD2.pidName(pid));
      Serial.print(" = ");
      Serial.print(OBD2.pidRead(pid));
      Serial.print(" ");
      Serial.print(OBD2.pidUnits(pid));
      Serial.println();
  //  }
    delay(100);
  }
}

void loop() {
  Serial.print(OBD2.pidName(ENGINE_RPM));
  Serial.print(" = ");
  Serial.print(OBD2.pidRead(ENGINE_RPM));
  Serial.print(OBD2.pidUnits(ENGINE_RPM));
  Serial.println();
  delay(1000);
}
