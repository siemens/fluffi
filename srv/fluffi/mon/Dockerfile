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

FROM ubuntu:bionic

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update \
&& apt-get install -y git wget curl screen htop net-tools netcat vim python3 python3-paho-mqtt python3-influxdb mosquitto

#RUN wget -qO- https://dl.google.com/go/go1.12.7.linux-amd64.tar.gz | tar xvz -C /usr/local

#RUN ln -s /usr/local/go/bin/go /usr/bin/go
#RUN ln -s /usr/local/go/bin/gofmt /usr/bin/gofmt
#RUN ln -s /usr/local/go/bin/godoc /usr/bin/godoc

#ENV GOPATH "/root/go"
#ENV PATH="/root/go/bin:${PATH}"
#ENV GO111MODULE="on"
#ENV GO111MODULE="auto"
#WORKDIR /root/go/pkg/mod/github.com/influxdata/influxdb@v1.7.7

WORKDIR /root
RUN wget -q https://dl.grafana.com/oss/release/grafana_6.2.5_amd64.deb \
&& wget -q https://dl.influxdata.com/influxdb/releases/influxdb_1.7.7_amd64.deb

RUN dpkg -i grafana_6.2.5_amd64.deb || true
RUN dpkg -i influxdb_1.7.7_amd64.deb || true

RUN apt -y -f install \
&& mkdir -p /etc/grafana/provisioning/jsons

ENTRYPOINT ["/run.sh", ""]

