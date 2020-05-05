#!/bin/bash
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
# Author(s): Pascal Eckmann, Thomas Riedmaier

wget -O mqttcli-x86_64 https://github.com/shirou/mqttcli/releases/download/0.0.3/mqttcli_linux_amd64
wget -O mqttcli-armv7l https://github.com/shirou/mqttcli/releases/download/0.0.3/mqttcli_linux_arm
chmod +x mqttcli-x86_64
chmod +x mqttcli-armv7l
tar -cf dist.tar mqttcli-x86_64 mqttcli-armv7l fluffmonplugins
cat mon.sh dist.tar > fm.sh 
rm dist.tar
chmod +x fm.sh
zip DeployMonitoring.zip install.sh fm.sh
