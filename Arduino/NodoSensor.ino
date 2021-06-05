/* Código de programa para nodo sensor
 *  
 *  TFG:
 *   Nodo sensor para control de calidad de aire con estimación de temperatura operativa
 *   basado en hardware abierto.
 *   
 *   Autor: Elena Martínez Parra 45925240J
 *   
 *   Dispositivo utilizado: NodeMCU 1.0
 *   Velocidad de transmisión: 9600bps.
 */
 
// Se incluyen las librerías necesarias:
// Para el microcontrolador ESP8266l
#include <ESP8266WiFi.h>

// Para la conexión con el proyecto de Blynk
#include <BlynkSimpleEsp8266.h>

// Para los sensores DHT22 (interno para To y externo para el ambiente)
#include <DHT.h>

// Común a los dispositivos que utilizan bus I2C (GDK101, TSL2591 y RTC DS3231).
#include <Wire.h>

// Para el sensor de luminosidad
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"

// Para el adaptador de tarjetas microSD
#include <SPI.h>
#include <SdFat.h>                                 // Mejor que la SD.h en cuanto al
                                                   // consumo
// Para el uso del reloj RTC DS3231
#include "RTClib.h"
/*  
 *   Direcciones I2C por defecto:
 *   GDK 101     :: 0x18
 *   TSL2591     :: 0x29
 *   RTC DS3231  :: 0x68
 */

char auth[] = "YDWKmVVIEWo_hJE42ZPg7O7niEVtN5hm";  // Identificador del proyecto en Blynk
char ssid[] = "MOVISTAR_B802";                     // Introducir el nombre de la red Wifi accesible
char pass[] = "n22WaAZWmN822fma83QU";              // Contraseña de la red Wifi
//char ssid[] = "Redmi";                           // Accesible en la casa-cueva
//char pass[] = "12345678";

#define BLYNK_PRINT Serial                         // Definición para mostrar los datos en Blynk
SimpleTimer timer;                                 // Sincroniza el reloj Arduino con Blynk

#define DHTPIN 2                                   // Se define el pin de conexión del sensor DHT22, D4 (GPIO2)

#define DHTTYPE DHT22                              // Se elige el sensor de la familia DHT que se utiliza
DHT dht(DHTPIN, DHTTYPE);                          // Configuración de la librería DHT
float Ha, Ta;                                      // Declaración de variables del DHT22

#define CO2 A0                                     // Se define la entrada analógica del sensor MH-Z14A
uint16_t ppm;                                      // Declaración de variable del MH-Z14A

int addr = 0x18;                                   // Dirección por defecto del sensor GDK101
byte buffer[2] = {0,0};                            // Se establece el bus que se utilizará en la interfaz I2C
int day,hour,minute,sec = 0;                       // Se establecen los datos secundarios del sensor, para
int status = 0;                                    // visualizar por monitor serie       
float value2;                                      // Declaración variable del GDK101                                              

Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);     // Pasa un número para el identificador del sensor
float lux;                                         // Declaración variable del TSL2591

#define DHTPIN_g 0                                 // Pin de conexión del sensor DHT22 para la Tg, D3 (GPIO0)
DHT dht_g(DHTPIN_g, DHTTYPE);                      // Configuración de la librería DHT para la Tg
float Hg, Tg, To;                                  // Declaración de variables del sensor de temperatura operativa

#define chipSelect 15                              // Lector SD. Pin CS - D8 (GPIO15)
SdFat SD;                                          // Necesario para sustituir SdFat.h por SD.h
File logFile;                                      // Variable de tipo fichero
char fileName[] = "datalog.csv";                   // Nombre del fichero de salida donde se guardarán las medidas
#define LOGFILE "datalog.csv"           

RTC_DS3231 rtc;                                    // Variable para la llamada al RTC
char buf[] = "DD/MM/YY \t hh:mm:ss";               // Tipo de buffer utilizado al mostrar la fecha y la hora

//Función para checkear la conexión de Blynk cada 30 segundos
void CheckConnection() {    
  if (!Blynk.connected()) {                        // Si no está conectado,
    Serial.println("No conectado al servidor Blynk"); 
    Blynk.connect();                               // Intenta conectarse con el tiempo de espera predeterminado
  }
  else{
    Serial.println("Conectado al servidor Blynk");     
  }
}

// Función para leer los sensores
void SendSensor(){
  // Mediciones del sensor DHT22 externo
  Ha = dht.readHumidity();                         // Se obtiene la humedad relativa del ambiente
  Ta = dht.readTemperature()+0.6134;               // Se obtiene la temperatura seca del ambiente y se suma el sesgo
  if (isnan(Ha) || isnan(Ta)){                     // Se comprueba si lee correctamente
    Serial.println("No se pudo leer desde el sensor DHT");
    return;
  }
  // Muestra los valores por el monitor serie
  Serial.println();
  Serial.print(F("Humedad relativa: \t\t"));
  Serial.print(Ha);
  Serial.println(F(" %"));
  Serial.print(F("Temperatura seca: \t\t"));
  Serial.print(Ta);
  Serial.println(F(" °C"));
  // Asocia los dos valores con las variables virtuales de Blynk correspondientes
  Blynk.virtualWrite(V0, uint8_t(Ha));             // V0 para la humedad relativa ambiental
  Blynk.virtualWrite(V1, uint8_t(Ta));             // V1 para la temperatura seca ambiental
  
  float v = analogRead(CO2)*3.3/1023.0;            // Convertir a voltaje la entrada A0
  ppm = int((v-0.4)*3125.0);                       // Calcular concentración CO2 en ppm con la tensión de entrada
  // Muestra el valor por el monitor serie
  Serial.println();
  Serial.print(F("Concentración CO2: \t\t"));
  Serial.print(ppm);
  Serial.println(F(" ppm"));
  // Asocia el dato con la variable virtuales de Blynk
  Blynk.virtualWrite(V2, ppm);                     // V2 para las ppm de CO2
  
  // Llamada a la función Gamma_Mod_Read_Value() del sensor GDK101
  Serial.println();
  Gamma_Mod_Read_Value();

  // Función para leer a la vez el espectro completo e infrarrojo y convertirlo a lx
  Serial.println();
  advancedRead();
  
  // Mediciones del sensor DHT22 colocado en la semiesfera negra mate para la Tg
  Hg = dht_g.readHumidity();                       // Se obtiene la humedad de globo
  Tg = dht_g.readTemperature()-1.3761;             // Se obtiene la temperatura de globo y se le resta el sesgo
  if (isnan(Hg) || isnan(Tg)){                     // Se comprueba si lee correctamente
    Serial.println("No se pudo leer desde el sensor de globo");
    return;
  }

  // Velocidad del aire para ventilación forzada
  float Va = 0.2;                                 
  // Cálculo de la temperatura radiante media a partir de la temperatura de globo (Tg) y del aire (Ta)
  double Trm = pow((pow((Tg+273.0),4.0) + 2.5*pow(10.0,8.0)*pow(Va,0.6)*(Tg-Ta)),0.25) - 273.0;
  // Cálculo de la temperatura operativa
  To = (Ta+float(Trm))/2;
  // Muestra el valor por el monitor serie
  Serial.println();
  Serial.print(F("Temperatura operativa: \t\t"));
  Serial.print(To);
  Serial.println(F(" ºC"));
  Serial.println();
  // Asocia el dato de To con la variable virtual de Blynk
  Blynk.virtualWrite(V5, uint8_t(To));             // V5 para la temperatura operativa
}

// Función para organizar las fases del sensor GDK101
void Gamma_Mod_Read_Value(){
  Gamma_Mod_Read(0xB0);                            // Muestra el funcionamiento
  Gamma_Mod_Read(0xB1);                            // Muestra el tiempo de medición
  Gamma_Mod_Read(0xB2);                            // Realiza la media de las mediciones cada 10 minutos
  Gamma_Mod_Read(0xB3);                            // Lee los valores medidos cada minuto
}

// Función para leer los datos del sensor GDK101
void Gamma_Mod_Read(int cmd){

  Wire.beginTransmission(addr);                    // Se comienza a escribir la secuencia
  Wire.write(cmd);
  Wire.endTransmission();                          // Fin de escritura de la secuencia
  delay(10);

  Wire.requestFrom(addr, 2);                       // Comienza a leer la secuencia

  // Bucle para leer cada uno de los datos
  byte i = 0;
  while(Wire.available())
  {
    buffer[i] = Wire.read();
    i++;
  }
  // Fin de lectura de la secuencia

  // Llamada a función para mostrar los resultados
  Print_Result(cmd);
}

// Función para los datos del GDK101 por el monitor serie
void Print_Result(int cmd){
  float value = 0.0f;                              // Declaración de las variables que se utilizarán

   switch(cmd){                                    // Selección del comando
    case 0xA0:                                     // Si se resetea (comando 0xA0) muestra por
      Serial.print(F("Reset Response\t\t\t"));     // el monitor serie si se ha realizado o no
      if(buffer[0] == 1) Serial.println("Reset Success.");
      else               Serial.println("Reset Fail(Status - Ready).");
    break;

    case 0xB0:
      Serial.print(F("GDK101 Status\t\t\t"));      // Muestra el funcionamiento del sensor
      switch(buffer[0]){                           // en función de los bits del buffer
        case 0: Serial.println("Ready"); break;
        case 1: Serial.println("10min Waiting"); break;
        case 2: Serial.println("Normal"); break;
      }
      status = buffer[0];
      Serial.print(F("VIB Status\t\t\t"));
      switch(buffer[1]){                           // Se comprueba si se ha detectado vibración
        case 0: Serial.println("OFF"); break;      // Si el bit 1 del buffer es 0, se mostrará OFF
        case 1: Serial.println("ON"); break;       // Si el bit 0 del buffer es 1, se mostrará ON   
      }
    break;

    case 0xB1:
      if(status > 0){
        sec++;                                     // Aumenta cada segundo
        Cal_Measuring_Time();                      // Llamada a la función de medición de tiempo
      }
    break;

    case 0xB2:
      Serial.print(F("Measuring Value(10min avg)\t"));
      value = buffer[0] + (float)buffer[1]/100;    // Valor medio cada 10 minutos
      Serial.print(value); Serial.println(" uSv/hr");
    break;

    case 0xB3:                                     // Comando para obtener los valores medidos
      Serial.print(F("Measuring Value(lmin avg)\t"));
      value = buffer[0] + (float)buffer[1]/100;    // Valor medido en uSv / hr
      Serial.print(value); Serial.println(F(" uSv/hr"));
      Serial.print(F("Concentración de Radón: \t"));
      float value2;
      if (value == 0){
        value2 = 0;
      }
      else{                                        // Si es distinto de 0,
        // Definición de las variables que se utilizan para el cálculo de CRn
        float lambda = 0.0000021;
        float densidad = 1600;
        float H = 3.5;
        float E = 0.25;
        float tv = 0.0027;
        float D = 0.000002;
        // Obtención del valor en Bq / m^3
        value2 = (value*114-0.451)*E*densidad*sqrt(lambda*D)/(0.1887*H*(lambda+tv)); 
      }
      // Muestra el valor en Bq/m^3 por el monitor serie
      Serial.print(value2); 
      Serial.println(F(" Bq/m^3"));
      // Asocia dicho dato con la variable virtual de Blynk
      Blynk.virtualWrite(V3,uint8_t(value2));      // V3 para el valor medido en Bq / m^3
    break;

    case 0xB4:
      Serial.print(F("FW Version\t\t\t"));        
      Serial.print(F("V")); Serial.print(buffer[0]);
      Serial.print(F(".")); Serial.println(buffer[1]);
    break;
    }  
}

// Para la medición de tiempo del GDK101
void Cal_Measuring_Time(){
  if(sec == 60) { sec = 0; minute++; }
  if(minute == 60) { minute = 0; hour++; }
  if(hour == 24) { hour = 0; day++; }
  // Muestra el tiempo de medición por el monitor serie
  Serial.print(F("Measuring Time\t\t\t"));
  Serial.print(day); Serial.print(F("d "));
  if(hour < 10) Serial.print(F("0"));
  Serial.print(hour); Serial.print(F(":"));
  if(minute < 10) Serial.print(F("0"));
  Serial.print(minute); Serial.print(F(":"));
  if(sec < 10) Serial.print(F("0"));
  Serial.println(sec);
}

// Función para leer los datos del sensor TSL2591
void advancedRead(void)
{
  uint32_t lum = tsl.getFullLuminosity();                   
  uint16_t ir, full;                                        
  ir = lum >> 16;                                  // 16 bits superiores para IR 
  full = lum & 0xFFFF;                             // 16 inferiores para espectro completo
  lux = tsl.calculateLux(full, ir);
  // Calibración del sensor
  if (lux > 200) {lux += 276.8573;}
  else if (lux > 100) {lux += 174.0614;}
  else if (lux > 60) {lux += 115.475;}
  else if (lux > 0) {lux += 74.9275;}
  // Muestra los valores de luminosidad por el monitor serie
  Serial.print(F("Luminosidad: \t"));
  Serial.print(F("IR: ")); Serial.print(ir);  Serial.print(F("  "));
  Serial.print(F("Full: ")); Serial.print(full); Serial.print(F("  "));
  Serial.print(F("Visible: ")); Serial.print(full - ir); Serial.print(F("  "));
  Serial.print(F("Lux: ")); Serial.println(float(lux));
  // Asocia el dato luminosidad en lx con la variable virtual de Blynk
  Blynk.virtualWrite(V4, uint8_t(lux));            // V4 para la luminosidad
}

// Función que almacena los datos en la tarjeta microSD y los muestra por el monitor serie 
void DatosTarjetaSD() {
  DateTime now = rtc.now();                        // Obtención de la fecha y hora actual
  String nowStr = now.toString(buf);               // Pasa a tipo "String"
  // Datos que se quieren guardar y mostrar por el monitor serie
  String dataString = nowStr+"\t"+String(Ha)+"\t"+String(Ta)+"\t"+String(ppm)+"\t  "+String(value2)+"\t      "
  +String(lux)+"\t"+String(Hg)+"\t"+String(Tg)+"\t"+String(To);
  
  if (!SD.begin(chipSelect,SPI_HALF_SPEED)){       // Chequeo
    Serial.println(F("Fallo en el adaptador de tarjetas microSD"));
  }
  else {                                           // Si no hay fallo,
    logFile = SD.open(fileName, FILE_WRITE);       // Abre el fichero
    if (logFile) {                                 // Si está disponible, escribe en él
      logFile.println(dataString);                 // Envía al fichero un string con los datos
      logFile.close();                             // Cierra el fichero
      Serial.println(F("Fecha\t\t Hora\t\tHR(%)\tTs(ºC)\tCO2(ppm)  CRn(Bq/m3)  Lum(lx)  Hg(%)\tTg(ºC)\tTo(ºC)"));
      Serial.println(dataString);                  // Envia al monitor serie el mismo string
      Serial.println();
      Serial.println(F("*****************************************************************"));
    }
    else {                                         // Si no está disponible, fallo
      Serial.println(F("Fallo en el adaptador de tarjetas microSD"));  
    }
  }
}

// Función para los detales del sensor TSL2591
void displaySensorDetails(void)
{
  sensor_t sensor;                                 // Declaración de la variable
  tsl.getSensor(&sensor);                          // Obtención de los detalles de dicho sensor
  Serial.println(F("--------------------------------"));
  Serial.print  (F("Sensor:       ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:   ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:    ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:    ")); Serial.print(sensor.max_value); Serial.println(F(" lux"));
  Serial.print  (F("Min Value:    ")); Serial.print(sensor.min_value); Serial.println(F(" lux"));
  Serial.print  (F("Resolution:   ")); Serial.print(sensor.resolution, 4); Serial.println(F(" lux"));  
  Serial.println(F("--------------------------------"));
  delay(500);
}

// Función para configurar el sensor TSL2591
void configureSensor(void)
{
  tsl.setGain(TSL2591_GAIN_MED);                   // Ganancia media de 25x
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);    // Tiempo de integración de 300ms
  
  Serial.print  (F("Gain:         "));
  tsl2591Gain_t gain = tsl.getGain();              // Obtención de la ganancia
  switch(gain) {                                   // Muestra la ganancia en función de la obtenida
    case TSL2591_GAIN_LOW: 
      Serial.println(F("1x (Low)"));
      break;
    case TSL2591_GAIN_MED:
      Serial.println(F("25x (Medium)"));
      break;
    case TSL2591_GAIN_HIGH:
      Serial.println(F("428x (High)"));
      break;
    case TSL2591_GAIN_MAX:
      Serial.println(F("9876x (Max)"));
      break;
  }
  Serial.print  (F("Timing:       "));
  Serial.print((tsl.getTiming() + 1) * 100, DEC);  // Muestra el tiempo de medición
  Serial.println(F(" ms"));
  Serial.print(F("==============================================="));
  Serial.println(F(""));
}

void setup() {
  Serial.begin(9600);                              // Velocidad de transmisión para monitor serie
  Wire.begin(4,5);                                 // Se establecen las entradas para la interfaz I2C
                                                   // GPIO4 (D2) = SDA y GPIO5 (D1) = SCL
  rtc.begin();                                     // Inicializa el reloj RTC DS3231
  // Cabecera de presentación en el monitor serie
  Serial.println(F(" "));
  Serial.println(F("Datalogger. Elena Martínez 2021"));
  Serial.println(F("Nodo sensor para control de calidad de aire con estimación de To basado en HW abierto")); 
  Serial.println(F("TFG. Ingenieria Electrónica, Robótica y Mecatrónica"));
  Serial.println(F("Placa Node MCU"));
  DateTime date = rtc.now();                       // Fecha y hora en el momento de cargar el programa
  Serial.println(date.toString(buf));              // Muestra la fecha y la hora en el monitor serie
  Serial.println(" "); 

  // Conexión Wifi
  WiFi.begin(ssid, pass);
  // Mientras se conecte, se reintenta 20 veces y si no, sólo se guardan los datos en la SD sin conectarse a Blynk 
  uint8_t i = 0;                                   // Se declara aquí para que se ponga a 0 cada carga
  uint8_t j = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if(i == 21){
    Serial.print("No pudo conectarse a "); 
    Serial.println(ssid);
    delay(500);
  }
  else {                                           // Si se conecta al WiFi, se intenta conectar a Blynk
    Blynk.connectWiFi(ssid,pass);                  // Configura conexión WiFi con Blynk
    timer.setInterval(30000L,CheckConnection);     // Chequeo cada 30 segundos
    Blynk.config(auth);                            // Configura el código de autenticación
    Blynk.connect();                               // Conexión con Blynk
    // Se reintenta 20 veces sino, sólo se guardan los datos en la tarjeta
    while (Blynk.connected() == false && j++ < 20) delay(500);
    if(j == 21){
      Serial.print("No pudo conectarse a Blynk");
      delay(500);
    }
    else {
      Serial.println("Conexión con Blynk");
    }
  }
  
  // Configura el pin CS
  pinMode(chipSelect,OUTPUT);

  // Puesta en marcha de los DHT22
  dht.begin();
  dht_g.begin();

  // Llamada a la función leer sensores y guardar datos en la tarjeta cada 15 minutos 
  // (1000 s * 60 s * 15 min = 900.000)
  timer.setInterval(900000L, SendSensor);
  timer.setInterval(900000L, DatosTarjetaSD);

  // Parte del sensor GDK101
  Serial.println(F("==============================================="));
  Serial.println(F("Gamma Sensor Sensing Start"));
  // Leer versión Firmware
  Gamma_Mod_Read(0xB4);
  // Llamada a la función leer datos del sensor GDK101 para resetearlo
  Gamma_Mod_Read(0xA0);
  Serial.println(F("==============================================="));

  // Parte del sensor TSL2591
  Serial.println(F("Arranque del Adafruit TSL2591"));
  if (tsl.begin()) {
    Serial.println(F("Sensor TSL2591 encontrado"));
  } 
  else {
    Serial.println(F("Sensor no encontrado... compruebe su cableado"));
    while (1);
  }    
  // Muestra informacion básica
  displaySensorDetails();  
  // Configura el sensor
  configureSensor();

  // Parte del adaptador SD
  if (!SD.begin(chipSelect,SPI_HALF_SPEED)) {      // Chequeo
    Serial.println(F("Fallo en el adaptador de tarjetas microSD"));
  }
  else {                                           // Si no hay fallo
    logFile = SD.open(fileName, FILE_WRITE);       // Abre el fichero
    if (logFile) {                                 // Si está disponible, escribe en él
      logFile.println(F("Fecha\tHora\tHR(%)\tTa(ºC)\tCO2(ppm)\tRn(Bq/m^3)\tLuminosidad(lx)\tHg(%)\tTg(ºC)\tTo(ºC)"));
      logFile.close();                             // Cierra el fichero
      Serial.println(F("... Cabecera de datos grabados"));
      Serial.println(F("... Dispositivos inicializados"));
      Serial.println(F("==============================================="));
    }
    else {
      Serial.println(F("Fallo en el adaptador de tarjetas microSD"));  
    }
  }
} 

void loop() {
  if (Blynk.connected()) {                         // Si se conecta a Blynk,
    Blynk.run();                                   // se inicializa  
  }
  else {                                           // Si no,
    Serial.print(".");                             // Se muestra un punto
    delay(10000);                                  // cada 10 segundos
  }
  
  timer.run();                                     // Inicializa SimpleTimer
 }
 
