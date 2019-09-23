# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Pascal Eckmann, Thomas Riedmaier

§§from distutils.dir_util import copy_tree
§§import subprocess
§§import code 
§§import copy
§§import requests
§§import json
§§import time
§§import sys
§§import os
§§import configparser
§§
§§url = "http://127.0.0.1:8080/api/v2/history/"
§§login = ('admin','admin')
§§lastCheck = False
§§lastHistory = False
§§lastDBCheck = False
§§
§§def deleteRequest(url, auth):
§§    try:
§§        response = requests.delete(url, auth = auth)
§§        time.sleep(0.3)
§§        return response
§§    except requests.exceptions.ConnectionError as err:
§§        print(err)
§§        time.sleep(1)
§§        sendRequest(url, auth)
§§
§§def getRequest(url, auth):
§§    try:
§§        response = requests.get(url, auth = auth)
§§        time.sleep(0.3)
§§        return response
§§    except requests.exceptions.ConnectionError as err:
§§        print(err)
§§        time.sleep(1)
§§        getRequest(url, auth)
§§
§§
§§print("Get History")
§§history = getRequest(url, login)
§§jsonResults = json.loads(history.text)
§§for result in jsonResults['results']:
    if result['mode'] == "cleanHistory.yml":
§§        if lastHistory:
§§            deleteRequest((url + str(result['id']) + "/"), login)
§§            print("delete cleanHistory " + str(result['id']))
§§        else:
§§            lastHistory = True
    if result['mode'] == "checkHostAlive.yml":
§§        if lastCheck:
§§            deleteRequest((url + str(result['id']) + "/"), login)
§§            print("delete checkHostAlive " + str(result['id']))
§§        else:
§§            lastCheck = True
    if result['mode'] == "manageAgents.yml":
§§        if lastDBCheck:
§§            deleteRequest((url + str(result['id']) + "/"), login)
            print("delete manageAgents " + str(result['id']))
§§        else:
§§            lastDBCheck = True
§§
§§
§§
§§
§§
