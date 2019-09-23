# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Thomas Riedmaier, Roman Bendt

§§FROM ubuntu:bionic
§§
§§ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update -y
RUN apt-get upgrade -y
§§RUN apt-get dist-upgrade -y

§§RUN adduser --system --uid 1000 --shell /bin/bash --gecos 'fluffi builder' --group --disabled-password --home /home/builder builder
§§
RUN apt-get install -yq wget build-essential git
§§
§§RUN wget -qO- https://dl.google.com/go/go1.12.5.linux-amd64.tar.gz | tar xvz -C /usr/local
§§RUN ln -s /usr/local/go/bin/go /usr/bin/go
§§RUN ln -s /usr/local/go/bin/gofmt /usr/bin/gofmt
§§RUN ln -s /usr/local/go/bin/godoc /usr/bin/godoc
§§ 
CMD ["/bin/bash"]
