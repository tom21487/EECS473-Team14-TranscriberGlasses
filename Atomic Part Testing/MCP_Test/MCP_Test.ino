/***

  Code for use of an MCP9700 Thermistor IC. Correct scaling factor and equation included.

***/

#define ADC_VREF_mV     3300.0 // in millivolt
#define ADC_RESOLUTION  4096.0
#define THERM_OFFSET    500.0
#define THERM_SCALE     0.1
#define PIN_MCP         39 // ESP32 pin GPIO36 (ADC0) connected to LM35

void setup() {
  Serial.begin(115200);
}

void loop() {
  // read the ADC value from the temperature sensor
  int adcVal = analogRead(PIN_MCP);
  Serial.println(adcVal);

  // convert the voltage to the temperature in °C
  float tempC = (adcVal - THERM_OFFS) * THERM_SCALE;

  // convert the °C to °F
  float tempF = tempC * 9 / 5 + 32;

  // print the temperature in the Serial Monitor:
  Serial.print("Temperature: ");
  Serial.print(tempC);   // print the temperature in °C
  Serial.print("°C");
  Serial.print("  <===>  "); // separator between °C and °F
  Serial.print(tempF);   // print the temperature in °F
  Serial.println("°F");

  delay(500);
}
 
