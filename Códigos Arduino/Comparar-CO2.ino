/* Código de programa para comparar las medidas de CO2 de los sensores CCS811 y MH-Z14A.
 *  TFG:
 *   Nodo sensor para control de calidad de aire con estimación de temperatura operativa 
 *   basado en hardware abierto.
 *  Autor: Elena Martínez Parra 45925240J.
 *  Dispositivo utilizado: NodeMCU 1.0.
 *  Velocidad de transmisión: 9600 bps.
 */
 
// Librerías para el sensor CCS811
#include <Wire.h>    // I2C library
#include "ccs811.h"  // CCS811 library

// Se define la entrada analógica del sensor MH-Z14A
#define CO2 A0

// Conexionado para ESP8266 NodeMCU: VCC a 3V3, GND a GND, SDA a D2, SCL a D1 WAK a D3 
CCS811 ccs811(D3); // WAKE on D3

// Variables para medir cada cierto tiempo
const long samplePeriod = 10000L;
long lastSampleTime = 0;

void setup() {
  // Habilitar el serial
  Serial.begin(9600);                   // Velocidad de transmisión para monitor serie

  // Habilitar I2C
  Wire.begin();

  // Habilitar CCS811
  ccs811.set_i2cdelay(50);              // Necesario para ESP8266 porque no maneja el 
  bool ok= ccs811.begin();              // estiramiento del reloj I2C correctamente
  if( !ok ) Serial.println("setup: CCS811 begin FAILED");

  // Empezar a medir
  ok= ccs811.start(CCS811_MODE_1SEC);
  if( !ok ) Serial.println("setup: CCS811 start FAILED");
}

void loop() {
  long now = millis();
  if (now > lastSampleTime + samplePeriod){
    lastSampleTime = now;
    
    // Leer datos del MH-Z14A
    uint16_t eco2, etvoc, errstat, raw;
    ccs811.read(&eco2,&etvoc,&errstat,&raw); 
    // Leer datos del CCS811
    int ppmV = readPPMV();
        
    // Imprimir los resultados de la medición en función del estado del CCS811
    if( errstat==CCS811_ERRSTAT_OK ) { 
      Serial.print("CCS811: ");
      Serial.print("eco2=");  Serial.print(eco2);     Serial.print(" ppm  ");
      Serial.print("\t");
      Serial.print("MH-Z14A: ");
      Serial.print("CO2=");  Serial.print(ppmV);     Serial.print(" ppm  ");
      Serial.println();
      delay(5000);
    } else if( errstat==CCS811_ERRSTAT_OK_NODATA ) {
      Serial.println("CCS811: waiting for (new) data");
    } else if( errstat & CCS811_ERRSTAT_I2CFAIL ) { 
      Serial.println("CCS811: I2C error");
    } else {
      Serial.print("CCS811: errstat="); Serial.print(errstat,HEX); 
      Serial.print("="); Serial.println( ccs811.errstat_str(errstat) ); 
    }
  }
  
  // Esperar
  delay(1000); 
}

// Función para leer los datos del sensor MH-Z14A conectado por señal analógica
float readPPMV(){
  float v = analogRead(CO2)*3.3/1023.0; // Convertir a voltaje la entrada A0
  float ppm = int((v-0.4)*3125.0);      // Calcular ppm con la tensión de entrada A0
  return ppm;
}
