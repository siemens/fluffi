# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Junes Najah, Thomas Riedmaier, Roman Bendt

§§from influxdb import InfluxDBClient
§§
dbclient = InfluxDBClient('mon.fluffi', 8086, 'root', 'root', 'FLUFFI')
§§
§§def create_json_body(name, tags, now, value):
§§    json_body = {
§§        "measurement": name,
§§        "tags": tags,
§§        "time": now,
§§        "fields": {
§§            "value": value
§§        }
§§    }
§§
§§    return json_body    
§§
§§
§§def write(json_bodies):
§§    try:
§§        dbclient.write_points(json_bodies) 
§§        return "Wrote data to db client succesfully"
§§    except Exception as e:
§§        print(e)
§§        return "Failed to write data"
§§
§§def print_result(name):
§§    result = dbclient.query('select value from {};'.format(name))
§§    print("Result: {0}".format(result))
