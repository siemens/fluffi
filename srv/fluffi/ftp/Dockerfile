# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Thomas Riedmaier, Fabian Russwurm, Roman Bendt

FROM ubuntu:bionic
§§
§§RUN apt-get update \
§§    && apt-get install -y --no-install-recommends vsftpd \
§§    && apt-get clean \
§§    && rm -rf /var/lib/apt/lists/*
§§
RUN adduser --disabled-password --gecos "" fluffi
§§
§§RUN mkdir -p /var/run/vsftpd/empty \
§§ && mkdir -p /etc/vsftpd \
§§ && mkdir -p /var/ftp \
§§ && mv /etc/vsftpd.conf /etc/vsftpd.orig
§§
CMD ["/run.sh"]
