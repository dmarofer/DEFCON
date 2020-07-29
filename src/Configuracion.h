
#define HOSTNAME "DefconRDT"

// Hola local con respecto a UTC
#define HORA_LOCAL 2

// Para el nombre del fichero de configuracion de comunicaciones
#define FICHERO_CONFIG_COM "/DefconCom.json"

// Para el nombre del fichero de configuracion del proyecto
#define FICHERO_CONFIG_PRJ "/DefconCfg.json"

// Tipo de cola (lib cppQueue)
#define	IMPLEMENTATION	FIFO

// TaskScheduler options:
//#define _TASK_TIMECRITICAL    // Enable monitoring scheduling overruns
#define _TASK_SLEEP_ON_IDLE_RUN // Enable 1 ms SLEEP_IDLE powerdowns between tasks if no callback methods were invoked during the pass 
#define _TASK_STATUS_REQUEST  	// Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only
//#define _TASK_WDT_IDS         // Compile with support for wdt control points and task ids
//#define _TASK_LTS_POINTER     // Compile with support for local task storage pointer
//#define _TASK_PRIORITY        // Support for layered scheduling priority
//#define _TASK_MICRO_RES       // Support for microsecond resolutionMM
//#define _TASK_DEBUG

// PINES
#define PINLEDS D5
#define PINBUZZER D6

// Numero total de LEDS
#define NUMEROLEDS 50

// Numero de leds de cada bloque
#define NUMLED0 20 // EL DEFCON
#define NUMLED5 6
#define NUMLED4 6
#define NUMLED3 6
#define NUMLED2 6
#define NUMLED1 6

// PARA EL COLOR. FORMATO HSV (HUE, SATURACION y BRILLO)

#define HUELED0 0               // Blanco asi que da igual porque bajaremos la saturacion a cero)
#define HUELED1 0               // Lo mismo
#define HUELED2 0               // ROJO
//#define HUELED3 10000           // NARANJA
#define HUELED3 32768           // Correccion hacia azul para hacerlo mas amarillo
#define HUELED4 22000           // VERDE
#define HUELED5 40000           // AZUL

#define SATLED0 0
#define SATLED1 0
#define SATLED2 255
#define SATLED3 150
#define SATLED4 200
#define SATLED5 0

// FRECUENCIA EN HZ DEL BERRIDO AL CAMBIO DEL DEFCON
#define FRECDEFCON 400

// TIMINGS en MS |__ TBUZZER __|__ TPAUSA1 __|__ TFADEOFF __|__ TPAUSA2 __|__ TFADEON__|
#define TBUZZER 2000
#define TPAUSA1 1000
#define TFADEOFF 255
#define TPAUSA2 1000
#define TFADEON 255