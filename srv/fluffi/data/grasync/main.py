# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Junes Najah, Thomas Riedmaier, Pascal Eckmann

from sqlalchemy import create_engine, text
from influxdb import InfluxDBClient
import pymysql
import datetime, time

from influx import *
from queries import *

pymysql.install_as_MySQLdb()

SQLALCHEMY_DATABASE_URI_WITHOUT_DB="mysql://fluffi_gm:fluffi_gm@db.fluffi/"

### GET DATA FOR EVERY FUZZJOB AND WRITE TO INFLUX DB
while True:
    active_fuzzjobs = []
    json_bodies = []
    NOW = datetime.datetime.utcnow()

    ### GET ACTIVE FUZZJOBS
    try:
        fluffi_gm_engine = create_engine(SQLALCHEMY_DATABASE_URI_WITHOUT_DB + 'fluffi_gm')
        connection = fluffi_gm_engine.connect()

        result = connection.execute(RUNNING_PROJECTS)
        for row in result:
            if row[0]:
                active_fuzzjobs.append(row[0])
            else:
                print("There are no active fuzzjobs!")

        connection.close()
        fluffi_gm_engine.dispose()
    except:
        print("Failed to connect to " + SQLALCHEMY_DATABASE_URI_WITHOUT_DB + 'fluffi_gm')
        time.sleep(10)
        continue
    print(active_fuzzjobs)
    try:
        for fuzzjob in active_fuzzjobs:        
            fuzzjob_engine = create_engine(SQLALCHEMY_DATABASE_URI_WITHOUT_DB + 'fluffi_' + fuzzjob, connect_args={"init_command":"SET SESSION time_zone='+00:00'"})
            connection = fuzzjob_engine.connect()

            # Covered Blocks and Population Size            
            result = connection.execute(COUNT_COVERED_BLOCKS)
            for row in result:
                json_bodies.append(create_json_body(fuzzjob+'_covered_blocks', {}, NOW, row['COUNT(DISTINCT ModuleID, Offset)']))
            
            result = connection.execute(text(COUNT_TCTYPE), {'tcType': 0})
            for row in result:
                json_bodies.append(create_json_body(fuzzjob+'_population', {}, NOW, row['COUNT(*)']))

            # Unique Crashes and Violations
            result = connection.execute(text(UNIQUE_TESTCASES), {'tcType': 3})
            for row in result:
                json_bodies.append(create_json_body(fuzzjob+'_unique_crashes', {}, NOW, row['COUNT(*)']))

            result = connection.execute(text(UNIQUE_TESTCASES), {'tcType': 2})
            for row in result:
                json_bodies.append(create_json_body(fuzzjob+'_unique_violations', {}, NOW, row['COUNT(*)']))

            # System CPU Utilization & Combined Queue Size & Executions per second
            result = connection.execute(INSTANCE_PERFORMANCE_STATS)
            for row in result:
                time_of_status = row['TimeOfStatus']

                status_dict = dict((k.strip(), float(v.strip())) for k,v in (item.split(' ') for item in [s.strip() for s in row['Status'].split('|')] if item))
                host_and_port = row['ServiceDescriptorHostAndPort']
                server = host_and_port.split('.')[0] if '.' in host_and_port else ''
                if 'SysCPUUtil' in status_dict:
                    json_bodies.append(create_json_body(fuzzjob+'_sys_cpu_util', {'server': server}, time_of_status, status_dict['SysCPUUtil']))
                if 'TestCasesQueueSize' in status_dict:
                    json_bodies.append(create_json_body(fuzzjob+'_queue_size', {'hostAndPort': host_and_port}, time_of_status, status_dict['TestCasesQueueSize']))
                if 'TestEvaluationsQueueSize' in status_dict:
                    json_bodies.append(create_json_body(fuzzjob+'_evaluations_queue_size', {'hostAndPort': host_and_port}, time_of_status, status_dict['TestEvaluationsQueueSize']))
                if 'TestcasesPerSecond' in status_dict:
                    json_bodies.append(create_json_body(fuzzjob+'_tc_per_second', {'hostAndPort': host_and_port}, time_of_status, status_dict['TestcasesPerSecond']))

            # Accumulated Crashes
            result = connection.execute(text(COUNT_TCTYPE), {'tcType': 1})
            for row in result:
                json_bodies.append(create_json_body(fuzzjob+'_total_hangs', {}, NOW, row['COUNT(*)']))

            result = connection.execute(text(COUNT_TCTYPE), {'tcType': 2})
            for row in result:
                json_bodies.append(create_json_body(fuzzjob+'_total_violations', {}, NOW, row['COUNT(*)']))

            result = connection.execute(text(COUNT_TCTYPE), {'tcType': 3})
            for row in result:
                json_bodies.append(create_json_body(fuzzjob+'_total_crashes', {}, NOW, row['COUNT(*)']))                        

            connection.close()
            fuzzjob_engine.dispose()

        msg = write(json_bodies)
        print(msg) 
    except Exception as e:
            print(e)
           
    time.sleep(30)

