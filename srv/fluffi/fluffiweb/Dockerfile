# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Thomas Riedmaier, Fabian Russwurm, Junes Najah, Roman Bendt

FROM tiangolo/uwsgi-nginx-flask:python3.6-alpine3.7
§§
# Install mariadb-client
RUN apk --no-cache --no-progress upgrade && \
    apk --no-cache --no-progress add mariadb-client

§§#COPY customx.conf /etc/nginx/conf.d/
§§ENV FLASK_APP main.py
§§ENV UWSGI_INI /app/uwsgi.ini
§§ENV FLASK_DEBUG 1
ENV STATIC_PATH /app/app/static
§§
§§COPY requirements.txt /tmp/
§§
§§#RUN pip3 install -U pip
§§RUN pip3 install -r /tmp/requirements.txt

