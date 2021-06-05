#include "DHT.h"

#define DHTPIN 2       // Digital pin connected to the DHT sensor [D4-GPIO2]
#define DHTPIN_g 0     // DHT para el sensor de temperatura de globo [D3-GPIO0]

#define DHTTYPE DHT22  // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);
DHT dht_g(DHTPIN_g, DHTTYPE);

void setup() {
  Serial.begin(9600);
  Serial.println(F("Test del sensor de temperatura radiante media!"));

  dht.begin();
  dht_g.begin();
}

void loop() {
  // Wait a few seconds between measurements.
  delay(120000);       // 120,000ms = 2min 

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  // Mediciones del sensor DHT22 colocado en la semiesfera negra mate para la Tg
  float Hg = dht_g.readHumidity();                 // Se obtiene la humedad en el interior de la semiesfera
  float Tg = dht_g.readTemperature();              // Se obtiene la temperatura de globo
  
  if (isnan(Hg) || isnan(Tg)){
    Serial.println("Failed to read from balloon sensor!");
    return;
  }

  // Velocidad del aire
  float Va = 0.2;                                 
  // Cálculo de la temperatura radiante media a partir de la temperatura de globo (Tg) y del aire (t)
  double Trm = pow((pow((Tg+273.0),4.0) + 2.5*pow(10.0,8.0)*pow(Va,0.6)*(Tg-t)),0.25) - 273.0;
  // Cálculo de la temperatura operativa
  float To = (t+float(Trm))/2;

  Serial.println();
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.print(F("°F"));
  Serial.println();
  Serial.print(F("Balloon Hum: "));              // Humidity inside the hemisphere
  Serial.print(Hg);
  Serial.print(F("%  Balloon Temp: "));          // Balloon temperature
  Serial.print(Tg);                                     
  Serial.print(F("ºC  Trm: "));
  Serial.print(Trm);                               
  Serial.print(F("ºC  To: "));                   // Operating temperature
  Serial.print(To);
  Serial.print(F("°C "));
  Serial.println();
}
