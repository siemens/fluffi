# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Junes Najah, Thomas Riedmaier

from flask import render_template
from datetime import datetime, timedelta

from app import app
from .nav import nav, registerElementDynamically

import shutil, os

# we overwrite the render function to add new elements to the navbar dynamically
_renderTemplate = render_template


def renderTemplate(*args, **kwargs):
    registerElementDynamically()

    return _renderTemplate(*args, nav = nav.elems, **kwargs)


# this is needed because sqlalchemy takes forever to do "engine_connect" if we give it a hostname
def fluffiResolve(possiblyHostname):
    import socket

    try:
        socket.inet_aton(possiblyHostname)
        # if we are here, this was a valid IP address
        return possiblyHostname
    except socket.error:
        # we need to resolve the hostname
        return socket.gethostbyname(possiblyHostname)


def formatSubtypeInput(formData, subtypes):
    formattedInput = ""

    for i, value in enumerate(formData):
        pipe = '' if i == len(formData) - 1 else '|'
        nameAndValue = subtypes[i] + "=" + value
        formattedInput += nameAndValue + pipe

    return formattedInput


def createDefaultSubtypes(subTypes):
    default = ""
    for i, t in enumerate(subTypes):
        if i == 0:
            default_count = "100"
        else:
            default_count = "0"
        if i == len(subTypes)-1:
            pipe = ""
        else:
            pipe = "|"
        default += t + "=" + default_count + pipe
    return default


def createDefaultSubtypesList(subTypes):
    default = []
    for i in range(0, len(subTypes)):
        if i == 0:
            default_count = "100"
        else:
            default_count = "0"
        default.append(default_count)
    return default
