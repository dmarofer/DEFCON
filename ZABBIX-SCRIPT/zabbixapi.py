############################################################################################################################################################
##
## Script Python para interrogar la API de Zabbix y enviar los datos por MQTT
## Desarrollado por Diego Maroto RDT Engineers
##
############################################################################################################################################################

## Ejecutar el script a gusto (a mano, o como servicio en el arranque del sistema)
## Es una pasarela. Se puede ejecutar en cualquier maquina, incluido el servidor de Zabbix o el servidor MQTT

## Necesario instalar los modulos necesarios en Python (si no dara error al importar estos namespaces)
from pyzabbix import ZabbixAPI
import json
import sys
import paho.mqtt.client as mqtt
import paho.mqtt.publish as publish
import time

# Autentificacion para el servidor MQTT
auth = {
  'username':"mosquitto_user",
  'password':"mosquitto_passwd"
}

# Api y auntentificacion del Zabbix
zapi = ZabbixAPI("https://zabbixserver:port/zabbix")
zapi.login("apiuser", "apipasswd")

zapi.timeout = 5
print("Connected to Zabbix API Version %s" % zapi.api_version())

while(True):

  # Leer la API
  try:

    disaster = int(zapi.trigger.get(only_true=1,countOutput=1,monitored=1,min_severity=5))
    high = int(zapi.trigger.get(only_true=1,countOutput=1,monitored=1,min_severity=4)) - disaster
    average = int(zapi.trigger.get(only_true=1,countOutput=1,monitored=1,min_severity=3)) - disaster - high
    warning = int(zapi.trigger.get(only_true=1,countOutput=1,monitored=1,min_severity=2)) - disaster - high - average

    # Publicar MQTT
    try:

      publish.single("cmnd/DEFCON/PROBLEMAS",
      payload=json.dumps({"DISASTER":disaster,"HIGH":high,"AVERAGE":average,"WARNING":warning}),
      hostname="mqtt_server",
      client_id="ZabbixAPI",
      auth=auth,
      port=1883,
      protocol=mqtt.MQTTv311)

    except:

      print ("Fallo publicando MQTT")

  except:

    print ("Fallo leyendo la API de Zabbix")


  # Esperar 30 segundos para repetir el proceso
  time.sleep(30)
