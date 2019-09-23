# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Junes Najah, Abian Blome, Thomas Riedmaier

import pymysql
from flask import Flask
from flask_bootstrap import Bootstrap
from flask_sqlalchemy import SQLAlchemy
from pytz import utc

pymysql.install_as_MySQLdb()

app = Flask(__name__)
Bootstrap(app)
app.config.from_object("config")
db = SQLAlchemy(app)

from .nav import nav

nav.init_app(app)

from app import views, models, controllers

from apscheduler.schedulers.background import BackgroundScheduler
from .health_check import healthCheck
from .helpers import deleteZipAndTempFiles
from config import LOCAL_DEV

scheduler = BackgroundScheduler()
scheduler.configure(timezone = utc)
scheduler.add_job(deleteZipAndTempFiles, 'interval', minutes=3)

if not LOCAL_DEV:
    scheduler.add_job(healthCheck, 'interval', minutes = 0.5)

scheduler.start()
