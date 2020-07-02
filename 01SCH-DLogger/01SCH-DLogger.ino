/*
 Name:		_01SCH_DLogger.ino
 Created:	7/2/2020 1:41:16 PM
 Author:	mrodriguez
*/



#include <BluetoothSerial.h>
BluetoothSerial serialbt;
//La velocidad de conexi�n de la PC es a 38400

String bftTiempo[60];
float bfrRTD[60];
float bfrDHTT[60];
int bfrDHTH[60];
String bfrLA[60];
String bfrLO[60];
String cadena[60];

void setup() {
	Serial.begin(115200);
	serialbt.begin("ESP32_Manu");
	Serial.println("Iniciando");
}

void loop() {
	datos_obtener();
	datos_formatear();

	if (serialbt.available()) {
		delay(100);
		char opcion = serialbt.read();
		evaluar_serial(opcion);
	}

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
		borrar_datos();
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
	for (int i = 0; i < 60; i++)
	{
		String min;
		min = i < 10 ? ("0" + String(i)) : String(i);
		bftTiempo[i] = "2020-04-27 14:" + min + ":00";
		bfrRTD[i] = 10.4 + i;
		bfrDHTT[i] = 10.5 + i;
		bfrDHTH[i] = 12 + i;
		bfrLA[i] = "21.1171409";
		bfrLO[i] = "-101.7030218";
	}
}

void datos_formatear() {
	//Convierte datos de los busffers para ser almaceados en tarjeta SD
	for (int i; i < 60; i++) {
		cadena[i] = "{\"time\":\"" + bftTiempo[i] +
			"\",\"rtd\":" + bfrRTD[i] +
			",\"dhtt\":" + bfrDHTT[i] +
			",\"dhth\":" + bfrDHTH[i] +
			",\"la\":\"" + bfrLA[i] +
			"\",\"lo\":\"" + bfrLO[i] + "\"}";
	}
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

void borrar_datos() {
	//Funcion para borrar datos de la tarjeta SD
	Serial.println("Datos borrados!");
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
	borrar_datos();
}

void actualizar_fecha() {
	//Esta funcion recibe una fecha que sirve para actualizar el reloj en tipo real del Datalogger
	String fecha = serialbt.readString();
	serialbt.println(fecha);
}