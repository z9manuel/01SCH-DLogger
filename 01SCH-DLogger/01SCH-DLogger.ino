/*
Name:		_01SCH_DLogger.ino
Created : 7 / 2 / 2020 1 : 41 : 16 PM
Finished : 7 / 21 / 2020 21 : 55 : 00 PM
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
#define Boton 15
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
unsigned long millis_previos = 0, millis_previos_activo = 0; 
int inervalo = 30000, inervalo_activo = 10000;

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
	debug = debugActivar();
	debug ? Serial.println("Debug activado!") : false;
	activado = vaidarActivar();
	into();
}

void loop() {
	if (!activado && digitalRead(Boton)) {
		activado = true;
		iniciarActivar();
		delay(10);
		ledOK();
	}
	
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
	unsigned long millies_atcuales_activo = millis();
	if (millies_atcuales_activo - millis_previos_activo > inervalo_activo) {
		if (activado) {
			millis_previos_activo = millies_atcuales_activo;
			digitalWrite(ledVerde, HIGH);
			delay(100);
			digitalWrite(ledVerde, LOW);
		}
		else {
			millis_previos_activo = millies_atcuales_activo;
			digitalWrite(ledAzul, HIGH);
			delay(100);
			digitalWrite(ledAzul, LOW);
		}
	}
	unsigned long millies_atcuales = millis();
	if (millies_atcuales - millis_previos > inervalo){
		if (activado) {
			millis_previos = millies_atcuales;
			datos_obtener();
			datos_formatear();
			SD_escribirLog(cadena);
		}
	}

	if (serialbt.available()) {
		delay(100);
		char opcion = serialbt.read();
		evaluar_serial(opcion);
	}

}

void evaluar_serial(char opcion) {
	String laHora;
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
		SD_leerLog();
		break;
	case 'c':
		//Opci�n para actualizar fecha
		laHora = serialbt.readString();
		debug ? Serial.println("La hora recibida es: " +  laHora) : false;
		if (laHora.length() == 21) {
			horaEstablecer(laHora);
		}
		else
			debug ? Serial.println("La hora recibida es erronea.") : false;
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

bool mostrar_datos() {
	//Muestra los datos almacenado en la tarjeta SD
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
			}
			dataLog.close();
			return 1;
		}
	}
	ledFalla();
	return false;
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

bool vaidarActivar() {
	if (SD_validar()) {
		File dataLog = SD.open("/activar", FILE_READ);
		if (dataLog) {
			debug ? Serial.println("El dispositivo esta activado.") : false;
			dataLog.close();
			ledOK();
			return true;
		}
	}
	return false;
}

bool debugActivar() {
	if (SD_validar()) {
		File dataLog = SD.open("/debug", FILE_READ);
		if (dataLog) {
			debug ? Serial.println("El dispositivo esta en modo DEBUG.") : false;
			dataLog.close();
			ledOK();
			return true;
		}
	}
	return false;
}

bool iniciarActivar() {
	if (SD_validar()) {
		SD.begin(SD_CS);
		File dataLog = SD.open("/activar", FILE_APPEND);
		if (dataLog) {
			debug ? Serial.println("El dispositivo ha sido activado.") : false;
			dataLog.close();
			activado = true;
			ledOK();
			return true;
		}
	}
		return false;
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
		if (SD.remove("/activar")) {
			activado = false;
			debug ? Serial.println("Equipo desactivado.") : false;
			schFile.close();
		}
		if (SD.remove("/log.txt")) {
			debug ? Serial.println("Registro borrado.") : false;
			schFile.close();
			return 1;
		}
	}
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

bool horaEstablecer(String tiempo) {
	DateTime now = reloj.now();
	int YY, MM, DD, hh, mm, ss;
	String anno, mes, dia, hora, minuto, segundo;

	for (int i = 0; i < 4; i++)
		anno += tiempo[i];
	for (int i = 5; i < 7; i++)
		mes += tiempo[i];
	for (int i = 8; i < 10; i++)
		dia += tiempo[i];
	for (int i = 11; i < 13; i++)
		hora += tiempo[i];
	for (int i = 14; i < 16; i++)
		minuto += tiempo[i];
	for (int i = 17; i <= 18; i++)
		segundo += tiempo[i];
	debug ? Serial.println("Tiempo a establecer: "+anno+mes+dia+hora+minuto+segundo) : false;
	YY = anno.toInt();
	MM = mes.toInt();
	DD = dia.toInt();
	hh = hora.toInt();
	mm = minuto.toInt();
	ss = segundo.toInt();

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

void into() {
	Serial.println("\n");
	Serial.println("CIATEC, A.C.");
	Serial.println("www.ciatec.mx");
	Serial.println("Sistema de registro de puntos de temperatura en tiempo y coordenadas.");
	Serial.println("\n\n");
}