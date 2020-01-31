# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Thomas Riedmaier

FROM ubuntu:bionic

# Install dependencies
RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -yq python3-virtualenv python3.6 python3.6-dev python3-pip gcc libffi-dev libkrb5-dev libffi6 libssl-dev libyaml-dev libsasl2-dev libldap2-dev sshpass git redis-server openssh-client sshpass mysql-client vim screen tree htop net-tools iputils-ping wget curl

# Install polemarch and everything we need in python
RUN pip3 install -U django==2.2.3 polemarch==1.3.1 polemarch-ansible==1.3.2 django-redis==4.10.0 requests==2.22.0 mysql-connector==2.2.9



#Prepare polemarch environment
RUN ln -s /usr/bin/python3 /usr/bin/python
RUN mkdir -p /tmp/polemarch/logs
RUN mkdir -p /tmp/polemarch/pid
RUN mkdir -p /root/.ssh
RUN ln -fs /dev/null /root/.ssh/known_hosts

# kill community nav item, since it DOSes our installation in a separate network
RUN echo ' li[data-url="/community_template"] { visibility:hidden; height:0px;}' >> /usr/local/lib/python3.6/dist-packages/polemarch/static/css/polemarch-gui.css



