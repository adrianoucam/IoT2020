# from turtle import position
from fastapi import FastAPI
from paho import mqtt
from fastapi_mqtt import FastMQTT, MQTTConfig
import re
import haversine as hs
from haversine import Unit
import os
import psutil 
import sqlite3



def is_open(path):
    for proc in psutil.process_iter():
        try:
            files = proc.get_open_files()
            if files:
                for _file in files:
                    if _file.path == path:
                        return True    
        except psutil.NoSuchProcess as err:
            print(err)
    return False


con = sqlite3.connect('/home/ubuntu/registro.db')

path = os.path.abspath('registro.db')

#print(is_open(path))
#con.close()
#print(is_open(path))
#cur = con.cursor()



mqtt_broker = 'broker.mqttdashboard.com'
mqtt_port = 1883
mqtt_topic = "arduino/LED"
FILE_NAME = "data.txt"

app = FastAPI()

fast_mqtt = FastMQTT(config=MQTTConfig(host = mqtt_broker, port= mqtt_port, keepalive = 60))

fast_mqtt.init_app(app)


# Mensagem de conecção
@fast_mqtt.on_connect()
def connect(client, flags, rc, properties):
    # fast_mqtt.client.subscribe("/mqtt") #subscribing mqtt topic 
    print("Connected: ", client, flags, rc, properties)

# Quando uma mensagem é postada no tópíco
@fast_mqtt.on_message()
async def message(client, topic, payload, qos, properties):
    print("Received message: ",topic, payload.decode(), qos, properties)
    return 0


# Mensagem de desconexão
@fast_mqtt.on_disconnect()
def disconnect(client, packet, exc=None):
    print("Disconnected")

# Mensagem de subscrição
@fast_mqtt.on_subscribe()
def subscribe(client, mid, qos, properties):
    print("subscribed", client, mid, qos, properties)



@app.get("/getdatasqlite")
async def get_data():
    sql_str = "SELECT id,data,portaria,datahora FROM registro"
    rows = []
    for row in cur.execute(sql_str):
        rows.append({
            "id": row[0],
            "chave": row[1],
            "portaria": row[2],
            "datahora": row[3],
        })    
    return rows

@fast_mqtt.subscribe(mqtt_topic)
async def get_topic_data(client, topic, payload, qos, properties):
    print("data: ", topic, payload.decode(), qos, properties)
    text_file = open(FILE_NAME, "a+")
    n = text_file.write(payload.decode()+"\n")
    text_file.close()
    latlng = payload.decode().split(",")
    sql_str = "INSERT INTO registro (data,portaria)  VALUES ('{}','{}')".format(latlng[0], latlng[1])


@app.get("/getdata")
async def get_data():
    mf = open(FILE_NAME, "r+")
    file_content = mf.readlines()
    # return file_content

    ret_pos = []

    for line in file_content:
        aux = line.replace("\n","")
        aux = aux.split(",")
        ret_pos.append({
            "chave": aux[0],
            "portaria": aux[1],
        })

    return ret_pos



@app.get("/teste")
async def teste():
    return {"data": 25, "idade":90}



