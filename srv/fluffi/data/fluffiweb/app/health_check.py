# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Junes Najah, Thomas Riedmaier

from sqlalchemy import create_engine
from app.utils.ftp import FTPConnector
from app.utils.ansible import AnsibleRESTConnector
from .helpers import fluffiResolve

import config
import paho.mqtt.client as paho


def healthCheck():
    try:
        client = paho.Client()
        client.connect(config.MQTT_HOST, 1883)
        client.publish("FLUFFI/webui/db", payload = int(checkDbConnection()), qos = 2)
        client.publish("FLUFFI/webui/ftp", payload = int(checkFtpConnection()))
        client.publish("FLUFFI/webui/ansible", payload = int(checkAnsibleConnection()))
    except Exception as e:
        print(e)
        print("Failed to connect with mqtt!")


def checkDbConnection():
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (config.DBUSER, config.DBPASS, fluffiResolve(config.DBHOST), config.DEFAULT_DBNAME))
        connection = engine.connect()
        connection.close()
        engine.dispose()
        return True
    except Exception as e:
        print(e)
        return False


def checkFtpConnection():
    try:
        ftpConnector = FTPConnector(config.FTP_URL)
        ftpConnector.getListOfFilesOnFTPServer("")
        return True
    except Exception as e:
        print(e)
        return False


def checkAnsibleConnection():
    try:
        AnsibleRESTConnector("http://pole.fluffi:8888/api/v2/", "admin", "admin")
        return True
    except Exception as e:
        print(e)
        return False
