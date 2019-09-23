# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Pascal Eckmann, Thomas Riedmaier, Roman Bendt

from distutils.dir_util import copy_tree
import subprocess
import code 
import copy
import requests
import json
import time
import sys
import os
import configparser

url = "http://127.0.0.1:8080/api/v2"
login = ('admin','admin')

# helper 
def sendRequestFromFile(url, filename, projectID=None, inventoryID=None):
    json_data=json.loads(open(filename).read())
    if projectID is not None:
        json_data['project'] = projectID
    if inventoryID is not None:
        json_data['inventory'] = inventoryID
    try:
        response = requests.post(url, json = json_data, auth=login)
        time.sleep(1)
        print(response)
        if(response.status_code < 300):
            jsonResult = json.loads(response.text)
            if 'id' in jsonResult:
                ID = jsonResult['id']
                return ID
        else:
            print("Error " + str(response.status_code) + ": " + response.text)
            sendRequestFromFile(url, filename, projectID, inventoryID)
    except requests.exceptions.ConnectionError as err:
        print(err)
        sendRequestFromFile(url, filename, projectID, inventoryID)

def sendRequest(url, json, auth):
    try:
        response = requests.post(url, json = json, auth = auth)
        return response
    except requests.exceptions.ConnectionError as err:
        print(err)
        sendRequest(url, json, auth)

# wait till API is available
print("Wait until polemarch is available")
while True:
    try:	
        response = requests.get(url,auth=login)
    except:
        print("Polemarch not available retrying...")
        time.sleep(0.5)
        continue
    if(response.status_code==200):
        print("Polemarch is now available")
        break
    print("Polemarch returns " + str(response.text) + "retrying...")

# Check if already initialized
url = "http://127.0.0.1:8080/api/v2/project/"
while True:
    try:	
        response = requests.get(url, auth=login)
        jsonResult = json.loads(response.text)
        if jsonResult['count'] > 0:
            print("Polemarch already initialized")
            sys.exit()
        break
    except:
        print("Polemarch not available retrying...")
        time.sleep(0.5)
        continue
    if(response.status_code==200):
        print("Polemarch is now available")
        break
    print("Polemarch returns " + str(response.text) + "retrying...")

# Create project
print("Add Project")
projectID = sendRequestFromFile("http://127.0.0.1:8080/api/v2/project/","requests/createProject.json")

# Create inventory and all hosts
print("Add Inventory")
with open('/usr/local/lib/python3.6/dist-packages/polemarch/projects/1/hosts', 'r') as myfile:
    inventar = myfile.read()
    jsonInventar = {"inventory_id": 1,"name": "Fluffi","raw_data": inventar }
    url = "http://127.0.0.1:8080/api/v2/project/1/inventory/import_inventory/"
    sendRequest(url, jsonInventar, login)

# Create periodic tasks
print("Add Periodic Task CheckHostAlive")
sendRequestFromFile("http://127.0.0.1:8080/api/v2/project/1/periodic_task/", "requests/addPeriodicTaskCheckHostsAlive.json", 1, 1)

print("Add Periodic Task CleanHistory")
sendRequestFromFile("http://127.0.0.1:8080/api/v2/project/1/periodic_task/", "requests/addPeriodicTaskCleanHistory.json", 1, 1)

print("Add Periodic Task ManageAgents")
sendRequestFromFile("http://127.0.0.1:8080/api/v2/project/1/periodic_task/", "requests/addPeriodicTaskManageAgents.json", 1, 1)

# send sync project command
print("Send Sync")
checkurl = "http://127.0.0.1:8080/api/v2/project/1/"
posturl = "http://127.0.0.1:8080/api/v2/project/1/sync/"
while True:
    try:	
        response = requests.post(posturl, auth=login)
    except:
        print("Polemarch not available retrying...")
        time.sleep(0.5)
        continue
    if(response.status_code==200):
        response = requests.get(checkurl, auth=login)
        if "OK" == json.loads(response.content)["status"]:
            break
        else:
            print("Polemarch not available retrying (2)...")
            time.sleep(0.5)

print("Initialization Finished!")
    #print("Polemarch returns " + str(response.text) + "retrying...")

