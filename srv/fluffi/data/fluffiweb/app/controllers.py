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
# Author(s): Junes Najah, Pascal Eckmann, Michael Kraus, Abian Blome, Thomas Riedmaier

import io
import csv
import subprocess
import time
import re
from base64 import b64encode
from os import system, unlink

from flask import abort
from sqlalchemy import *

from app import db, models
from .constants import *
from .helpers import *
from .queries import *





def createNewDatabase(name):
    dbName = config.DBPREFIX + name
    sqlFile = open(os.path.join(app.root_path, config.DBFILE), "r")
    sqlFileData = sqlFile.read()
    sqlFile.close()
    encodedDbName = b64encode(bytes(dbName, 'utf-8')).decode("utf-8").replace('=', '')
    # replace names
    sqlFileData = sqlFileData.replace("fluffi", dbName)

    if not os.path.exists("/tmp"):
        os.makedirs("/tmp")
    with open("/tmp/create_%s" % encodedDbName, 'w') as tempDBFile:
        tempDBFile.write(sqlFileData)

    # if os is windows (test dev)
    if os.name == 'nt':
        mysqlCommand = "Get-Content '/tmp/create_%s' | mysql -h '%s' -u '%s' -p'%s'" % (
            encodedDbName, fluffiResolve(config.DBHOST), config.DBUSER, config.DBPASS)
        p = subprocess.Popen(["powershell.exe", mysqlCommand])
        p.communicate()
    else:
        mysqlCommand = "mysql -h '%s' -u '%s' -p'%s' < '/tmp/create_%s'" % (
            fluffiResolve(config.DBHOST), config.DBUSER, config.DBPASS, encodedDbName)
        if system(mysqlCommand) != 0:
            print('Could not execute mysql command')

    unlink("/tmp/create_%s" % encodedDbName)
    project = models.Fuzzjob(name = name, DBHost = config.DBHOST, DBUser = config.DBUSER, DBPass = config.DBPASS,
                             DBName = dbName)

    return project


def listFuzzJobs():     
    try:
        projects = models.Fuzzjob.query.all()
    except Exception as e:
        print("Database connection failed! Make sure the hostname db.fluffi is available with user fluffi_gm. " + str(e))   
        projects = []

    for project in projects:        
        try:
            engine = create_engine(
                'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
            connection = engine.connect()
        
            result = connection.execute(COMPLETED_TESTCASES_COUNT)
            countOfCompletedTestcases = result.fetchone()[0]            

            result = connection.execute(NUMBER_OF_NO_LONGER_LISTED)
            numberOfNoLongerListed = result.fetchone()[0]
            project.testcases = countOfCompletedTestcases + numberOfNoLongerListed

            result = connection.execute(getITCountOfTypeQuery(0))
            project.numPopulation = result.fetchone()[0]

            result = connection.execute(getITCountOfTypeQuery(5))
            project.numMinimizedPop = result.fetchone()[0]

            result = connection.execute(getITCountOfTypeQuery(1))
            project.numHang = result.fetchone()[0]

            result = connection.execute(NUM_UNIQUE_ACCESS_VIOLATION)
            project.numUniqueAccessViolation = result.fetchone()[0]

            result = connection.execute(getITCountOfTypeQuery(4))
            project.numNoResponse = result.fetchone()[0]

            result = connection.execute(NUM_UNIQUE_CRASH)
            project.numUniqueCrash = result.fetchone()[0]

            result = connection.execute(getMICountOfTypeQuery(2))
            project.numTE = result.fetchone()[0]

            result = connection.execute(getMICountOfTypeQuery(1))
            project.numTR = result.fetchone()[0]

            result = connection.execute(getMICountOfTypeQuery(0))
            project.numTG = result.fetchone()[0]

            result = connection.execute(GET_CHECKED_RATING)
            project.checkRating = len(result.fetchall()) == 0            

            project.numLM = int(models.Localmanagers.query.filter_by(Fuzzjob = project.ID).count())
        except Exception as e:
            print(e)
            project.status = "Unreachable"
            project.testcases = "-"
            project.numPopulation = "-"
            project.numMinimizedPop = "-"
            project.numHang = "-"
            project.numUniqueAccessViolation = ""
            project.numAccessViolation = "-"
            project.numException = "-"
            project.numNoResponse = "-"
            project.numUniqueCrash = "-"
            project.checkRating = False
            project.numLM = 0
            project.numTE = "-"
            project.numTR = "-"
            project.numTG = "-"          
        finally:
            connection.close()
            engine.dispose()

    return projects


def getLocations():    
    try:
        locations = db.session.query(models.Locations)
    except:
        print("Please check your database connection and make sure the hostname db.fluffi is available with user fluffi_gm.")
        locations = []

    return locations


def getDownloadPath():
    return app.root_path[:-3]


def getLocationFormWithChoices(projId, locationForm):
    choices = []
    locationFuzzJobs = models.LocationFuzzjobs.query.filter_by(Fuzzjob = projId).all()
    locationFuzzJobsIds = [f.Location for f in locationFuzzJobs]

    for location in db.session.query(models.Locations):
        if location.ID not in locationFuzzJobsIds:
            choices.append(location.Name)

    locationForm.location.choices = choices

    return locationForm


def getProject(projId):        
    try:
        project = models.Fuzzjob.query.filter_by(ID = projId).first()
        
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
    
        result = connection.execute(COMPLETED_TESTCASES_COUNT)
        countOfCompletedTestcases = result.fetchone()[0]
        
        result = connection.execute(NUMBER_OF_NO_LONGER_LISTED)
        numberOfNoLongerListed = result.fetchone()[0]
        project.testcases = countOfCompletedTestcases + numberOfNoLongerListed

        result = connection.execute(getITCountOfTypeQuery(0))
        project.numPopulation = result.fetchone()[0]
        
        result = connection.execute(getLatestTestcaseOfType(0))
        dateTimeOfLatestPopulation = result.fetchone()[0]        
        timeOfLatestPopulation = dateTimeOfLatestPopulation.strftime("%H:%M - %d.%m.%Y")      
        project.timeOfLatestPopulation = timeOfLatestPopulation

        result = connection.execute(getITCountOfTypeQuery(5))
        project.numMinimizedPop = result.fetchone()[0]

        result = connection.execute(getITCountOfTypeQuery(2))
        project.numTotalAccessViolation = result.fetchone()[0]

        result = connection.execute(NUM_UNIQUE_ACCESS_VIOLATION)
        project.numUniqueAccessViolation = result.fetchone()[0]

        result = connection.execute(getITCountOfTypeQuery(3))
        project.numCrash = result.fetchone()[0]

        result = connection.execute(NUM_UNIQUE_CRASH)
        project.numUniqueCrash = result.fetchone()[0]

        result = connection.execute(getITCountOfTypeQuery(1))
        project.numHang = result.fetchone()[0]

        result = connection.execute(getITCountOfTypeQuery(3))
        project.numException = result.fetchone()[0]

        result = connection.execute(getITCountOfTypeQuery(4))
        project.numNoResponse = result.fetchone()[0]

        result = connection.execute(NUM_BLOCKS)
        project.numBlocks = result.fetchone()[0]

        result = connection.execute(getMICountOfTypeQuery(2))
        project.numTE = result.fetchone()[0]

        result = connection.execute(getMICountOfTypeQuery(1))
        project.numTR = result.fetchone()[0]

        result = connection.execute(getMICountOfTypeQuery(0))
        project.numTG = result.fetchone()[0]

        result = connection.execute(GET_TOTAL_CPU_SECONDS)
        totalCPUSeconds = result.fetchone()[0]
        project.totalCPUHours = round(totalCPUSeconds / 3600)

        result = connection.execute(GET_CHECKED_RATING)
        project.checkRating = len(result.fetchall()) == 0        

        result = connection.execute(GET_SETTINGS)
        project.settings = []
        config = getConfigJson()

        for row in result:
            setting = type('', (), {})()
            setting.name = row["SettingName"]
            setting.value = row["SettingValue"]
            if setting.name == "runnerType":
                setting.options = [x for x in config["RunnerTypeOptions"] if x != setting.value]
            elif setting.name == "generatorTypes":
                setting.mutatorTypesIDs = config["GeneratorTypes"]
            elif setting.name == "evaluatorTypes":
                setting.evaluatorTypesIDs = config["EvaluatorTypes"]
            setting.ID = row["ID"]
            project.settings.append(setting)

        project.possibleSettingNames = [sn for sn in config["DefaultSettingNames"] if
                                        sn not in [s.name for s in project.settings]]
        result = connection.execute(GET_TARGET_MODULES)
        project.modules = []

        for row in result:
            module = type('', (), {})()
            module.name = row["ModuleName"]
            module.path = row["ModulePath"]
            module.ID = row["ID"]
            project.modules.append(module)

        project.locations = db.session.query(models.LocationFuzzjobs, models.Locations.Name, models.Locations.ID).filter_by(
            Fuzzjob = projId).outerjoin(models.Locations)
        project.numLM = models.Localmanagers.query.filter_by(Fuzzjob = projId).count()
        
        project.status = "Reachable"
    except Exception as e:
        print(e)
        project.status = "Unreachable"
        project.testcases = "-"
        project.numPopulation = "-"
        project.numTotalAccessViolation = "-"
        project.numUniqueAccessViolation = "-"
        project.timeOfLatestPopulation = "-"
        project.numMinimizedPop = "-"
        project.numHang = "-"
        project.numCrash = "-"
        project.numUniqueCrash = "-"
        project.numAccessViolation = "-"
        project.numException = "-"
        project.numNoResponse = "-"
        project.numBlocks = "-"
        project.numTE = "-"
        project.numTG = "-"
        project.numTR = "-"
        project.settings = []
        project.modules = []
        project.checkRating = False
        project.totalCPUHours = "-"
        project.locations = []
        project.numLM = 0
    finally:
        connection.close()
        engine.dispose()    

    return project


def getProjects():
    try:
        projects = models.Fuzzjob.query.all()
    except Exception as e:
        print(e)
        projects = []

    for project in projects:        
        try:
            engine = create_engine(
                'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
            connection = engine.connect()
            resultDirty = connection.execute(GET_PROJECTS)
            result = resultDirty.fetchall()[0]
            project.testcases = result[0] + result[1]
            project.numPopulation = result[2]
            project.numHang = result[3]
            project.numAccessViolation = result[4]
            project.numException = result[5]
            project.numNoResponse = result[6]
            project.checkRating = result[7] is None            
            project.status = "Reachable"
        except Exception as e:
            print(e)
            project.status = "Unreachable"
            project.testcases = "-"
            project.numPopulation = "-"
            project.numHang = "-"
            project.numAccessViolation = "-"
            project.numException = "-"
            project.numNoResponse = "-"
            project.checkRating = False
        finally:
            connection.close()
            engine.dispose()

    return projects


def updateInfoHandler(projId, infoType):
    msg, info, status = "", "", ""
        
    try:
        project = models.Fuzzjob.query.filter_by(ID=projId).first()
    
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        
        if infoType == "timeOfLatestPopulation":        
            result = connection.execute(getLatestTestcaseOfType(0))
            dateTimeOfLatestPopulation = result.fetchone()[0]               
            info = dateTimeOfLatestPopulation.strftime("%H:%M - %d.%m.%Y")  
        # other infos can be added here
        # else: 
        
        msg = "Success"    
        status = "OK" 
    except Exception as e:
        print(e)
        info = ""
        msg = "Failed to get updated info!"
        status = "ERROR" 
    finally:
        connection.close()
        engine.dispose()           
    
    return msg, info, status

def getGeneralInformationData(projId, stmt):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    data = type('', (), {})()
    data.testcases = []
    data.project = project

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        result = connection.execute(stmt)
        for row in result:
            testcase = type('', (), {})()
            testcase.testcaseID = row["ID"]
            if "CrashFootprint" in row and row["CrashFootprint"] is not None:
                    testcase.footprint = row["CrashFootprint"]
            testcase.ID = "{}:{}".format(row["CreatorServiceDescriptorGUID"], row["CreatorLocalID"])
            testcase.rating = row["Rating"]
            
            if row["NiceName"] is None:
                testcase.triggerInsert = True
                if row["NiceNameMI"] is None:
                    testcase.niceName = "{}:{}".format(row["CreatorServiceDescriptorGUID"], row["CreatorLocalID"])                
                else:
                    testcase.niceName = "{}:{}".format(row["NiceNameMI"], row["CreatorLocalID"])                    
            else:
                testcase.triggerInsert = False
                testcase.niceName = row["NiceName"]
                
            testcase.timeOfInsertion = row["TimeOfInsertion"]
            data.testcases.append(testcase)
    except Exception as e:
        print(e)
        pass
    finally:
        connection.close()
        engine.dispose()

    return data


def getRowCount(projId, stmt, params=None):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    data = 0

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        result = connection.execute(text(stmt), params) if params else connection.execute(stmt)
        data = result.fetchone()[0]
    except Exception as e:
        print(e)
        pass
    finally:
        connection.close()
        engine.dispose()

    return data


def getResultOfStatement(projId, stmt, params=None):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    result = None

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        result = connection.execute(text(stmt), params) if params else connection.execute(stmt)
    except Exception as e:
        print(e)
        pass
    finally:
        connection.close()
        engine.dispose()

    return result


def getSettingArchitecture(projId): 
    settingArch = ""
       
    result = getResultOfStatement(projId, GET_RUNNERTYPE)    
    if result is not None:
        try:                         
            runnerType = result.fetchone()[0]
            runnerTypeLowerCase = runnerType.lower()
        except:
            runnerType = ""
            runnerTypeLowerCase = ""
                    
        if "x64" in runnerTypeLowerCase:
            settingArch = "x64"
        elif "x86" in runnerTypeLowerCase:
            settingArch = "x86"
        elif "arm32" in runnerTypeLowerCase:
            settingArch = "arm32"
        elif "arm64" in runnerTypeLowerCase:
            settingArch = "arm64"
        
    return settingArch
    

def getResultOfStatementForGlobalManager(stmt, params=None):
    result = None

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (config.DBUSER, config.DBPASS, fluffiResolve(config.DBHOST), "fluffi_gm"))
    connection = engine.connect()
    try:
        result = connection.execute(text(stmt), params) if params else connection.execute(stmt)
    except Exception as e:
        print(e)
        pass
    finally:
        connection.close()
        engine.dispose()

    return result


def insertOrUpdateNiceName(projId, myGUID, myLocalID, newName, command, elemType):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    
    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        if elemType == "testcase":
            data = {"guid": myGUID, "localId": myLocalID, "newName": newName}
            statement = text(INSERT_NICE_NAME_TESTCASE) if command == "insert" else text(UPDATE_NICE_NAME_TESTCASE)
            connection.execute(statement, data)
        elif elemType == "managedInstance":
            data = {"sdguid": myGUID, "newName": newName}
            statement = text(INSERT_NICE_NAME_MANAGED_INSTANACE) if command == "insert" else text(
                UPDATE_NICE_NAME_MANAGED_INSTANACE)
            connection.execute(statement, data)
        else:
            msg, status = "Error: Invalid elemType in insertOrUpdateNiceName(...)", "error"
            return msg, status

        msg, status = "Testcase {} successful".format(command), "OK"
    except Exception as e:
        print(e)
        msg, status = "Error: Could not {} testcase".format(command), "error"
    finally:
        connection.close()
        engine.dispose()

    return msg, status


def getLocation(locId):
    location = type('', (), {})()
    location.projects = []
    location.ID = locId
    location.name = db.session.query(models.Locations.Name).filter_by(ID = locId).first()[0]

    for l in models.LocationFuzzjobs.query.filter_by(Location = locId).all():
        currentFuzzjob = models.Fuzzjob.query.filter_by(ID = l.Fuzzjob).first()
        location.projects.append(currentFuzzjob)
    
    location.workers = models.Workers.query.filter(models.Workers.Location == locId, models.Workers.Fuzzjob == None, models.Workers.Agenttype != 4).all()
    return location


def getLocalManager(projId):
    localManagers = []
    project = models.Fuzzjob.query.filter_by(ID = projId).first()

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), "fluffi_gm"))
    connection = engine.connect()
    try:
        resultLM = connection.execute(text(GET_LOCAL_MANAGERS), {"fuzzjobID": projId})

        for row in resultLM:
            localManager = {"ServiceDescriptorGUID": row["ServiceDescriptorGUID"],
                            "ServiceDescriptorHostAndPort": row["ServiceDescriptorHostAndPort"]}
            localManagers.append(localManager)

    except Exception as e:
        print(e)
        pass
    finally:
        connection.close()
        engine.dispose()

    return localManagers


def getManagedInstancesAndSummary(projId):
    """ 
    New data in the status string will be added to the managedInstances automatically
    They only need to be added to viewManagedInstances.html
    """
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    managedInstances = {"instances": [], "project": project}
    localManagers = []
    summarySection = {}
    numOfRTT = 0
    sumOfAverageRTT = 0

    engineOne = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connectionOne = engineOne.connect()
    engineTwo = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), "fluffi_gm"))
    connectionTwo = engineTwo.connect()
    try:
        resultMI = connectionOne.execute(GET_MANAGED_INSTANCES)
        resultLM = connectionTwo.execute(text(GET_LOCAL_MANAGERS), {"fuzzjobID": projId})


        for row in resultLM:
            kill = 1 if models.CommandQueue.query.filter_by(Argument = row["ServiceDescriptorHostAndPort"],
                                                            Done = 0).first() else 0
            localManager = {"ServiceDescriptorGUID": row["ServiceDescriptorGUID"],
                            "ServiceDescriptorHostAndPort": row["ServiceDescriptorHostAndPort"], "kill": kill}
            localManagers.append(localManager)

        columnNames = resultMI.keys()

        for row in resultMI:
            instance = {}
            for cn in columnNames:
                instance[cn] = row[cn]

            if instance["ServiceDescriptorHostAndPort"] is not None:
                instance["kill"] = 0 if models.CommandQueue.query.filter_by(
                    Argument = instance["ServiceDescriptorHostAndPort"], Done = 0).first() is None else 1
            if instance["Status"] is not None:
                keyValueStatuses = [s.strip() for s in instance["Status"].split('|')]
                parsedStatuses = dict((k.strip(), float(v.strip())) for k, v in
                                      (item.split(' ') for item in keyValueStatuses if len(item) > 0))
                for statusKey in parsedStatuses:
                    if statusKey == "AverageRTT":
                        instance[statusKey] = round(parsedStatuses[statusKey])
                        sumOfAverageRTT += instance[statusKey]
                        numOfRTT += 1
                    elif statusKey == "SysCPUUtil" or statusKey == "ProcCPUUtil":
                        instance[statusKey] = parsedStatuses[statusKey]
                    else:
                        instance[statusKey] = round(parsedStatuses[statusKey])
                        key = "SumOf" + statusKey
                        if key in summarySection:
                            summarySection[key] += instance[statusKey]
                        else:
                            summarySection[key] = instance[statusKey]
            managedInstances["instances"].append(instance)
    except Exception as e:
        print(e)
        pass
    finally:
        connectionOne.close()
        engineOne.dispose()
        connectionTwo.close()
        engineTwo.dispose()

    managedInstances["instances"] = sorted(managedInstances["instances"], key = lambda k: k["AgentType"])
    average = sumOfAverageRTT / numOfRTT if numOfRTT != 0 else 0
    summarySection['AverageRTT'] = round(average, 1)

    return managedInstances, summarySection, localManagers


def getViolationsAndCrashes(projId):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    violationsAndCrashes = type('', (), {})()
    violationsAndCrashes.testcases = []
    violationsAndCrashes.project = project

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        result = connection.execute(GET_VIOLATIONS_AND_CRASHES)

        for row in result:
            testcase = type('', (), {})()
            testcase.count = row["count(*)"]
            testcase.footprint = row["CrashFootprint"]
            testcase.type = row["TestCaseType"]
            violationsAndCrashes.testcases.append(testcase)
    except Exception as e:
        print(e)
        pass
    finally:
        connection.close()
        engine.dispose()

    return violationsAndCrashes


def updateModuleBinaryAndPath(projId, moduleId, formData, rawBytesData):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()

    if project is not None:
        try:
            engine = create_engine(
                'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
            connection = engine.connect()
            statement = "UPDATE target_modules SET "

            for key in formData:
                if formData[key]:
                    statement += "{}='{}', ".format(key, formData[key])

            if rawBytesData["RawBytes"]:
                statement += "RawBytes=:RawBytes WHERE ID={};".format(moduleId)
                statement = text(statement)
                connection.execute(statement, rawBytesData)
            else:
                statement = statement[:-2] + " WHERE ID={};".format(moduleId)
                connection.execute(statement)
        except Exception as e:
            print(e)
            return "Error: Could not edit module", "error"
        finally:
            connection.close()
            engine.dispose()
    else:
        return "Error: Could not find project", "error"

    return "Edit module was successful", "success"


def insertSettings(projId, request, settingForm):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()

    if project is not None:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        try:
            if len(settingForm.option_module.data) > 0 and len(settingForm.option_module_value.data) > 0:
                data = {"SettingName": settingForm.option_module.data,
                        "SettingValue": settingForm.option_module_value.data}
                statement = text(INSERT_SETTINGS)
                connection.execute(statement, data)
            else:
                return "Error: Name or value cannot be empty!", "error"
            # for multiple rows in settingForm
            f = request.form
            for key in f.keys():
                if '_optionname' in key:
                    if len(f.getlist(key)[0]) > 1:
                        optionNum = key.split("_")[0]
                        settingName = request.form.get(str(optionNum) + '_optionname')
                        settingValue = request.form.get(str(optionNum) + '_optionvalue')
                        data = {"SettingName": settingName, "SettingValue": settingValue}
                        statement = text(INSERT_SETTINGS)
                        connection.execute(statement, data)

        except Exception as e:
            print(e)
            return "Error: Could not add setting", "error"
        finally:
            connection.close()
            engine.dispose()
    else:
        return "Error: Could not find project", "error"

    return "Added setting", "success"


def uploadNewTarget(targetFile):
    try:
        targetFileData = targetFile.read()
        targetFileName = targetFile.filename
        FTP_CONNECTOR.saveTargetFileOnFTPServer(targetFileData, targetFileName)
        return "Uploaded new Target!", "success"
    except Exception as e:
        print(e)
        return "Error: Failed saving Target on FTP Server", "error"


def setNewBasicBlocks(targetFile, projId):
    basicBlocks = []
    targetFile.seek(0)
    data = targetFile.read()
    reader = csv.reader(data.decode('utf-8').splitlines())
    for row in reader:
        basicBlocks.append(row)

    project = models.Fuzzjob.query.filter_by(ID = projId).first()

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        targetModulesData = connection.execute(GET_TARGET_MODULES)
        targetModules = []
        for row in targetModulesData:
            targetModules.append([row[1], row[0]])

        for row in basicBlocks:
            for targetModule in targetModules:
                if row[0] == targetModule[0]:
                    row[0] = targetModule[1]
                    data = {"ModuleID": row[0], "Offset": int(row[1],0)}
                    statement = text(INSERT_BLOCK_TO_COVER)
                    connection.execute(statement, data)

    except Exception as e:
        print(e)
        return "Error: Failed to add new BasicBlocks to database", "error"
    finally:
        connection.close()
        engine.dispose()

    print(targetFile.filename)
    return "Added new BasicBlocks to database", "success"


def executeResetFuzzjobStmts(projId, deletePopulation):
    project = getProject(projId)

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        for stmt in ResetFuzzjobStmts:
            connection.execute(stmt)

        if deletePopulation:
            connection.execute(DELETE_TESTCASES)
            connection.execute(RESET_RATING)
        else:
            connection.execute(DELETE_TESTCASES_WITHOUT_POPULATION)
            connection.execute(RESET_RATING)

        return "Reset Fuzzjob was successful", "success"
    except Exception as e:
        print(e)
        return "Error: Failed to reset fuzzjob", "error"
    finally:
        connection.close()
        engine.dispose()


def insertModules(projId, request):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()

    if project is not None:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        try:
            if 'targetModules' in request.files:
                for module in request.files.getlist("targetModules"):
                    if module.filename:
                        data = {"ModuleName": module.filename, "ModulePath": "*", "RawBytes": module.read()}
                        statement = text(INSERT_MODULE)
                        connection.execute(statement, data)
                    else:
                        return "Error: Filename cannot be empty", "error"
        except Exception as e:
            print(e)
            return "Error: Failed to add module", "error"
        finally:
            connection.close()
            engine.dispose()
        return "Added Module(s)", "success"
    else:
        return "Error: Project not found", "error"


def createByteIOTestcase(projId, testcaseId):
    filename = ""
    (guid, localId) = testcaseId.split(":")
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    byteIO = io.BytesIO()

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        data = {"guid": guid, "localId": localId}
        statement = text(GET_NN_TESTCASE_RAWBYTES)
        result = connection.execute(statement, data)

        for row in result:
            filename = row["NiceName"] if row["NiceName"] else testcaseId
            rawBytes = row["RawBytes"]

        connection.close()
        engine.dispose()
        byteIO.write(rawBytes)
        byteIO.seek(0)
    except Exception as e:
        print(e)
        abort(400)
    finally:
        connection.close()
        engine.dispose()

    return byteIO, filename


def createByteIOForSmallestVioOrCrash(projId, footprint):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    byteIO = io.BytesIO()

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        data = {"footprint": footprint}
        statement = text(GET_SMALLEST_VIO_OR_CRASH_TC)
        result = connection.execute(statement, data)

        rawBytes = result.fetchone()[0]
        byteIO.write(rawBytes)
        byteIO.seek(0)
    except Exception as e:
        print(e)
        abort(400)
    finally:
        connection.close()
        engine.dispose()

    return byteIO


def addCommand(projId, guid, hostAndPort = None):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        if not hostAndPort:
            data = {"guid": guid}
            statement = text(MANAGED_INSTANCES_HOST_AND_PORT)
            result = connection.execute(statement, data)
            hostAndPort = result.fetchone()[0]

        command = models.CommandQueue(Command = "kill", Argument = hostAndPort, Done = 0)
        db.session.add(command)
        db.session.commit()
    except Exception as e:
        print(e)
    finally:
        connection.close()
        engine.dispose()


def addCommandToKillInstanceType(projId, myType):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        data = {"myType": myType}
        statement = text(MANAGED_INSTANCES_HOST_AND_PORT_AGENT_TYPE)

        if myType == 4:
            statement = text(GET_MI_HOST_AND_PORT)
        result = connection.execute(statement, data)

        for row in result:
            hostAndPort = row["ServiceDescriptorHostAndPort"]
            command = models.CommandQueue(Command = "kill", Argument = hostAndPort, Done = 0)
            db.session.add(command)
            db.session.commit()

    except Exception as e:
        print(e)
        pass
    finally:
        connection.close()
        engine.dispose()


def insertTestcases(projId, files):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        localId = connection.execute(GET_MAX_LOCALID).fetchone()[0]

        if localId is None:
            localId = 0
        else:
            localId += 1

        # insert testcases and add filenames as nice names
        for f in files:
            connection.execute(text(INSERT_TESTCASE_POPULATION), {"rawData": f.read(), "localId": localId})
            connection.execute(text(INSERT_NICE_NAME_TESTCASE), {"localId": localId, "guid":"initial", "newName": f.filename})
            localId += 1

        return "Added Testcase(s)", "success"
    except Exception as e:
        print(e)
        if "Duplicate entry" in str(e):
            return "SQLAlchemy Exception: Duplicate entry for key", "error"
    finally:
        connection.close()
        engine.dispose()

    return "Failed to add Testcase", "error"


def updateSettings(projId, settingId, settingValue):
    if len(settingValue) > 0:
        project = models.Fuzzjob.query.filter_by(ID=projId).first()
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        try:
            statement = text(UPDATE_SETTINGS)
            data = {"ID": settingId, "SettingValue": settingValue}
            connection.execute(statement, data)
            message = "Setting modified"
            status = "OK"
        except Exception as e:
            print(e)
            message = "Could not modify setting"
            status = "Error"
        finally:
            connection.close()
            engine.dispose()
    else:
        message = "Value cannot be empty!"
        status = "Error"

    return message, status


def deleteElement(projId, elementName, query, data):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        connection.execute(text(query), data)
        msg, category = elementName + " deleted", "success"
    except Exception as e:
        print(e)
        msg, category = "Error: Could not delete " + elementName, "error"
    finally:
        connection.close()
        engine.dispose()

    return msg, category


def deleteConfiguredInstance(sysName, fjName, typeFromReq):        
    try:
        system = models.Systems.query.filter_by(Name=sysName).first()
    except Exception as e:
        print(e)
        return "ERROR", "System {} was not found".format(sysName)
            
    fuzzjobId = None
    agentType = AGENT_TYPES.get(typeFromReq, None)           
    
    if agentType is not None:
        try:
            if agentType != 4:
                fuzzjob = models.Fuzzjob.query.filter_by(name=fjName).first()
                fuzzjobId = fuzzjob.ID
                
            instance = models.SystemFuzzjobInstances.query.filter_by(System=system.ID, Fuzzjob=fuzzjobId, AgentType=agentType).first()
            if instance is not None:
                db.session.delete(instance)
                db.session.commit() 
                return "OK", "Successfully removed instance!"
            
            return "ERROR", "Failed! Cannot find instance."
        except Exception as e:
            print(e)
            return "ERROR", "Failed to delete instance."
    
    return "ERROR", "Missing agent type"  


def insertFormInputForConfiguredInstances(request, system):
    try:      
        sys = models.Systems.query.filter_by(Name=system).first()        
        if sys is None:
            return "Error: Could not configure Instances. System does not exist!", "error"
                                        
        for key, value in request.form.items(): 
            agentType = AGENT_TYPES.get(key[-2:], None)                                                  
            
            if agentType is not None and agentType != 4:
                fuzzjobName = key[:-3]
                fj = models.Fuzzjob.query.filter_by(name=fuzzjobName).first()           
                if fj is not None:                                             
                    instance = models.SystemFuzzjobInstances.query.filter_by(System=sys.ID, Fuzzjob=fj.ID,
                                                                            AgentType=agentType).first()
                    
                    arch = request.form.get(key + '_arch', None) 
                    
                    try:
                        valueAsInt = int(value)
                    except ValueError:
                        valueAsInt = -1 
                        
                    # update system fuzzjob instance
                    if instance is not None:                                                   
                        if valueAsInt > 0:                                
                            instance.InstanceCount = valueAsInt
                        elif valueAsInt == 0:
                            db.session.delete(instance)                               
                        if arch is not None:
                            instance.Architecture = arch 
                            
                    # Add new system fuzzjob instance
                    else:
                        if arch is not None and valueAsInt > 0:
                            newInstance = models.SystemFuzzjobInstances(System=sys.ID, Fuzzjob=fj.ID,
                                                                        AgentType=agentType,
                                                                        InstanceCount=valueAsInt, Architecture=arch)
                            db.session.add(newInstance)
                    db.session.commit()                                                
        return "Success: Configured Instances!", "success"            
    except Exception as e:
        print(e)
        return "Error: Could not configure Instances!", "error"
    
    
def insertFormInputForConfiguredFuzzjobInstances(request, fuzzjob):
    try:      
        fj = models.Fuzzjob.query.filter_by(name=fuzzjob).first()         
        if fj is None:
            return "Error: Could not configure Instances. Fuzzjob does not exist!", "error"
                                        
        for key, value in request.form.items(): 
            agentType = AGENT_TYPES.get(key[-2:], None)                                                  
            
            if agentType is not None and agentType != 4:
                systemName = key[:-3]
                sys = models.Systems.query.filter_by(Name=systemName).first()            
                if sys is not None:                                             
                    instance = models.SystemFuzzjobInstances.query.filter_by(System=sys.ID, Fuzzjob=fj.ID,
                                                                            AgentType=agentType).first()
                    
                    arch = request.form.get(key + '_arch', None) 
                    
                    try:
                        valueAsInt = int(value)
                    except ValueError:
                        valueAsInt = -1 
                        
                    # update system fuzzjob instance
                    if instance is not None:                                                   
                        if valueAsInt > 0:                                
                            instance.InstanceCount = valueAsInt
                        elif valueAsInt == 0:
                            db.session.delete(instance)                               
                        if arch is not None:
                            instance.Architecture = arch 
                            
                    # Add new system fuzzjob instance
                    else:
                        if arch is not None and valueAsInt > 0:
                            newInstance = models.SystemFuzzjobInstances(System=sys.ID, Fuzzjob=fj.ID,
                                                                        AgentType=agentType,
                                                                        InstanceCount=valueAsInt, Architecture=arch)
                            db.session.add(newInstance)
                    db.session.commit()                                                
        return "Success: Configured Instances!", "success"            
    except Exception as e:
        print(e)
        return "Error: Could not configure Instances!", "error"


def insertFormInputForProject(form, request):
    myFuzzjobName = form.name.data.strip()
    validFuzzjobName = bool(re.match("^[A-Za-z0-9]*$", myFuzzjobName))
    
    targetFileName = ""

    if not validFuzzjobName or not form.targetCMDLine.data or form.option_module.data is None or not form.option_module_value.data:
        return ["Error: Could not create project! Check input data!", "error"]

    targetFileUpload = False

    if 'targetfile' in request.files:
        targetFile = request.files['targetfile']
        if targetFile:
            targetFileData = targetFile.read()
            targetFileName = targetFile.filename
            FTP_CONNECTOR.saveTargetFileOnFTPServer(targetFileData, targetFileName)
            targetFileUpload = True

    project = createNewDatabase(name=myFuzzjobName)
    db.session.add(project)
    db.session.commit()
    locations = request.form.getlist('location')

    for loc in locations:
        location = models.Locations.query.filter_by(Name=loc).first()
        location_fuzzjob = models.LocationFuzzjobs(Location=location.ID, Fuzzjob=project.ID)
        db.session.add(location_fuzzjob)
        db.session.commit()

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        data = {"SettingName": 'targetCMDLine', "SettingValue": form.targetCMDLine.data}
        statement = text(INSERT_SETTINGS)
        connection.execute(statement, data)

        data = {"SettingName": 'hangTimeout', "SettingValue": form.option_module_value.data}
        statement = text(INSERT_SETTINGS)
        connection.execute(statement, data)

        data = {"SettingName": 'runnerType', "SettingValue": form.subtype.data}
        statement = text(INSERT_SETTINGS)
        connection.execute(statement, data)

        data = {"SettingName": 'generatorTypes',
                "SettingValue": formatSubtypeInput(request.form.getlist("generatorTypes"),
                                                   JSON_CONFIG["GeneratorTypes"])}
        statement = text(INSERT_SETTINGS)
        connection.execute(statement, data)

        data = {"SettingName": 'evaluatorTypes',
                "SettingValue": formatSubtypeInput(request.form.getlist("evaluatorTypes"),
                                                   JSON_CONFIG["EvaluatorTypes"])}
        statement = text(INSERT_SETTINGS)
        connection.execute(statement, data)

        if targetFileUpload:
            deployment_package = models.DeploymentPackages(name = targetFileName)
            db.session.add(deployment_package)
            db.session.commit()
            fuzzjob_deployment_package = models.FuzzjobDeploymentPackages(Fuzzjob = project.ID,
                                                                          DeploymentPackage = deployment_package.ID)
            db.session.add(fuzzjob_deployment_package)
            db.session.commit()

        f = request.form

        for key in f.keys():
            if '_optionname' in key:
                if len(f.getlist(key)[0]) > 1:
                    optionNum = key.split("_")[0]
                    nextOption = request.form.get(str(optionNum) + '_optionname')
                    nextOptionValue = request.form.get(str(optionNum) + '_optionvalue')
                    if nextOption is not None:
                        data = {"SettingName": nextOption, "SettingValue": nextOptionValue}
                        statement = text(INSERT_SETTINGS)
                        connection.execute(statement, data)

        if 'targetModulesOnCreate' in request.files:
            for module in request.files.getlist("targetModulesOnCreate"):
                data = {"ModuleName": module.filename, "ModulePath": "*", "RawBytes": module.read()}
                statement = text(INSERT_MODULE)
                connection.execute(statement, data)

        localId = connection.execute(GET_MAX_LOCALID).fetchone()[0]

        if localId is None:
            localId = 0
        else:
            localId += 1

        if 'filename' in request.files:
            # insert testcases and add filenames as nice names
            for f in request.files.getlist('filename'):
                connection.execute(text(INSERT_TESTCASE_POPULATION), {"rawData": f.read(), "localId": localId})
                connection.execute(text(INSERT_NICE_NAME_TESTCASE), {"localId": localId,"guid": "initial", "newName": f.filename})
                localId += 1
        else:
            return ["Error: No files were found!", "error"]

        if 'basicBlockFile' in request.files:
            if form.subtype.data == "ALL_GDB":
                setNewBasicBlocks(request.files['basicBlockFile'], project.ID)

        return ["Success: Created new project", "success", project.ID]
    except Exception as e:
        print(e)
        return ["Error: " + str(e), "error"]
    finally:
        connection.close()
        engine.dispose()


def addTargetZipToFuzzjob(projId, targetFileName):
    deployment_package = models.DeploymentPackages(name=targetFileName)
    db.session.add(deployment_package)
    db.session.commit()
    fuzzjob_deployment_package = models.FuzzjobDeploymentPackages(Fuzzjob=projId,
                                                                  DeploymentPackage=deployment_package.ID)
    db.session.add(fuzzjob_deployment_package)
    db.session.commit()


def removeAgents(projId):
    instances = models.SystemFuzzjobInstances.query.filter_by(Fuzzjob=projId).all()
    for instance in instances:
        if instance is not None:
            db.session.delete(instance)
    db.session.commit()
    

def getGraphData(projId):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    graphdata = dict()
    nodes = []    
    edges = []
    nodeIds = []

    engine = create_engine('mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, project.DBHost, project.DBName))
    connection = engine.connect()
    try:
        result = connection.execute(GET_POPULATION_DETAILS)

        for row in result:
            testcase = dict()
            # Added a cyId for cytoscape, cause the css selector could not handle the ':' inside the ID
            testcase["cyId"] = "%s-%d" % (row["CreatorServiceDescriptorGUID"], row["CreatorLocalID"])
            testcase["realId"] = "%s:%d" % (row["CreatorServiceDescriptorGUID"], row["CreatorLocalID"])
            testcase["niceNameTC"] = row["NiceNameTC"] if row["NiceNameTC"] else ""
            testcase["niceNameMI"] = "{}:{}".format(row["NiceNameMI"], row["CreatorLocalID"]) if row[
                "NiceNameMI"] else ""
            testcase["parent"] = "%s-%d" % (row["ParentServiceDescriptorGUID"], row["ParentLocalID"])
            testcase["type"] = "%d" % row["TestCaseType"]
            nodes.append(testcase)
            nodeIds.append(testcase["cyId"])
            edge = {"parent": testcase["parent"], "child": testcase["cyId"], "label": 0}
            edges.append(edge)
        
        result = connection.execute(GET_CRASH_DETAILS)

        # cluster crashes by footprint
        for row in result:
            if row["CrashFootprint"]:
                footprintNode = dict()
                footprintNode["cyId"] = row["CrashFootprint"]
                footprintNode["realId"] = ""
                footprintNode["crashFootprint"] = row["CrashFootprint"]
                footprintNode["parent"] = "N/A"
                nodes.append(footprintNode)

                crashParentResult = connection.execute(text(GET_CRASH_PARENTS),
                                                       {"CrashFootprint": row["CrashFootprint"]})

                for crashParentRow in crashParentResult:
                    parentCyID = "%s-%d" % (
                        crashParentRow["ParentServiceDescriptorGUID"], crashParentRow["ParentLocalID"])
                    numEdges = crashParentRow["NumberEdges"]
                    edge = {"parent": parentCyID, "child": footprintNode["cyId"], "label": numEdges}
                    edges.append(edge)
      
        removeEdges = edges.copy()
        for edge in removeEdges:
            if edge["parent"] not in nodeIds:
                edges.remove(edge)                

    except Exception as e:
        print(e)
        pass
    finally:
        connection.close()
        engine.dispose()

    graphdata["nodes"] = nodes
    graphdata["edges"] = edges
    
    return graphdata


class DownloadArchiveLockFile:
    file_path = "/download.lock"
    tmp_path = "/downloadTemp"

    # if lock file in dir: return False
    def check_file(self):

        file_available = False
        for x in range(0, 2):
            if os.access(self.file_path, os.F_OK):
                file_available = True
            time.sleep(0.01)

        return not file_available

    def write_file(self, type, nice_name, pid=0, max_val=0, download=""):
        if self.check_file():
            file_w = open(self.file_path, "a")
            file_w.write("TYPE=" + type + "\n" +
                         "PID=" + str(pid) + "\n" +
                         "NICE_NAME=" + nice_name + "\n" +
                         "PROGRESS=0\n" +
                         "MAX_VAL=" + str(max_val) + "\n" +
                         "STATUS=\n" +
                         "MESSAGE=\n" +
                         "DOWNLOAD_PATH=" + download + "\n" +
                         "END=0\n")
            file_w.close()

    def read_file(self):
        if os.access(self.file_path + ".bak", os.R_OK):
            try:
                os.rename(self.file_path + ".bak", self.file_path)
            except Exception as e:
                print(str(e))
        if os.access(self.file_path, os.R_OK):
            file_r = open(self.file_path, "r")
            content = {}
            for line in file_r:
                line_list = line.split("=", 1)
                if '\n' in line:
                    value = line_list[1].replace('\n', '')
                else:
                    value = line_list[1]
                content[line_list[0]] = value
            file_r.close()
            return content
        else:
            return ""

    def read_file_entry(self, entry):
        if os.access(self.file_path + ".bak", os.R_OK):
            try:
                os.rename(self.file_path + ".bak", self.file_path)
            except Exception as e:
                print(str(e))
        if os.access(self.file_path, os.R_OK):
            file_r = open(self.file_path, "r")
            for num, line in enumerate(file_r):
                if entry in line:
                    file_r.close()
                    line_list = line.split("=", 1)
                    if '\n' in line:
                        value = line_list[1].replace('\n', '')
                    else:
                        value = line_list[1]
                    return value
            file_r.close()
            return ""
        else:
            return ""

    def change_file_entry(self, entry, value):
        if os.access(self.file_path, os.R_OK):
            file = open(self.file_path, 'r')
            lines = file.readlines()
            file.close()
            for num, line in enumerate(lines):
                if entry in line:
                    lines[num] = entry + "=" + value + "\n"
            out = open(self.file_path + ".bak", 'w+')
            out.writelines(lines)
            out.close()
            try:
                os.rename(self.file_path + ".bak", self.file_path)
            except Exception as e:
                print(str(e))

    def delete_file(self):
        if os.access(self.file_path, os.R_OK):
            if os.path.exists(self.tmp_path):
                shutil.rmtree(self.tmp_path, ignore_errors=True)
            os.remove(self.file_path)
        else:
            if os.access(self.file_path, os.R_OK) and self.read_file_entry("END") == "1":
                if os.path.exists(self.tmp_path):
                    shutil.rmtree(self.tmp_path, ignore_errors=True)
                os.remove(self.file_path)
