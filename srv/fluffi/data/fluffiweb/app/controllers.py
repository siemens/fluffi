# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Junes Najah, Pascal Eckmann, Michael Kraus, Abian Blome, Thomas Riedmaier

import io
import csv
import threading
import subprocess
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
    projects = models.Fuzzjob.query.all()

    for project in projects:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        try:
            result = connection.execute(COMPLETED_TESTCASES_COUNT)
            project.testcases = result.fetchone()[0]

            result = connection.execute(NUMBER_OF_NO_LONGER_LISTED)
            numberOfNoLongerListed = result.fetchone()[0]
            project.testcases = project.testcases + numberOfNoLongerListed

            result = connection.execute(getITCountOfTypeQuery(0))
            project.numPopulation = result.fetchone()[0]

            result = connection.execute(getITCountOfTypeQuery(5))
            project.numMinimizedPop = result.fetchone()[0]

            result = connection.execute(getITCountOfTypeQuery(1)) 
            project.numHang = result.fetchone()[0]

            result = connection.execute(NUM_UNIQUE_ACCESS_VIOLATION_TYPE_2)
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
            if len(result.fetchall()) == 0:
                project.checkRating = True
            else:
                project.checkRating = False

            project.numLM = int(models.Localmanagers.query.filter_by(Fuzzjob = project.id).count())
        except Exception as e:
            print(e)
            project.status = "Unreachable"
            project.testcases = "-"
            project.numPopulation = "-"
            project.numMinimizedPop = "-"
            project.numHang = "-"
            project.numAccessViolation = "-"
            project.numException = "-"
            project.numNoResponse = "-"
            project.numLM = 0
        finally:
            connection.close()
            engine.dispose()

    return projects


def getLocations():
    locations = db.session.query(models.Locations)

    return locations


def getDownloadPath():
    return app.root_path[:-3]


class CreateTestcaseArchive(threading.Thread):
    def __init__(self, projId, nice_name, name_list, count_statement_list, statement_list):
        super().__init__()
        self.progress = 0
        self.projId = projId
        self.nice_name = nice_name
        self.name_list = name_list
        self.count_statement_list = count_statement_list
        self.statement_list = statement_list
        self.max_val_list = []
        # status: 0 = default/running, 1 = success, 2 = error
        self.status = (0, "")
        self.stop = False

    def setMaxVal(self):
        print(self.name_list)
        project = models.Fuzzjob.query.filter_by(id=self.projId).first()
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        try:
            for count_statement in self.count_statement_list:
                result = connection.execute(count_statement)
                self.max_val_list.append(int((result.fetchone()[0])/20) + 1)
        except Exception as e:
            self.status = (2, str(e))
        finally:
            connection.close()
            engine.dispose()

    def run(self):
        project = models.Fuzzjob.query.filter_by(id=self.projId).first()
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        try:
            if not self.stop:
                for num, statement in enumerate(self.statement_list):
                    if not self.stop:
                        path = app.root_path + "/tmp/" + self.name_list[num]
                        if len(self.name_list) == 1:
                            zipFilePath = getDownloadPath() + self.name_list[num] + ".zip"
                        else:
                            zipFilePath = getDownloadPath() + "testcase_set.zip"
                        if os.path.isfile(zipFilePath):
                            os.remove(zipFilePath)
                        if os.path.exists(path):
                            shutil.rmtree(path)
                        os.makedirs(path)

                        for block in range(0, self.max_val_list[num]):
                            if not self.stop:
                                self.progress = self.progress + 1
                                block_statement = statement[:-1] + " LIMIT " + str(block * 20) + ", 20;"
                                result = connection.execute(block_statement)

                                for row in result:
                                    if 'NiceName' in row.keys():
                                        fileName = row["NiceName"] if row["NiceName"] else "{}_id{}".format(
                                            row["CreatorServiceDescriptorGUID"],
                                            row["ID"])
                                    else:
                                        fileName = "{}_id{}".format(row["CreatorServiceDescriptorGUID"], row["ID"])
                                    rawData = row["RawBytes"]
                                    f = open(path + "/" + fileName, "wb+")
                                    f.write(rawData)
                                    f.close()

                        if len(self.statement_list) == num + 1 or not self.stop:
                            if len(self.statement_list) > 1:
                                path = app.root_path + "/tmp"
                                filename = shutil.make_archive("testcase_set", "zip", path)
                            else:
                                filename = shutil.make_archive(self.name_list[num], "zip", path)

                            self.status = (1, "File " + filename + " created.")
                            if os.path.exists(path):
                                shutil.rmtree(path, ignore_errors=True)
        except Exception as e:
            print(str(e))
            self.status = (2, str(e))
        finally:
            connection.close()
            engine.dispose()

    def end(self):
        self.stop = True
        path = app.root_path + "/tmp"
        if os.path.exists(path):
            shutil.rmtree(path, ignore_errors=True)


def getLocationFormWithChoices(projId, locationForm):
    choices = []
    locationFuzzJobs = models.LocationFuzzjobs.query.filter_by(Fuzzjob = projId).all()
    locationFuzzJobsIds = [f.Location for f in locationFuzzJobs]

    for location in db.session.query(models.Locations):
        if location.id not in locationFuzzJobsIds:
            choices.append(location.Name)

    locationForm.location.choices = choices

    return locationForm


def getProject(projId):
    project = models.Fuzzjob.query.filter_by(id = projId).first()

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        result = connection.execute(COMPLETED_TESTCASES_COUNT)
        project.testcases = result.fetchone()[0]

        result = connection.execute(NUMBER_OF_NO_LONGER_LISTED)
        numberOfNoLongerListed = result.fetchone()[0]
        project.testcases = project.testcases + numberOfNoLongerListed

        result = connection.execute(getITCountOfTypeQuery(0))
        project.numPopulation = result.fetchone()[0]

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
        if len(result.fetchall()) == 0:
            project.checkRating = True
        else:
            project.checkRating = False

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
            setting.id = row["Id"]
            project.settings.append(setting)

        project.possibleSettingNames = [sn for sn in config["DefaultSettingNames"] if
                                        sn not in [s.name for s in project.settings]]
        result = connection.execute(GET_TARGET_MODULES)
        project.modules = []

        for row in result:
            module = type('', (), {})()
            module.name = row["ModuleName"]
            module.path = row["ModulePath"]
            module.id = row["ID"]
            project.modules.append(module)

        project.status = "Reachable"
    except Exception as e:
        print(e)
        project.status = "Unreachable"
        project.testcases = "-"
        project.numPopulation = "-"
        project.numMinimizedPop = "-"
        project.numHang = "-"
        project.numAccessViolation = "-"
        project.numException = "-"
        project.numNoResponse = "-"
        project.numBlocks = "-"
        project.numTE = "-"
        project.numTG = "-"
        project.numTR = "-"
        project.settings = []
        project.modules = []
    finally:
        connection.close()
        engine.dispose()

    project.locations = db.session.query(models.LocationFuzzjobs, models.Locations.Name, models.Locations.id).filter_by(
        Fuzzjob = projId).outerjoin(models.Locations)
    project.numLM = models.Localmanagers.query.filter_by(Fuzzjob = projId).count()

    return project


def getProjects():
    projects = models.Fuzzjob.query.all()

    for project in projects:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        try:
            result_dirty = connection.execute(GET_PROJECTS)
            result = result_dirty.fetchall()[0]
            project.testcases = result[0] + result[1]
            project.numPopulation = result[2]
            project.numHang = result[3]
            project.numAccessViolation = result[4]
            project.numException = result[5]
            project.numNoResponse = result[6]
            if result[7] is None:
                project.checkRating = True
            else:
                project.checkRating = False

            project.status = "Reachable"
        except Exception as e:
            print(e)
            project.status = "Unreachable"
            project.testcases = "-"
            project.numPopulation = "-"
            project.numMinimizedPop = "-"
            project.numHang = "-"
            project.numAccessViolation = "-"
            project.numException = "-"
            project.numNoResponse = "-"
            project.checkRating = False
        finally:
            connection.close()
            engine.dispose()

    return projects


def getGeneralInformationData(projId, stmt):
    project = models.Fuzzjob.query.filter_by(id = projId).first()
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
            testcase.id = "{}:{}".format(row["CreatorServiceDescriptorGUID"], row["CreatorLocalID"])
            testcase.rating = row["Rating"]
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


def insertOrUpdateNiceName(projId, myId, newName, command, elemType):
    project = models.Fuzzjob.query.filter_by(id = projId).first()

    engine = create_engine(
        'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
    connection = engine.connect()
    try:
        if elemType == "testcase":
            data = {"testcaseID": myId, "newName": newName}
            statement = text(INSERT_NICE_NAME_TESTCASE) if command == "insert" else text(UPDATE_NICE_NAME_TESTCASE)
            connection.execute(statement, data)
        elif elemType == "managedInstance":
            data = {"sdguid": myId, "newName": newName}
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
    location.id = locId
    location.name = db.session.query(models.Locations.Name).filter_by(id = locId).first()[0]

    for l in models.LocationFuzzjobs.query.filter_by(Location = locId).all():
        currentFuzzjob = models.Fuzzjob.query.filter_by(id = l.Fuzzjob).first()
        location.projects.append(currentFuzzjob)
    location.workers = models.Workers.query.filter_by(Location = locId, Fuzzjob = None).all()

    return location


def getManagedInstancesAndSummary(projId):
    # new data in Status will be added to the instances in managedInstances automatically - you just need to add them in
    # viewManagedInstances.html
    project = models.Fuzzjob.query.filter_by(id = projId).first()
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

    # sort list of instances by AgentType 
    managedInstances["instances"] = sorted(managedInstances["instances"], key = lambda k: k["AgentType"])
    average = sumOfAverageRTT / numOfRTT if numOfRTT != 0 else 0
    summarySection['AverageRTT'] = round(average, 1)

    return managedInstances, summarySection, localManagers


def getViolationsAndCrashes(projId):
    project = models.Fuzzjob.query.filter_by(id = projId).first()
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


def insertSettings(projId, request, settingForm):
    project = models.Fuzzjob.query.filter_by(id = projId).first()

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

    project = models.Fuzzjob.query.filter_by(id=projId).first()

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
                    data = {"ModuleID": row[0], "Offset": row[1]}
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
            connection.execute(RESET_INITIAL_RATING)

        return "Reset Fuzzjob was successful", "success"
    except Exception as e:
        print(e)
        return "Error: Failed to reset fuzzjob", "error"
    finally:
        connection.close()
        engine.dispose()


def insertModules(projId, f):
    project = models.Fuzzjob.query.filter_by(id = projId).first()

    if project is not None:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        try:
            isEmpty = False
            for key in f.keys():
                if '_targetname' in key:
                    if len(f.getlist(key)[0]) > 1:
                        targetNum = key.split("_")[0]
                        moduleName = f.get(str(targetNum) + '_targetname')
                        modulePath = f.get(str(targetNum) + '_targetpath')
                        if len(moduleName) > 0 and len(modulePath) > 0:
                            data = {"ModuleName": moduleName, "ModulePath": modulePath}
                            statement = text(INSERT_MODULE)
                            connection.execute(statement, data)
                        else:
                            isEmpty = True
                    else:
                        isEmpty = True
            if len(f.getlist('targetModules')) > 0 and f.getlist('targetModules')[0]:
                isEmpty = False
                for moduleName in f.getlist('targetModules'):
                    data = {"ModuleName": moduleName, "ModulePath": "*"}
                    statement = text(INSERT_MODULE)
                    connection.execute(statement, data)
            if isEmpty:
                return "Error: Input cannot be empty", "error"
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
    project = models.Fuzzjob.query.filter_by(id = projId).first()
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
    project = models.Fuzzjob.query.filter_by(id = projId).first()
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
    project = models.Fuzzjob.query.filter_by(id = projId).first()

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
    project = models.Fuzzjob.query.filter_by(id = projId).first()

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
    project = models.Fuzzjob.query.filter_by(id = projId).first()

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
            testcaseID = connection.execute(text(GET_TESTCASE_ID), {"creatorlocalID": localId}).fetchone()[0]
            connection.execute(text(INSERT_NICE_NAME_TESTCASE), {"testcaseID": testcaseID, "newName": f.filename})
            localId += 1

        return "Added Testcase(s)", "success"
    except Exception as e:
        print(e)
        abort(400)
    finally:
        connection.close()
        engine.dispose()

    return "Failed to add Testcase", "error"


def updateSettings(projId, settingId, settingValue):
    if len(settingValue) > 0:
        project = models.Fuzzjob.query.filter_by(id=projId).first()
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        try:
            statement = text(UPDATE_SETTINGS)
            data = {"Id": settingId, "SettingValue": settingValue}
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
    project = models.Fuzzjob.query.filter_by(id = projId).first()

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


def insertFormInputForConfiguredInstances(request, system):
    f = request.form

    try:
        dic = {}
        for key, value in f.items():
            if 'arch' in key[-4:]:
                dic[key] = value
        for key, value in f.items():
            if value is not "" and 'arch' not in key[-4:]:
                agenttype = 0

                if "tr" in key[-3:]:
                    agenttype = 1
                if "te" in key[-3:]:
                    agenttype = 2
                if "lm" in key[-3:]:
                    agenttype = 4

                sys = models.Systems.query.filter_by(Name = system).first()
                fuzzjobId = None

                if agenttype != 4:
                    fj = models.Fuzzjob.query.filter_by(name = key[:-3]).first()
                    fuzzjobId = fj.id

                arch = dic.get(key + '_arch')
                instance = models.SystemFuzzjobInstances.query.filter_by(System = sys.id, Fuzzjob = fuzzjobId,
                                                                         AgentType = int(agenttype)).first()
                if instance is not None:
                    db.session.delete(instance)
                    db.session.commit()

                if value != 0:
                    newinstances = models.SystemFuzzjobInstances(System = sys.id, Fuzzjob = fuzzjobId,
                                                                 AgentType = agenttype,
                                                                 InstanceCount = value, Architecture = arch)
                    db.session.add(newinstances)
                    db.session.commit()

        return "Success: Configured Instances!", "success"
    except Exception as e:
        print(e)
        return "Error: Could not configure Instances!", "error"


def insertFormInputForConfiguredFuzzjobInstances(request, fuzzjob):
    f = request.form

    try:
        dic = {}
        for key, value in f.items():
            if 'arch' in key[-4:]:
                dic[key] = value
        for key, value in f.items():
            if value is not "" and 'arch' not in key[-4:]:
                agenttype = 0
                if "tr" in key[-3:]:
                    agenttype = 1
                if "te" in key[-3:]:
                    agenttype = 2
                if "lm" in key[-3:]:
                    agenttype = 4

                fj = models.Fuzzjob.query.filter_by(name = fuzzjob).first()
                systemId = None

                if agenttype != 4:
                    sys = models.Systems.query.filter_by(Name = key[:-3]).first()
                    systemId = sys.id

                arch = dic.get(key + '_arch')
                instance = models.SystemFuzzjobInstances.query.filter_by(System = systemId, Fuzzjob = fj.id,
                                                                         AgentType = int(agenttype)).first()
                if instance is not None:
                    db.session.delete(instance)
                    db.session.commit()
                if value != 0:
                    newinstances = models.SystemFuzzjobInstances(System = systemId, Fuzzjob = fj.id,
                                                                 AgentType = agenttype,
                                                                 InstanceCount = value, Architecture = arch)
                    db.session.add(newinstances)
                    db.session.commit()

        return "Success: Configured Instances!", "success"
    except Exception as e:
        print(e)
        return "Error: Could not configure Instances!", "error"

 
def insertFormInputForProject(form, request):
    myProjName = form.name.data.replace(" ", "_")
    targetFileName = ""
    
    if not myProjName or not form.targetCMDLine.data or form.option_module.data is None or not form.option_module_value.data:
        return ["Error: Could not create project! Check input data!", "error"]

    targetFileUpload = False

    if 'targetfile' in request.files:
        targetFile = request.files['targetfile']
        targetFileData = targetFile.read()
        targetFileName = targetFile.filename
        FTP_CONNECTOR.saveTargetFileOnFTPServer(targetFileData, targetFileName)
        targetFileUpload = True

    project = createNewDatabase(name=myProjName)
    db.session.add(project)
    db.session.commit()
    locations = request.form.getlist('location')

    for loc in locations:
        location = models.Locations.query.filter_by(Name=loc).first()
        location_fuzzjob = models.LocationFuzzjobs(Location=location.id, Fuzzjob=project.id)
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
            fuzzjob_deployment_package = models.FuzzjobDeploymentPackages(Fuzzjob = project.id,
                                                                          DeploymentPackage = deployment_package.id)
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

        for key in f.keys():
            if '_targetname' in key:
                if len(f.getlist(key)[0]) > 1:
                    targetNum = key.split("_")[0]
                    nextTarget = request.form.get(str(targetNum) + '_targetname')
                    nextTargetPath = request.form.get(str(targetNum) + '_targetpath')
                    if nextTarget is not None:
                        data = {"ModuleName": nextTarget, "ModulePath": nextTargetPath}
                        statement = text(INSERT_MODULE)
                        connection.execute(statement, data)

        if 'targetModulesOnCreate' in request.files:
            for module in request.files.getlist("targetModulesOnCreate"):
                data = {"ModuleName": module.filename, "ModulePath": "*"}
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
                testcaseID = connection.execute(text(GET_TESTCASE_ID), {"creatorlocalID": localId}).fetchone()[0]
                connection.execute(text(INSERT_NICE_NAME_TESTCASE), {"testcaseID": testcaseID, "newName": f.filename})
                localId += 1
        else:
            return ["Error: No files were found!", "error"]

        if 'basicBlockFile' in request.files:
            if form.subtype.data == "ALL_GDB":
                setNewBasicBlocks(request.files['basicBlockFile'], project.id)

        return ["Success: Created new project", "success", project.id]
    except Exception as e:
        print(e)
        return ["Error: " + str(e), "error"]
    finally:
        connection.close()
        engine.dispose()


def getGraphData(projId):
    project = models.Fuzzjob.query.filter_by(id = projId).first()
    graphdata = dict()
    nodes = []
    edges = []

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
            edge = {"parent": testcase["parent"], "child": testcase["cyId"], "label": 0}
            edges.append(edge)

        result = connection.execute(GET_CRASH_DETAILS)

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

    except Exception as e:
        print(e)
        pass
    finally:
        connection.close()
        engine.dispose()

    graphdata["nodes"] = nodes
    graphdata["edges"] = edges

    return graphdata


class ArchiveProject(threading.Thread):
    def __init__(self, projId, nice_name):
        super().__init__()
        self.projId = projId
        self.nice_name = nice_name
        # status: 0 = default/running, 1 = success, 2 = error, 3 = finish
        self.status = (0, "Step 0/4: Start archiving fuzzjob.")
        self.stop = False

    def run(self):
        fuzzjob = models.Fuzzjob.query.filter_by(id=self.projId).first()

        if fuzzjob or not self.stop:
            try:
                scriptFile = os.path.join(app.root_path, "archiveDB.sh")
                returncode = subprocess.call(
                    [scriptFile, config.DBUSER, config.DBPASS, config.DBPREFIX, fuzzjob.name.lower(), config.DBHOST])
                if returncode != 0:
                    self.status = (2, str("Error Step 1/4: Database couldn't be archived."))
                else:
                    self.status = (1, str("Step 1/4: Database archived."))
            except Exception as e:
                print(e)
                self.status = (2, str(e))

            if self.status[0] is not 2 or not self.stop:
                fileName = config.DBPREFIX + fuzzjob.name.lower() + ".sql.gz"
                success = FTP_CONNECTOR.saveArchivedProjectOnFTPServer(fileName)
                if success:
                    self.status = (1, "Step 2/4: Archived project saved on ftp server.")
                else:
                    self.status = (2, "Step 2/4: Archived project couldn't be saved on ftp server.")

                os.remove(fileName)

                if self.status[0] == 1 or not self.stop:
                    try:
                        if models.Fuzzjob.query.filter_by(id=self.projId).first() is not None:
                            fuzzjob = models.Fuzzjob.query.filter_by(id=self.projId).first()
                            db.session.delete(fuzzjob)
                            db.session.commit()
                            self.status = (1, 'Step 3/4: Fuzzjob sucessfully deleted.')
                        else:
                            self.status = (2, 'Step 3/4: Fuzzjob not found.')
                    except Exception as e:
                        self.status = (2, str(e))

                    if self.status[0] == 1 or not self.stop:
                        engine = create_engine(
                            'mysql://%s:%s@%s/%s' % (
                            config.DBUSER, config.DBPASS, fluffiResolve(config.DBHOST), config.DEFAULT_DBNAME))
                        connection = engine.connect()
                        try:
                            connection.execute("DROP DATABASE {};".format(config.DBPREFIX + fuzzjob.name.lower()))
                            self.status = (3, 'Step 4/4: Database deleted.')
                        except Exception as e:
                            self.status = (2, str(e))
                        finally:
                            connection.close()
                            engine.dispose()

    def end(self):
        self.stop = True
