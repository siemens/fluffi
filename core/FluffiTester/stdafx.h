/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Abian Blome, Michael Kraus
*/

#pragma once

#include "targetver.h"

#include <map>
#include <deque>
#include <iomanip>
#include <mutex>
#include <random>
#include <future>
#include <regex>
#include <experimental/filesystem>

#include "Ws2tcpip.h"

//Easylogging
#include "easylogging++.h"

//ZeroMQ
#include <zmq.hpp>
#include <zmq_addon.hpp>

//Protobuf
#include <FLUFFI.pb.h>

// File streams
#include <fstream>
#include <iostream>
#include <limits>

//MariaDB
#include <mysql.h>

// Headers for CppUnitTest
#include "CppUnitTest.h"
