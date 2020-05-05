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
# Author(s): Junes Najah, Thomas Riedmaier, Abian Blome, Pascal Eckmann

import os
import socket

BASEDIR = os.path.abspath(os.path.dirname(__file__))
LOCAL_DEV = False
SQLALCHEMY_DATABASE_URI = "mysql://root:toor@localhost/fluffi_gm" if LOCAL_DEV else "mysql://fluffi_gm:fluffi_gm@" \
                                                                                    + socket.gethostbyname('db.fluffi')\
                                                                                    + "/fluffi_gm"
SQLALCHEMY_TRACK_MODIFICATIONS = False
WTF_CSRF_ENABLED = True
SECRET_KEY = "asdf"
BOOTSTRAP_SERVE_LOCAL = True
SEND_FILE_MAX_AGE_DEFAULT = 0


DBHOST = "localhost" if LOCAL_DEV else socket.gethostbyname('db.fluffi')
DBUSER = "root" if LOCAL_DEV else "fluffi_gm"
DBPASS = "toor" if LOCAL_DEV else "fluffi_gm"
DBFILE = "sql_files/createLMDB.sql"
DBPREFIX = "fluffi_"
DEFAULT_DBNAME = "information_schema"
FTP_URL = socket.gethostbyname('ftp.fluffi') if not LOCAL_DEV else 'ftp.fluffi'
MQTT_HOST = socket.gethostbyname('mon.fluffi') if not LOCAL_DEV else 'mon.fluffi'
MQTT_PORT = 1883
