/*
 Name:		_01SCH_DLogger.ino
 Created:	7/2/2020 1:41:16 PM
 Author:	mrodriguez
*/

#include <BluetoothSerial.h>
#include "FS.h"
#include "SD.h"
#include <SPI.h>
#include <DHT.h>
#include "RTClib.h"

#define DHTPIN		13
#define DHTTYPE		DHT22
#define SD_CS		5						// Define CS pin para modulo SD
BluetoothSerial serialbt;			//La velocidad de conexión de la PC es a 38400
DHT dht(DHTPIN, DHTTYPE);
RTC_DS1307 reloj;
File schFile;

int ledVerde = 4;
int ledAzul = 2;
int bfrDHTH;
float bfrRTD;
float bfrDHTT;
String bfrLA;
String bfrLO;
String cadena;
String bftTiempo;

void setup() {
	pinMode(ledVerde, OUTPUT);
	pinMode(ledAzul, OUTPUT);
	digitalWrite(ledVerde, HIGH);
	delay(500);
	digitalWrite(ledAzul, HIGH);
	Serial.begin(115200);
	SD_validar();
	iniciarReloj();
	serialbt.begin("SUCAHERSA_DL");
	dht.begin();
	Serial.println("Iniciando");
	delay(500);
	digitalWrite(ledVerde, LOW);
	delay(500);
	digitalWrite(ledAzul, LOW);
	delay(1000);
}

void loop() {
	datos_obtener();
	datos_formatear();
	SD_escribirLog(cadena);

	if (serialbt.available()) {
		delay(100);
		char opcion = serialbt.read();
		evaluar_serial(opcion);
	}

	delay(30000);

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
		Serial.println("No se peuede leer temperatura/humedad");
		delay(2000);
	}
	bftTiempo = tiempo_obtener();	//ok
	bfrRTD = 0.00;					//NO
	bfrDHTT = t;					//ok
	bfrDHTH = h;					//ok
	bfrLA = "21.1171409";
	bfrLO = "-101.7030218";
}

void datos_formatear() {
	//Convierte datos de los busffers para ser almaceados en tarjeta SD
		cadena = "{\"time\":\"" + bftTiempo +
			"\",\"rtd\":" + bfrRTD +
			",\"dhtt\":" + bfrDHTT +
			",\"dhth\":" + bfrDHTH +
			",\"la\":\"" + bfrLA +
			"\",\"lo\":\"" + bfrLO + "\"}";
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
		Serial.println(linea);
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
				Serial.println(linea);
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
	return false;
}

bool SD_escribirLog(String cadena) {
	if (SD_validar()) {
		SD.begin(SD_CS);
		File dataLog = SD.open("/log.txt", FILE_APPEND);
		if (dataLog) {
			digitalWrite(ledAzul, HIGH);
			dataLog.println(cadena);
			dataLog.close();
			Serial.println(cadena);
			delay(150);
			digitalWrite(ledAzul, LOW);
			return 1;
		}
	}
	return false;
}

bool SD_borrarLog() {
	if (SD_validar()) {
		SD.begin(SD_CS);
		if (SD.remove("/log.txt")) {
			Serial.println("Registro borrado.");
			schFile.close();
			return 1;
		}
	}
	return false;
}

bool SD_validar() {
	SD.begin(SD_CS);
	if (!SD.begin(SD_CS)) {
		Serial.println("Error modulo SD!");
		return false;
	}
	uint8_t cardType = SD.cardType();
	if (cardType == CARD_NONE) {
		Serial.println("Error tarjeta SD!");
		return false;
	}
	if (!SD.begin(SD_CS)) {
		Serial.println("ERROR - Falla en tarjeta SD!");
		return false;
	}
	return 1;
}

void iniciarReloj() {
	if (!reloj.begin()) {
		Serial.println("No se encontro reloj");
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
	DD = 10;
	hh = 11;
	mm = 49;
	ss = 00;

	reloj.adjust(DateTime(YY, MM, DD, hh, mm, ss));
	return 1;
}
