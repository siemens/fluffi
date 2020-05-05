#!/usr/bin/env python3
# Copyright 2017-2020 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including without
# limitation the rights to use, copy, modify, merge, publish, distribute,
# sublicense, and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
# SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
# 
# Author(s): Roman Bendt, Thomas Riedmaier
import paho.mqtt.client as mqtt
import datetime
import time
import math
from threading import Thread, Lock
from influxdb import InfluxDBClient

mutex = Lock()
cumulated_json = []
seenmachines = {}

def watchdog_push():
    global seenmachines, mutex, cumulated_json
    while True:
        time.sleep(5)
        now = datetime.datetime.utcnow()
        json_body = []
        for m, l in seenmachines.items():
            json_body.append({
                    "measurement": "lastseen",
                    "tags": {
                        "machine": m,
                    },
                    "time": now,
                    "fields": {
                        "value": math.ceil((now-l).total_seconds())
                    }
                })

        print("=========================== POSTING ACUMULATED DATA ===========================")
        sendingcopy = []
        with mutex:
            if len(json_body) is not 0:
                for i in json_body:
                    cumulated_json.append(i)
            sendingcopy = cumulated_json
            cumulated_json = []

        dbclient.write_points(sendingcopy)

def on_message(client, userdata, msg):
    global seenmachines, mutex, cumulated_json
    # FLUFFI/state/machine/measurement
    # FLUFFI/webui/measurement
    # FLUFFI/cmd/commandname
    info = msg.topic.split('/')[1:]
    # Use utc as timestamp
    receiveTime=datetime.datetime.utcnow()
    message=msg.payload.decode("utf-8")
    isfloatValue=False
    try:
        # Convert the string to a float so that it is stored as a number and not a string in the database
        val = float(message)
        isfloatValue=True
    except:
        print("Could not convert " + message + " to a float value")
        val = message
        isfloatValue=False

    print(str(receiveTime) + ": " + msg.topic + " " + str(val))

    json_body = []
    if info[0] == "cmd":
        # FLUFFI/cmd/forget-machine <machinename>
        if info[1] == "forget-machine":
            print("order to delete", message)
            with mutex:
                try:
                    del seenmachines[message]
                except KeyError:
                    pass
        else:
            pass

    elif info[0] == "state":
        if isfloatValue:
            machine = info[1]
            measurement = info[2]
            if ':' in measurement:
                mspl = measurement.split(':')
                measurement = mspl[0]
                machine += mspl[1]
            json_body = [
                {
                    "measurement": measurement,
                    "tags": {
                        "machine": machine,
                    },
                    "time": receiveTime,
                    "fields": {
                        "value": val
                    }
                }
            ]
            with mutex:
                seenmachines[info[1]] = receiveTime

    elif info[0] == "webui":
        if isfloatValue:
            submodule = info[0] # always webui
            measurement = info[1] 
            json_body = [
                {
                    "measurement": measurement,
                    "tags": {
                        "webui": submodule,
                    },
                    "time": receiveTime,
                    "fields": {
                        "value": val
                    }
                }
            ]
            with mutex:
                seenmachines['webui'] = receiveTime
    else:
        print("invalid module received")
        return

    if len(json_body) is not 0:
        with mutex:
            for i in json_body:
                cumulated_json.append(i)

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("FLUFFI/#",2)

def on_disconnect(client, userdata, rc=0):
    print("disconnected with result code ", str(rc))
    client.loop_stop()

# Set up a client for InfluxDB
dbclient = InfluxDBClient('localhost', 8086, 'root', 'root', 'FLUFFI')

# Initialize the MQTT client that should connect to the broker
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

# thread to collect and send data
t = Thread(target=watchdog_push)

conOK = False
while(conOK == False):
    try:
        client.connect("localhost", 1883, 5)
        conOK = True
    except:
        conOK = False
    time.sleep(2)

# start collector
t.start()

# loop to the broker
client.loop_start()

while True:
    time.sleep(5)

