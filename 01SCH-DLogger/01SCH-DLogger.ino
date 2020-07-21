/*
Name:		_01SCH_DLogger.ino
Created : 7 / 2 / 2020 1 : 41 : 16 PM
Author : mrodriguez
*/

#include <BluetoothSerial.h>
#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include <DHT.h>
#include "RTClib.h"
#include <TinyGPS++.h>
#include <SoftwareSerial.h>


#define DHTPIN		13
#define DHTTYPE		DHT22
#define SD_CS		5						// Define CS pin para modulo SD
static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;
BluetoothSerial serialbt;			//La velocidad de conexión de la PC es a 38400
DHT dht(DHTPIN, DHTTYPE);
RTC_DS1307 reloj;
File schFile;
TinyGPSPlus gps;
SoftwareSerial ssGPS(RXPin, TXPin);

bool activado = 0;
bool debug = 0;
int ledVerde = 4;
int ledAzul = 2;
int bfrDHTH;
float bfrRTD;
float bfrDHTT;
float bfrLA;
float bfrLO;
String cadena;
String bftTiempo;
unsigned long millis_previos = 0, inervalo = 30000;

void setup() {
	pinMode(ledVerde, OUTPUT);
	pinMode(ledAzul, OUTPUT);
	ledOK();
	Serial.begin(115200);
	SD_validar();
	iniciarReloj();
	serialbt.begin("SUCAHERSA_DL");
	dht.begin();
	Serial.println("Iniciando");
	ssGPS.begin(GPSBaud);
	ledOK();
	debug ? Serial.println("Debug activado!") : false;
	
	
}

void loop() {
	while (ssGPS.available() > 0)
		if (gps.encode(ssGPS.read())) {
			if (GPSobtener() == false) {
				//ledFalla();
				//Serial.println("Sin datos GPS.");
				break;
			}

		}

	if (millis() > 5000 && gps.charsProcessed() < 10)
	{
		debug ? Serial.println("GPS no detectado: revisar tarjeta.") : false;
		ledFalla();
		while (true);
	}

	unsigned long millies_atcuales = millis();
	if (millies_atcuales - millis_previos > inervalo)
	{
		millis_previos = millies_atcuales;
		datos_obtener();
		datos_formatear();
		SD_escribirLog(cadena);
	}

	if (serialbt.available()) {
		delay(100);
		char opcion = serialbt.read();
		evaluar_serial(opcion);
	}

	//delay(15000);

	/*
		if (Serial.available()) {
		char x = Serial.read();
		serialbt.write(x);
	}
	if (serialbt.available()) {
		char y = serialbt.read();
		Serial.print(y);
	}
	*/
}

void evaluar_serial(char opcion) {
	switch (opcion) {
	case 'd':
		//Opci�n para mostrar datos
		mostrar_datos();
		break;
	case 'b':
		//Opci�n para borrar datos
		SD_borrarLog();
		break;
	case 's':
		//Opci�n para enviar datos
		enviar_datos();
		break;
	case 'c':
		//Opci�n para actualizar fecha
		actualizar_fecha();
		break;
	default:
		break;
	}

}
void datos_obtener() {
	//Generaci�n de datos que simulan los datos obtenidos de los sensores
	float h = dht.readHumidity();
	float t = dht.readTemperature();

	if (isnan(h) || isnan(t)) {
		debug ? Serial.println("No se peuede leer temperatura/humedad") : false;
		delay(2000);
	}
	bftTiempo = tiempo_obtener();	//ok
	bfrRTD = 0.00;					//NO
	bfrDHTT = t;					//ok
	bfrDHTH = h;					//ok
}

void datos_formatear() {
	//Convierte datos de los busffers para ser almaceados en tarjeta SD
	cadena = "{\"time\":\"" + bftTiempo +
		"\",\"rtd\":" + bfrRTD +
		",\"dhtt\":" + bfrDHTT +
		",\"dhth\":" + bfrDHTH +
		",\"la\":\"" + String(bfrLA, 6) +
		"\",\"lo\":\"" + String(bfrLO, 6) + "\"}";
	//Serial.println(cadena);
}


void mostrar_datos() {
	//Muestra los datos almacenado en la tarjeta SD
	String linea;
	for (int i = 0; i < 60; i++) {
		linea = cadena[i];
		Serial.println(linea);
		serialbt.println(linea);
	}
}


void enviar_datos() {
	//Muestra los datos almacenado en la tarjeta SD y despues los debe borrar
	String linea;
	serialbt.println("INICIO");
	for (int i = 0; i < 60; i++) {
		linea = cadena[i];
		debug ? Serial.println(linea) : false;
		serialbt.println(linea);
	}
	serialbt.println("FIN");
	SD_borrarLog();
}

void actualizar_fecha() {
	//Esta funcion recibe una fecha que sirve para actualizar el reloj en tipo real del Datalogger
	String fecha = serialbt.readString();
	serialbt.println(fecha);
}



bool SD_leerLog() {
	if (SD_validar()) {
		File dataLog = SD.open("/log.txt", FILE_READ);
		if (dataLog) {
			bool enviado = 0;
			String linea;
			dataLog.position();
			while (dataLog.available()) {
				linea = dataLog.readStringUntil('\n');
				debug ? Serial.println(linea) : false;
				serialbt.println(linea);
				/*enviado = enviar_a_API(linea);
				if (enviado == false) {
					return false;
				}*/
			}
			dataLog.close();
			if (SD_borrarLog()) {
				return 1;
			}
		}
	}
	ledFalla();
	return false;
}

bool SD_escribirLog(String cadena) {
	if (SD_validar()) {
		SD.begin(SD_CS);
		File dataLog = SD.open("/log.txt", FILE_APPEND);
		if (dataLog) {
			dataLog.println(cadena);
			dataLog.close();
			debug ? Serial.println(cadena) : false;
			ledOK();
			return 1;
		}
	}
	ledFalla();
	return false;
}

bool SD_borrarLog() {
	if (SD_validar()) {
		SD.begin(SD_CS);
		if (SD.remove("/log.txt")) {
			debug ? Serial.println("Registro borrado.") : false;
			schFile.close();
			return 1;
		}
	}
	ledFalla();
	ledFalla();
	return false;
}

bool SD_validar() {
	SD.begin(SD_CS);
	if (!SD.begin(SD_CS)) {
		debug ? Serial.println("Error modulo SD!") : false;
		ledFalla();
		return false;
	}
	uint8_t cardType = SD.cardType();
	if (cardType == CARD_NONE) {
		debug ? Serial.println("Error tarjeta SD!") : false;
		ledFalla();
		return false;
	}
	if (!SD.begin(SD_CS)) {
		debug ? Serial.println("ERROR - Falla en tarjeta SD!") : false;
		ledFalla();
		return false;
	}
	return 1;
}

void iniciarReloj() {
	if (!reloj.begin()) {
		debug ? Serial.println("No se encontro reloj") : false;
		while (1);
	}
	//horaEstablecer();
}

String tiempo_obtener() {
	DateTime now = reloj.now();
	String ahora;
	String YY, MM, DD, hh, mm, ss;
	YY = String(now.year());
	MM = String(now.month());
	DD = String(now.day());
	hh = String(now.hour());
	mm = String(now.minute());
	ss = String(now.second());

	MM.length() == 1 ? MM = "0" + MM : MM;
	DD.length() == 1 ? DD = "0" + DD : DD;
	hh.length() == 1 ? hh = "0" + hh : hh;
	mm.length() == 1 ? mm = "0" + mm : mm;
	ss.length() == 1 ? ss = "0" + ss : ss;
	ahora = YY + "-" + MM + "-" + DD + " " + hh + ":" + mm + ":" + ss;
	return ahora;
}


bool horaEstablecer() {
	DateTime now = reloj.now();
	int YY, MM, DD, hh, mm, ss;

	YY = 2020;
	MM = 07;
	DD = 11;
	hh = 01;
	mm = 40;
	ss = 00;

	reloj.adjust(DateTime(YY, MM, DD, hh, mm, ss));
	return 1;
}

bool GPSobtener()
{
	if (gps.location.isValid())
	{
		bfrLA = gps.location.lat();
		bfrLO = gps.location.lng();
		return true;
	}
	else
	{
		float bfrLA = 0;
		float bfrLO = 0;
		return false;
	}
}


void ledOK() {
	for (int i = 1; i <= 3; i++) {
		digitalWrite(ledVerde, HIGH);
		digitalWrite(ledAzul, HIGH);
		delay(100);
		digitalWrite(ledVerde, LOW);
		digitalWrite(ledAzul, LOW);
		delay(100);
	}
	
}

void ledFalla() {
	for (int i = 1; i <= 10; i++) {
		digitalWrite(ledVerde, LOW);
		digitalWrite(ledAzul, HIGH);
		delay(100);
		digitalWrite(ledVerde, HIGH);
		digitalWrite(ledAzul, LOW);
		delay(100);
	}
	digitalWrite(ledVerde, LOW);
	digitalWrite(ledAzul, LOW);
}