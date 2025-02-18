
#include <Defcon.h>
#include <Arduino.h>
#include <ArduinoJson.h>				// OJO: Tener instalada una version NO BETA (a dia de hoy la estable es la 5.13.4). Alguna pata han metido en la 6
#include <NTPClient.h>					// Para la gestion de la hora por NTP
#include <WiFiUdp.h>					// Para la conexion UDP con los servidores de hora.
#include <FS.h>							// Libreria Sistema de Ficheros
#include <Configuracion.h>				// Fichero de configuracion
#include <Adafruit_NeoPixel.h>			// Para los LED: https://adafruit.github.io/Adafruit_NeoPixel/html/class_adafruit___neo_pixel.html

// OBJETOS
Adafruit_NeoPixel MisLeds (NUMEROLEDS, PINLEDS, NEO_GRB + NEO_KHZ400);

// Constructor
Defcon::Defcon(String fich_config_Defcon, NTPClient& ClienteNTP) : ClienteNTP(ClienteNTP) {

    HardwareInfo = "Defcon-1.2";
	ComOK = false;
	HayQueSalvar = false;
	mificheroconfig = fich_config_Defcon;
	t_uptime = millis();
	
	// Inicializar los arrays
	// Valores de HUE para los LED
	HueBloque[0] = HUELED0;
	HueBloque[1] = HUELED1;
	HueBloque[2] = HUELED2;
	HueBloque[3] = HUELED3;
	HueBloque[4] = HUELED4;
	HueBloque[5] = HUELED5;

	// Valores de Saturacion para los LED
	SaturacionBloque[0] = SATLED0;
	SaturacionBloque[1] = SATLED1;
	SaturacionBloque[2] = SATLED2;
	SaturacionBloque[3] = SATLED3;
	SaturacionBloque[4] = SATLED4;
	SaturacionBloque[5] = SATLED5;
	
	// Valores de indice del primer led de cada bloque
	PrimerLed[0] = 0;
	PrimerLed[5] = PrimerLed[0] + NUMLED0;
	PrimerLed[4] = PrimerLed[5] + NUMLED5;
	PrimerLed[3] = PrimerLed[4] + NUMLED4;
	PrimerLed[2] = PrimerLed[3] + NUMLED3;
	PrimerLed[1] = PrimerLed[2] + NUMLED2;

	// Valores de indice del ultimo led de cada bloque
    UltimoLed[0] = NUMLED0 -1;
	UltimoLed[5] = UltimoLed[0] + NUMLED5;
	UltimoLed[4] = UltimoLed[5] + NUMLED4;
	UltimoLed[3] = UltimoLed[4] + NUMLED3;
	UltimoLed[2] = UltimoLed[3] + NUMLED2;
	UltimoLed[1] = UltimoLed[2] + NUMLED1;

}

// Asignar la funcion de callback de responder comandos
void Defcon::SetRespondeComandoCallback(RespondeComandoCallback ref) {

	MiRespondeComandos = (RespondeComandoCallback)ref;

}

// Metodo que devuelve un JSON con el estado
String Defcon::MiEstadoJson(int categoria) {

	DynamicJsonBuffer jBuffer;
	JsonObject& jObj = jBuffer.createObject();

	// Dependiendo del numero de categoria en la llamada devolver unas cosas u otras
	switch (categoria)
	{

	case 1:

		// Esto llena de objetos de tipo "pareja propiedad valor"
		jObj.set("TIME", ClienteNTP.getFormattedTime());				// HORA
		jObj.set("HI", HardwareInfo);									// Info del Hardware
		jObj.set("UPT", millis() - t_uptime / 1000 );					// Uptime en segundos
		jObj.set("DL", DefconLevelActual);								// Defcon Level
		jObj.set("DRT", (millis() - MillisRXDatos) / 1000 );			// Tiempo en seg. desde la recepcion del utimo dato de zabbix


		break;

	case 2:

		jObj.set("INFO2", "INFO2");							
		
		break;

	default:

		jObj.set("NOINFO", "NOINFO");						// MAL LLAMADO

		break;
	}


	// Crear un buffer (aray de 100 char) donde almacenar la cadena de texto del JSON
	char JSONmessageBuffer[200];

	// Tirar al buffer la cadena de objetos serializada en JSON con la propiedad printTo del objeto de arriba
	jObj.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

	// devolver el char array con el JSON
	return JSONmessageBuffer;
	
}

// Guardar configuracion en fichero
boolean Defcon::SalvaConfig(){
	

	File mificheroconfig_handler = SPIFFS.open(mificheroconfig, "w");

	if (!mificheroconfig_handler) {
		Serial.println("No se puede abrir el fichero de configuracion de mi proyecto");
		return false;
	}

	if (mificheroconfig_handler.print(MiEstadoJson(1))){

		return true;

	}

	else {

		return false;

	}

}

// Leer configuracion desde fichero.
boolean Defcon::LeeConfig(){

	// Sacar del fichero de configuracion, si existe, las configuraciones permanentes
	if (SPIFFS.exists(mificheroconfig)) {

		File mificheroconfig_handler = SPIFFS.open(mificheroconfig, "r");
		if (mificheroconfig_handler) {
			size_t size = mificheroconfig_handler.size();
			// Declarar un buffer para almacenar el contenido del fichero
			std::unique_ptr<char[]> buf(new char[size]);
			// Leer el fichero al buffer
			mificheroconfig_handler.readBytes(buf.get(), size);
			DynamicJsonBuffer jsonBuffer;
			JsonObject& json = jsonBuffer.parseObject(buf.get());
			if (json.success()) {

				Serial.print("Configuracion de mi proyecto Leida: ");
				json.printTo(Serial);
				Serial.println("");
				return true;

			}

			return false;

		}

		return false;

	}

	return false;

}

// Inicializar las cosas
void Defcon::Iniciar(){

	Serial.println("Iniciando Leds");

	// Test de Inicio

	MisLeds.begin();

		
	MisLeds.clear();
	MisLeds.show();

	
	// Poner a Cero los niveles Defcon (nivel cero es todo apagado)
	DefconLevelActual = 5;
	DefconLevelFuturo = 5;
	Estado_Cambio_Defcon = DEFCON_SIN_CAMBIOS;

	// Pitido Comunicaciones KO
	SilencioComunicaciones = false;

	// Poner los LED en su estado INICIAL
	uint32_t ColorDestino = MisLeds.ColorHSV(HueBloque[DefconLevelActual], SaturacionBloque[DefconLevelActual], 255);
	MisLeds.fill(ColorDestino,PrimerLed[DefconLevelActual], (UltimoLed[DefconLevelActual]-PrimerLed[DefconLevelActual]) + 1);
	
	// La cabecera a rojo al iniciar (SIN RED)
	ColorDestino = MisLeds.Color(255,0,0);
	MisLeds.fill(ColorDestino,PrimerLed[0], (UltimoLed[0]-PrimerLed[0]) + 1);
	Estado_Cabecera_Actual = CABECERA_SINRED;
	Estado_Cabecera_Futuro = CABECERA_SINRED;

	// Poner el Brillo global
	MisLeds.setBrightness(255);
	
	// Pasar los datos a los LED
	MisLeds.show();

	// Inicializar a 0 los problemas
	for (uint8_t i=0; i<4; i++){

		ProblemasZabbix[i] = 0;

	}

	// Poner a cero el contador de datos recibidos
	MillisRXDatos = 0;

	// Inicializar el flasheo de la cabecera
	FlasheandoCabecera = false;

}

// A ejecutar lo mas rapido posible
void Defcon::RunFast() {
	
	if (HayQueSalvar){

		SalvaConfig();
		HayQueSalvar = false;

	}

	// Para gestionar el color de cabecera si recibo o no datos.	
	switch (Estado_Cabecera_Actual) {
	
		case CABECERA_OK:
		
			if ((millis() - MillisRXDatos) > 100000){

				this->SetCabecera(CABECERA_SIN_DATOS);

			}

		break;

		case CABECERA_SIN_DATOS:
		
			if ((millis() - MillisRXDatos) <= 100000){

				this->SetCabecera(CABECERA_OK);

			}

		break;
		
		default:
		break;

	}


	// Maquina de estado para el timing del cambio de Defcon
	this->MaquinaEstadoCambioDefconRun();	

	// Maquina de estado para el cambio de la cabecera
	this->MaquinaEstadoCambioCabeceraRun();

	// Maquina de estado para el flasheo de la cabecera
	this->MaquinaEstadoFlasheoCabecera();
	
}

// Cambiar el nivel Defcon
void Defcon::SetDefconLevel (int l_DefconLevel){

	DefconLevelFuturo = l_DefconLevel;

}

// Cambiar el color de los leds de cabecera
void Defcon::SetCabecera(Defcon::TipoEstadosCabecera l_Estado_Cabecera){

	Estado_Cabecera_Futuro = l_Estado_Cabecera;

}

// Funcion para recibir los datos de Zabbix via JSON
void Defcon::Problemas (String jsonproblemas){

	// Resetear el contador de tiempo de RX de datos
	MillisRXDatos = millis();

	// Pillo los valores del JSON
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.parseObject(jsonproblemas);
	if (json.success()) {

		//Serial.print("JSON ZABBIX OK");
		//json.printTo(Serial);
		//Serial.println("");

		ProblemasZabbix[0] = json.get<uint8_t>("DISASTER");
		ProblemasZabbix[1] = json.get<uint8_t>("HIGH");
		ProblemasZabbix[2] = json.get<uint8_t>("AVERAGE");
		ProblemasZabbix[3] = json.get<uint8_t>("WARNING");
				
	}

	// Y calculo el nivel DEFCON
	int DefconFromZabbix = DefconLevelActual;

	// Con este algoritmo que empieza de 5 a 1
	// Si no hay problemas Defcon 5
	if (ProblemasZabbix[0] == 0 && ProblemasZabbix[1] == 0 && ProblemasZabbix[2] == 0 && ProblemasZabbix[3] == 0){

		DefconFromZabbix = 5;

	}

	// Si hay algun Warning o algun Average al menos Defcon 4
	if (ProblemasZabbix[2] > 0 || ProblemasZabbix[3] > 0){

		DefconFromZabbix = 4;

	}

	// Si hay algun HIGH o mas de 3 AVERAGES o mas de 8 WARNINGS al menos Defcon 3
	if (ProblemasZabbix[1] >0 || ProblemasZabbix[2] > 3 || ProblemasZabbix[3] > 8){

		DefconFromZabbix = 3;

	}

	// Si hay algun DISASTER o mas de 3 HIGH o mas de 6 AVERAGES o mas de 15 WARNINGS al menos Defcon 2
	if (ProblemasZabbix[0] > 0 || ProblemasZabbix[1] >3 || ProblemasZabbix[2] > 6 || ProblemasZabbix[3] > 15){

		DefconFromZabbix = 2;

	}

	// Si hay mas de 3 DISASTER o mas de 6 HIGH o mas de 15 AVERAGES o mas de 25 WARNINGS DEFCON 1 GUERRA MUNDIAL
	if (ProblemasZabbix[0] > 3 || ProblemasZabbix[1] >6 || ProblemasZabbix[2] > 15 || ProblemasZabbix[3] > 25){

		DefconFromZabbix = 1;

	}

	// Mirar el estado Defcon Actual y actualizar si ha cambiado

	if (DefconLevelActual != DefconFromZabbix){

		DefconLevelFuturo = DefconFromZabbix;
		
	}

}

// Funciones Privadas
// Inicia el tiempo de delta
void Defcon::Delta1Begin(){

	MillisDelta1 = millis();

}

// Devuelve el tiempo de Delta
unsigned long Defcon::GetDelta1(){

	return millis() - MillisDelta1;

}

// Mandar a los topic stat las configuraciones actuales
void Defcon::MandaConfig(){

	MiRespondeComandos("DEFCONLEVEL", String(DefconLevelActual));
	
}

// Para pitar si las comunicaciones no estan OK.
void Defcon::PitaAvisoComKO(){

	pinMode(PINBUZZER, OUTPUT);
	tone(PINBUZZER,1200,50);	
	delay(100);
	tone(PINBUZZER,1200,50);	
	delay(100);
	tone(PINBUZZER,1200,100);	

}

// Funcion que avisa de "algo" con un aviso sonoro
void Defcon::Aviso (int l_NumeroAviso){

	switch (l_NumeroAviso){

		case 1:

			pinMode(PINBUZZER, OUTPUT);
			tone(PINBUZZER,1200,300);	
			delay(600);
			tone(PINBUZZER,1200,300);	
			delay(600);
			tone(PINBUZZER,1200,300);	
			delay(600);

			//this->FlaseoCabecera(true);
			
		break;

		case 2:

			pinMode(PINBUZZER, OUTPUT);
			tone(PINBUZZER,1200,200);	
			delay(400);
			tone(PINBUZZER,1200,200);	
			delay(400);
			tone(PINBUZZER,1200,200);	
			delay(400);
			tone(PINBUZZER,1200,200);	
			delay(400);
			tone(PINBUZZER,1200,200);	
			delay(400);
			tone(PINBUZZER,1200,200);	
			delay(400);
			tone(PINBUZZER,1200,200);	
			delay(400);
			tone(PINBUZZER,1200,200);	
			delay(400);

			this->FlaseoCabecera(true);
			
		break;
		
		default:
		
		break;
	}

}

// Funcion para probar los led y el buzzer y reiniciar.
void Defcon::HwTest(){

	MisLeds.clear();
	MisLeds.show();

	// Probar todos los led en blanco uno por uno
	for (uint8_t i = 0; i < NUMEROLEDS; i++)
	{
		MisLeds.fill(MisLeds.Color(255,255,255),i,1);
		MisLeds.show();
		delay(200);
		MisLeds.clear();
		delay(200);

	}
		
	// Probar el buzzer
	pinMode(PINBUZZER, OUTPUT);
	tone(PINBUZZER,1000,500);	
	delay(600);
	tone(PINBUZZER,1200,500);	
	delay(600);
	tone(PINBUZZER,1400,500);	
	delay(2000);
	ESP.restart();

}

// Para activar el silencio de los avisos de comunicaciones (ACK, Enterado)
void Defcon::SilenciaAvisoComunicaciones(){

	SilencioComunicaciones = true;

}

// Funcion para activar el parpadeo de los LED durante 1 minuto (o hasta que acabe el problema)
void Defcon::FlaseoCabecera(bool l_flaseocabecera){

	switch(l_flaseocabecera){

		case true:

			FlasheandoCabecera = true;
			millisunflasheo = millis();
			millisflasheototal = millis();
			
		break;

		case false:

			FlasheandoCabecera = false;
			MisLeds.setBrightness(255);
			
			// Esto es la chapu porque no se por que no funciona bien la funcion brillo con algunos led, con el 3 en mi caso
			uint32_t ColorDestino = MisLeds.ColorHSV(HueBloque[DefconLevelActual], SaturacionBloque[DefconLevelActual], 255);
			MisLeds.fill(ColorDestino,PrimerLed[DefconLevelActual], (UltimoLed[DefconLevelActual]-PrimerLed[DefconLevelActual]) + 1);
			
			MisLeds.show();
			
		break;
		
	}

}

// Maquina de estado del cambio de Defcon
void Defcon::MaquinaEstadoCambioDefconRun(){


	switch (Estado_Cambio_Defcon){

		
		case DEFCON_SIN_CAMBIOS:

			if (DefconLevelFuturo != DefconLevelActual){

				this->Delta1Begin();
				tone(PINBUZZER,FRECDEFCON,TBUZZER);
				Estado_Cambio_Defcon = DEFCON_AVISANDO;
				if (FlasheandoCabecera) { this->FlaseoCabecera(false);}
				
			}

			// Si es igual (no hay cambios en el defcon), nos vamos a entretener pitando si fallan las comunicaciones
			// Como el delta no se usa mientras no haya cambio de defcon lo usare para esto
			// Si estamos en el cambio de defcon entonces esto de los pitidos no se hace
			else{

				if (Estado_Cabecera_Actual != CABECERA_OK && !SilencioComunicaciones && GetDelta1() >= TAVISOS*1000){

					this->PitaAvisoComKO();
					Delta1Begin();

				}

			}

		break;
		

		case DEFCON_AVISANDO:

			if (this->GetDelta1() >= TBUZZER){

				// Parar el Pitido
				this->Delta1Begin();
				Estado_Cambio_Defcon = DEFCON_PAUSA1;

			}

		break;
		

		case DEFCON_PAUSA1:

			if (this->GetDelta1() >= TPAUSA1 ){

				this->Delta1Begin();
				CuentaCiclosFade = 255;									// Para el siguiente estado. Haremos tantos fade como brillos haya encendidos
				Estado_Cambio_Defcon = DEFCON_FADE_OFF;					// Al siguiente estado que vamos

			}

		break;
		

		case DEFCON_FADE_OFF:

			// Cada vez que transcurra el tiempo que corresponda que calculamos aqui ....
			// Imaginemos que el brillo del bloque a apagar es 137. Tendremos que hacer 137 ciclos en TFADEOFF tiempo en total, o sea a un intervalo determinado
			if (GetDelta1() >= (TFADEOFF / 255) && CuentaCiclosFade >= 0){

				if (CuentaCiclosFade > 0) { CuentaCiclosFade--;}
				uint32_t ColorDestino = MisLeds.ColorHSV(HueBloque[DefconLevelActual], SaturacionBloque[DefconLevelActual], CuentaCiclosFade);
				MisLeds.fill(ColorDestino,PrimerLed[DefconLevelActual], (UltimoLed[DefconLevelActual]-PrimerLed[DefconLevelActual]) + 1);
				MisLeds.show();
				Delta1Begin();
				
			}

			// Cuando lleguemos a Cero, pues ya hemos acabado el fade, siguiente estado
			if (CuentaCiclosFade == 0){

				Estado_Cambio_Defcon = DEFCON_PAUSA2;
				Delta1Begin();

			}

		break;
		

		case DEFCON_PAUSA2:
		
			if (this->GetDelta1() >= TPAUSA2 ){

				this->Delta1Begin();
				CuentaCiclosFade = 0;		// Para el siguiente estado. Haremos tantos fade como brillos haya encendidos
				Estado_Cambio_Defcon = DEFCON_FADE_ON;					// Al siguiente estado que vamos

			}
		
		break;
		

		case DEFCON_FADE_ON:

			// Cada vez que transcurra el tiempo que corresponda que calculamos aqui ....
			// Imaginemos que el brillo del bloque a apagar es 137. Tendremos que hacer 137 ciclos en TFADEOFF tiempo en total, o sea a un intervalo determinado
			if (GetDelta1() >= (TFADEON / 255) && CuentaCiclosFade <= 255){

				if (CuentaCiclosFade < 255) {CuentaCiclosFade++;}
				uint32_t ColorDestino = MisLeds.ColorHSV(HueBloque[DefconLevelFuturo], SaturacionBloque[DefconLevelFuturo], CuentaCiclosFade);
				MisLeds.fill(ColorDestino,PrimerLed[DefconLevelFuturo], (UltimoLed[DefconLevelFuturo]-PrimerLed[DefconLevelFuturo]) + 1);
				MisLeds.show();
				Delta1Begin();
				

			}

			// Cuando lleguemos a Cero, pues ya hemos acabado el fade y el proceso de cambio entero
			if (CuentaCiclosFade == 255){

				Estado_Cambio_Defcon = DEFCON_SIN_CAMBIOS;
				DefconLevelActual = DefconLevelFuturo;
				MiRespondeComandos("DEFCONLEVEL", String(DefconLevelActual));
				this->FlaseoCabecera(true);

			}

		break;
	
	}

}

// Maquina de estado para la cabecera
void Defcon::MaquinaEstadoCambioCabeceraRun(){

	if (Estado_Cabecera_Futuro != Estado_Cabecera_Actual){

		switch (Estado_Cabecera_Futuro){
	
			case CABECERA_SINRED:
				
				MisLeds.fill(MisLeds.Color(255,0,0),PrimerLed[0], (UltimoLed[0]-PrimerLed[0]) + 1);
				Estado_Cabecera_Actual = CABECERA_SINRED;
				MisLeds.show();
				this->FlaseoCabecera(true);

			break;

			case CABECERA_AP_MODE:
				
				MisLeds.fill(MisLeds.Color(0,0,255),PrimerLed[0], (UltimoLed[0]-PrimerLed[0]) + 1);
				Estado_Cabecera_Actual = CABECERA_AP_MODE;
				MisLeds.show();
				this->FlaseoCabecera(true);

			break;


			case CABECERA_SINMQTT:
				
				MisLeds.fill(MisLeds.Color(255,140,0),PrimerLed[0], (UltimoLed[0]-PrimerLed[0]) + 1);
				Estado_Cabecera_Actual = CABECERA_SINMQTT;
				MisLeds.show();
				this->FlaseoCabecera(true);

			break;

			case CABECERA_SIN_DATOS:
				
				MisLeds.fill(MisLeds.Color(255,0,255),PrimerLed[0], (UltimoLed[0]-PrimerLed[0]) + 1);
				Estado_Cabecera_Actual = CABECERA_SIN_DATOS;
				MisLeds.show();
				this->MandaConfig();
				this->FlaseoCabecera(true);

			break;

			case CABECERA_OK:
				
				MisLeds.fill(MisLeds.Color(255,255,255),PrimerLed[0], (UltimoLed[0]-PrimerLed[0]) + 1);
				Estado_Cabecera_Actual = CABECERA_OK;
				MisLeds.show();
				this->MandaConfig();
				SilencioComunicaciones = false;
				this->FlaseoCabecera(false);

			break;
	
		}

	}
	
}

// Maquina de estado para el parpadeo
void Defcon::MaquinaEstadoFlasheoCabecera(){

	switch(FlasheandoCabecera){

		case true:

			// Apagar si encendido
			if (MisLeds.getBrightness() > 1 && (millis() - millisunflasheo > 1000)){

				
				MisLeds.setBrightness(1);
				MisLeds.show();
				millisunflasheo = millis();
				

			}

			// Encender si apagado
			else if (MisLeds.getBrightness() <= 1 && (millis() - millisunflasheo > 1000)){

				MisLeds.setBrightness(255);
				
				// Esto es la chapu porque no se por que no funciona bien la funcion brillo con algunos led, con el 3 en mi caso
				uint32_t ColorDestino = MisLeds.ColorHSV(HueBloque[DefconLevelActual], SaturacionBloque[DefconLevelActual], 255);
				MisLeds.fill(ColorDestino,PrimerLed[DefconLevelActual], (UltimoLed[DefconLevelActual]-PrimerLed[DefconLevelActual]) + 1);
				
				MisLeds.show();
				
				millisunflasheo = millis();
			
			}

			// Apagado automatico del flaseo si cabecera esta OK
			if (Estado_Cabecera_Actual == CABECERA_OK && (millis() - millisflasheototal) > 60000 ){

				this->FlaseoCabecera(false);

			}

		break;

		case false:
	

		break;

	}


}