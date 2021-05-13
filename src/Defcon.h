#include <Arduino.h>
#include <NTPClient.h>					// Para la gestion de la hora por NTP
#include <WiFiUdp.h>					// Para la conexion UDP con los servidores de hora.
#include <Configuracion.h>				// Fichero de configuracion
#include <Adafruit_NeoPixel.h>			// Para los LED

#pragma once

class Defcon
{



private:
    
    bool HayQueSalvar;
	String mificheroconfig;

	unsigned long t_uptime;						// Para el tiempo que llevamos en marcha

    typedef void(*RespondeComandoCallback)(String comando, String respuesta);			// Definir como ha de ser la funcion de Callback (que le tengo que pasar y que devuelve)
	RespondeComandoCallback MiRespondeComandos = nullptr;								// Definir el objeto que va a contener la funcion que vendra de fuera AQUI en la clase.

	// Para almacenar Alias (referencia) al objeto tipo NTPClient para poder usar en la clase el que viene del Main
    NTPClient &ClienteNTP;

	// PARA LOS NEOPIXEL
	int pixelFormat = NEO_GRB + NEO_KHZ800;   

	// Un array que almacenara el color para cada bloque
	uint16_t HueBloque [6];
	// Uno para la sturacion
	uint8_t SaturacionBloque[6];
	// Otro array para almacenar los brillos para cada bloque
	//uint8_t BrilloBloque[6];
	// Array que almacena el numero del primer led del bloque
	uint16_t PrimerLed[6];
	// Array que almacena el numero del ultimo led de cada bloque
	uint16_t UltimoLed[6];

	// Funciones internas para manejo del tiempo
	unsigned long MillisDelta1;		// Variable para el almacenmiento del Millis
	void Delta1Begin();				// Apuntar los millis
	unsigned long GetDelta1();		// Devuelve la resta

	// Funcion de Vida para la maquina de estado del cambio de Defcon
	void MaquinaEstadoCambioDefconRun();

	// Variables internas para el estado DEFCON
	int DefconLevelActual;
	int DefconLevelFuturo;

	// Para la maquina de estado del proceso de cambio de Defcon
	enum TipoEstadoCambioDefcon {

		DEFCON_SIN_CAMBIOS,
		DEFCON_AVISANDO,
		DEFCON_PAUSA1,
		DEFCON_FADE_OFF,
		DEFCON_PAUSA2,
		DEFCON_FADE_ON,
		
	}Estado_Cambio_Defcon;

	// Contador de ciclos para los FADE. Vamos a hacer tantos ciclos como valga el valor de Brillo (0-255) en el tiempo que corresponda.
	uint8_t CuentaCiclosFade;

	// Para almacenar el numero de problemas que vienen de Zabbix (DISASTER, HIGH, AVERAGE, WARNING)
	uint8_t ProblemasZabbix[4];

	// Para el cambio de color y estado de la cabecera.
	void MaquinaEstadoCambioCabeceraRun();

	// Un contador para cambiar la luz de cabecera si estamos mucho tiempo sin recibir datos de Zabbiz
	unsigned long MillisRXDatos;

	// Para enviar la config a los topic stat
	void MandaConfig();

	// Variable para Silenciar los avisos de comunicaciones mal.
	bool SilencioComunicaciones;

	// Funcion para activar o desactivar el flaseo de la cabecera
	void FlaseoCabecera (bool l_flaseocabecera);
	
	// Variables Globales para el flasheo de la cabecera
	bool FlasheandoCabecera;
	unsigned long millisunflasheo;
	unsigned long millisflasheototal;
	uint8_t BrilloActual;

	// Maquina estado para el flasheo de la cabecera
	void MaquinaEstadoFlasheoCabecera();

public:

	// Para el estado de la cabecera
	enum TipoEstadosCabecera {

		CABECERA_SINRED,
		CABECERA_AP_MODE,
		CABECERA_SINMQTT,
		CABECERA_SIN_DATOS,
		CABECERA_OK,

	}Estado_Cabecera_Actual, Estado_Cabecera_Futuro;

    Defcon(String fich_config_Defcon, NTPClient& ClienteNTP);
    ~Defcon() {};

    //  Variables Publicas
	String HardwareInfo;											// Identificador del HardWare y Software
	bool ComOK;														// Si la wifi y la conexion MQTT esta OK
	
	// Funciones Publicas
	String MiEstadoJson(int categoria);								// Devuelve un JSON con los estados en un array de 100 chars (la libreria MQTT no puede con mas de 100)
	
	void TaskRun();													// A ejecutar en intervalo lento
	void RunFast();													// A ejecutar lo mas rapido posible

    void SetRespondeComandoCallback(RespondeComandoCallback ref);	// Definir la funcion para pasarnos la funcion de callback del enviamensajes
	boolean LeeConfig();
	boolean SalvaConfig();

	void Iniciar();
    
	void SetCabecera(Defcon::TipoEstadosCabecera l_Estado_Cabecera);
	
	void SetDefconLevel (int l_DefconLevel);							// Para acmabiar el estado de Defcon

	void SetBrillo (uint8_t l_brillo);

	void Problemas (String jsonproblemas);

	void PitaAvisoComKO();

	void SilenciaAvisoComunicaciones();

	void Aviso(int l_NumeroAviso);
		
};
