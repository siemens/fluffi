# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Junes Najah, Thomas Riedmaier

import datetime
import json
import config
from app import app

from app.utils.ansible import AnsibleRESTConnector
from app.utils.ftp import FTPConnector


def getConfigJson():
    with open(app.root_path + '/static/config.json') as f:
        data = json.load(f)
    return data


JSON_CONFIG = getConfigJson()
FLUFFI_VERSION = "1.0"
FOOTER_SIEMENS = "Siemens AG, Corporate Technology {} | FLUFFI {}".format(datetime.datetime.now().year, FLUFFI_VERSION)
TESTCASE_TYPES = {
    "population": 0,
    "hangs": 1,
    "accessViolations": 2,
    "crashes": 3,
    "noResponses": 4,
    "minimized": 5
}

FTP_CONNECTOR = FTPConnector(config.FTP_URL)
ANSIBLE_REST_CONNECTOR = AnsibleRESTConnector("http://pole.fluffi:8888/api/v2/", "admin", "admin")
