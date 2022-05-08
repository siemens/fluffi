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
# Author(s): Junes Najah, Pascal Eckmann, Abian Blome, Thomas Riedmaier, Michael Kraus

import io
import csv
import subprocess
import time
import re
import binascii
import difflib
import copy
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
            connection.close()
            engine.dispose()
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
        timeOfLatestPopulation = dateTimeOfLatestPopulation.strftime("%H:%M - %d.%m.%Y") if dateTimeOfLatestPopulation is not None else ""
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
            module.coveredBlocks = row["CoveredBlocks"] if row["CoveredBlocks"] is not None else 0
            module.ID = row["ID"]
            project.modules.append(module)

        project.locations = db.session.query(models.LocationFuzzjobs, models.Locations.Name, models.Locations.ID).filter_by(
            Fuzzjob = projId).outerjoin(models.Locations)
        project.numLM = models.Localmanagers.query.filter_by(Fuzzjob = projId).count()
        
        project.status = "Reachable"
        connection.close()
        engine.dispose()  
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
            connection.close()
            engine.dispose()
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
            info = dateTimeOfLatestPopulation.strftime("%H:%M - %d.%m.%Y") if dateTimeOfLatestPopulation is not None else ""
        # other infos can be added here
        
        msg = "Success"    
        status = "OK" 
        connection.close()
        engine.dispose() 
    except Exception as e:
        print(e)
        info = ""
        msg = "Failed to get updated info!"
        status = "ERROR"             
    
    return msg, info, status


def getGeneralInformationData(projId, stmt):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    data = type('', (), {})()
    data.testcases = []
    data.project = project
    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()

        result = connection.execute(stmt)

        for row in result:
            testcase = type('', (), {})()
            testcase.testcaseID = row["ID"]
            
            if "CrashFootprint" in row and row["CrashFootprint"] is not None:
                testcase.footprint = row["CrashFootprint"]
            
            if "CoveredBlocks" in row:
                testcase.coveredBlocks = row["CoveredBlocks"] if row["CoveredBlocks"] is not None else 0
                     
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

        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)

    return data


def getRowCount(projId, stmt, params=None):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    data = 0
    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        result = connection.execute(text(stmt), params) if params else connection.execute(stmt)
        data = result.fetchone()[0]
        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)

    return data
    
    
def writeHexDiffFile(hexOneFile, hexTwoFile, diffFile):
    with open(hexOneFile, "rb") as f1, open(hexTwoFile, "rb") as f2:
        matcher = difflib.SequenceMatcher(None, f1.read(), f2.read())
        with open(diffFile, 'a') as csvfile:
            w = csv.writer(csvfile, delimiter=' ')
            for tag, i1, i2, j1, j2 in matcher.get_opcodes():
                w.writerow([tag, i1, i2, j1, j2])
        return matcher.ratio()


def getHexDiff(diffFile):
    diff = []
    file = open(diffFile, "rU")
    reader = csv.reader(file, delimiter=' ')
    for row in reader:
        diff.append(row)

    return diff


def loadHexInFile(projId, testcaseId, filePath):
    offset = 1
    hexLen = 0
    pageCount = 0
    testcaseParentId = ""
    testcaseParentGuid = ""

    with open(filePath, 'wb') as testcaseHexfile:
        while True:
            resultTestcaseHex = getResultOfStatement(projId, GET_TESTCASE_DUMP,
                                                     {"testcaseID": testcaseId, "offset": offset})
            if resultTestcaseHex is not None:
                rows = resultTestcaseHex.fetchall()
                for row in rows:
                    for num, x in enumerate(row):
                        if num == 0:
                            testcaseHexfile.write(x)
                        elif num == 1:
                            hexLen = x
                            offset += 960
                            pageCount = max(int(x / 960) + (x % 960 > 0), pageCount)
                        elif num == 2:
                            testcaseParentId = x
                        elif num == 3:
                            testcaseParentGuid = x
            if offset > hexLen:
                break
    return testcaseParentId, testcaseParentGuid, pageCount


def getScaledDiff(diff):
    insSpacePT = []
    insSpaceT = []
    newTempDiff = copy.deepcopy(diff)
    newDiff = []

    for num, row in enumerate(diff):
        if (max(int(row[2]), int(newTempDiff[num][2])) - max(int(row[1]), int(newTempDiff[num][1])) >
            max(int(row[4]), int(newTempDiff[num][4])) - max(int(row[3]), int(newTempDiff[num][3]))):
            newTempDiff[num][4] = str(int(newTempDiff[num][2]))
            for subNum in range(num, len(diff)):
                space = int(newTempDiff[subNum][4]) - int(newTempDiff[subNum][3])
                newTempDiff[subNum][3] = newTempDiff[subNum-1][4] if subNum > 0 else 0
                newTempDiff[subNum][4] = str(int(newTempDiff[subNum][3]) + space)

        elif (max(int(row[2]), int(newTempDiff[num][2])) - max(int(row[1]), int(newTempDiff[num][1])) < max(int(row[4]),
              int(newTempDiff[num][4])) - max(int(row[3]), int(newTempDiff[num][3]))):
            newTempDiff[num][2] = str(int(newTempDiff[num][4]))
            for subNum in range(num, len(diff)):
                space = int(newTempDiff[subNum][2]) - int(newTempDiff[subNum][1])
                newTempDiff[subNum][1] = newTempDiff[subNum-1][2] if subNum > 0 else 0
                newTempDiff[subNum][2] = str(int(newTempDiff[subNum][1]) + space)

        if ((int(row[2]) - int(row[1])) - (int(row[4]) - int(row[3]))) > 0:
            insSpaceT.append([int(newTempDiff[num][3]) + (int(row[4]) - int(row[3])),
                              (int(row[2]) - int(row[1])) - (int(row[4]) - int(row[3]))])
        elif ((int(row[4]) - int(row[3])) - (int(row[2]) - int(row[1]))) > 0:
            insSpacePT.append([int(newTempDiff[num][1]) + (int(row[2]) - int(row[1])),
                               (int(row[4]) - int(row[3])) - (int(row[2]) - int(row[1]))])

        newDiff.append(newTempDiff[num])

    return newDiff, insSpaceT, insSpacePT


def getPageCountList(diff, newDiff, insSpaceT):
    pageCount = []

    pageCount.append(0)
    offsetPage = 0
    allCharCount = int(newDiff[-1][4])
    remainCharCount = int(diff[-1][4])
    for _ in range(0, int(int(newDiff[-1][4]) / 960) + (int(newDiff[-1][4]) % 960 > 0)):
        chunkCount = 960
        for insertion in insSpaceT:
            if offsetPage < insertion[0] < (offsetPage + chunkCount):
                if (insertion[0] + insertion[1]) <= (offsetPage + chunkCount):
                    chunkCount -= insertion[1]
                else:
                    chunkCount -= (offsetPage + 960) - insertion[0]
            offsetPage = chunkCount + offsetPage
            remainCharCount -= chunkCount
        if allCharCount >= chunkCount:
            if remainCharCount <= 0:
                chunkCount += (960 - chunkCount)
            allCharCount -= chunkCount
            pageCount.append(chunkCount + pageCount[-1])

    return pageCount


def getTestcaseHexFromFileDiff(diff, newDiff, insSpaceT, insSpacePT, pageCount, testcaseHexdumpFile, testcaseParentHexdumpFile, offset):
    chunkCount = 960
    for insertion in insSpaceT:
        if offset < insertion[0] < (offset + chunkCount):
            if (insertion[0] + insertion[1]) <= (offset + chunkCount):
                chunkCount -= insertion[1]
            else:
                chunkCount -= (offset + 960) - insertion[0]

    testcaseHexdump = getTestcaseHexFromFile(testcaseHexdumpFile, offset, chunkCount)

    pageNext = 0
    for num, page in enumerate(pageCount):
        if page == offset:
            if len(pageCount) > num + 1:
                pageNext = pageCount[num + 1]
            else:
                pageNext = int(newDiff[-1][4])

    pointer = offset
    for num, row in enumerate(newDiff):

        x = set(range(int(row[1]), int(row[2])))
        y = set(range(int(row[3]), int(row[4])))
        z = set(range(offset, pageNext))

        if len(list(set.intersection(x, z))) > 0 or len(list(set.intersection(y, z))) > 0:
            if row[0] == "delete":
                if (int(diff[num][4]) - int(diff[num][3])) < (int(diff[num][2]) - int(diff[num][1])):
                    for x in range(pointer, min((int(newDiff[num][2]) - int(newDiff[num][1])) + pointer, pageNext)):
                        testcaseHexdump.insert(x - offset, "  ")
                        pointer += 1
                else:
                    if int(row[3]) < offset:
                        pointer += (int(row[4]) - offset)
                    else:
                        pointer += (int(row[4]) - int(row[3]))
            elif row[0] == "replace":
                if (int(diff[num][4]) - int(diff[num][3])) <= (int(diff[num][2]) - int(diff[num][1])):
                    if (int(diff[num][2]) - int(diff[num][1])) + int(newDiff[num][1]) > offset:
                        pointer += (int(diff[num][4]) - int(diff[num][3]))
                    for x in range(pointer, min(int(newDiff[num][2]), pageNext)):
                        if pageNext > int(newDiff[num][1]):
                            testcaseHexdump.insert(x - offset, "  ")
                        else:
                            testcaseHexdump.insert(int(newDiff[num][1]), "  ")
                        pointer += 1
                else:
                    if int(row[3]) < offset:
                        pointer += int(row[4]) - offset
                    else:
                        pointer += int(row[4]) - int(row[3])
            else:
                if int(row[3]) < offset:
                    pointer += int(row[4]) - offset
                else:
                    pointer += int(row[4]) - int(row[3])
        if pointer >= pageNext:
            break

    pOffset = offset
    for insertion in insSpacePT:
        if offset > (insertion[0] + insertion[1]):
            pOffset -= insertion[1]
        elif insertion[0] < offset < (offset + 960) > (insertion[0] + insertion[1]):
            pOffset -= (offset - insertion[0])
    chunkCount = 960
    for insertion in insSpacePT:
        if pOffset < insertion[0] < (pOffset + chunkCount):
            if (insertion[0] + insertion[1]) <= (pOffset + chunkCount):
                chunkCount -= insertion[1]
            else:
                chunkCount -= (pOffset + 960) - insertion[0]

    testcaseParentHexdump = getTestcaseHexFromFile(testcaseParentHexdumpFile, pOffset, chunkCount)

    pointer = offset
    for num, row in enumerate(newDiff):

        x = set(range(int(row[1]), int(row[2])))
        y = set(range(int(row[3]), int(row[4])))
        z = set(range(offset, pageNext))

        if len(list(set.intersection(x, z))) > 0 or len(list(set.intersection(y, z))) > 0:
            if row[0] == "insert":
                if (int(diff[num][2]) - int(diff[num][1])) < (int(diff[num][4]) - int(diff[num][3])):
                    if int(newDiff[num][3]) < pointer:
                        insEnd = int(diff[num][4])
                    else:
                        insEnd = (int(newDiff[num][4]) - int(newDiff[num][3])) + pointer
                    for x in range(pointer, min(insEnd, pageNext)):
                        testcaseParentHexdump.insert(x - offset, "  ")
                        pointer += 1
                else:
                    if int(row[1]) < offset:
                        pointer += (int(row[2]) - offset)
                    else:
                        pointer += (int(row[2]) - int(row[1]))
            elif row[0] == "replace":
                if (int(diff[num][2]) - int(diff[num][1])) <= (int(diff[num][4]) - int(diff[num][3])):
                    if (int(diff[num][2]) - int(diff[num][1])) + int(newDiff[num][3]) > offset:
                        pointer += (int(diff[num][2]) - int(diff[num][1]))
                    for x in range(pointer, min(int(newDiff[num][4]), pageNext)):
                        if pageNext > int(newDiff[num][3]):
                            testcaseParentHexdump.insert(x - offset, "  ")
                        else:
                            testcaseParentHexdump.insert(int(newDiff[num][3]), "  ")
                        pointer += 1
                else:
                    if int(row[1]) < offset:
                        pointer += int(row[2]) - offset
                    else:
                        pointer += int(row[2]) - int(row[1])
            else:
                if int(row[1]) < offset:
                    pointer += int(row[2]) - offset
                else:
                    pointer += int(row[2]) - int(row[1])
        if pointer >= pageNext:
            break

    return pOffset, testcaseHexdump, testcaseParentHexdump


def getTestcaseHexFromFile(testcaseHexdumpFile, offset, chunksize):
    with open(testcaseHexdumpFile, "rb") as f:
        f.seek(offset, 0)
        hexChunk = binascii.hexlify(f.read(chunksize)).decode('ascii')
        hexList = [hexChunk[i:i + 2] for i in range(0, len(hexChunk), 2)]
    return hexList


def getTestcaseParentInfo(projId, testcaseId):
    testcaseParentId = ""
    testcaseParentGuid = ""
    resultTestcaseParent = getResultOfStatement(projId, GET_TESTCASE_PARENT,
                                             {"testcaseID": testcaseId})
    if resultTestcaseParent is not None:
        rows = resultTestcaseParent.fetchall()
        for row in rows:
            for num, x in enumerate(row):
                if num == 0:
                    testcaseParentId = x
                elif num == 1:
                    testcaseParentGuid = x
    return testcaseParentId, testcaseParentGuid


def deleteOldHexdumpFiles(fileNameSub):
    for element in os.listdir("/"):
        if fileNameSub in element and element.count(".") >= 2:
            if (int(round(time.time())) - int(round(os.stat("/" + element).st_mtime))) / 60 > 30:
                os.remove("/" + element)


def getTextByHex(hexTable, split):
    decodedData = []
    count = 0
    targetLen = (int(len(hexTable) / split) + (len(hexTable) % split > 0)) * split
    for _ in range(len(hexTable), targetLen):
        hexTable.append("  ")
        count += 1

    for b in hexTable:
        if b == "  ":
            decodedData.append(" ")
        else:
            if 0 <= int(b, 16) <= 31 or 127 <= int(b, 16) <= 159:
                decodedText = "."
            else:
                decodedText = binascii.unhexlify(b).decode("latin-1", "ignore")
            if not decodedText:
                decodedData.append(".")
            else:
                decodedData.append(decodedText)

    for _ in range(len(decodedData), targetLen):
        decodedData.append(" ")

    textOutputF = [hexTable[x:x + split] for x in range(0, len(hexTable), split)]
    decodedDataF = [decodedData[x:x + split] for x in range(0, len(decodedData), split)]

    return textOutputF, decodedDataF


def getResultOfStatement(projId, stmt, params=None):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    result = None
    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        result = connection.execute(text(stmt), params) if params else connection.execute(stmt)
        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)

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
    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (config.DBUSER, config.DBPASS, fluffiResolve(config.DBHOST), "fluffi_gm"))
        connection = engine.connect()
        result = connection.execute(text(stmt), params) if params else connection.execute(stmt)
        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)

    return result


def insertOrUpdateNiceName(projId, myGUID, myLocalID, newName, command, elemType):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
        
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()

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

        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)
        msg, status = "Error: Could not {} testcase".format(command), "error"    

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
    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), "fluffi_gm"))
        connection = engine.connect()

        resultLM = connection.execute(text(GET_LOCAL_MANAGERS), {"fuzzjobID": projId})

        for row in resultLM:
            localManager = {"ServiceDescriptorGUID": row["ServiceDescriptorGUID"],
                            "ServiceDescriptorHostAndPort": row["ServiceDescriptorHostAndPort"]}
            localManagers.append(localManager)
        
        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)                

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
    
    try:
        engineOne = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connectionOne = engineOne.connect()
        engineTwo = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), "fluffi_gm"))
        connectionTwo = engineTwo.connect()

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
        
        connectionOne.close()
        engineOne.dispose()
        connectionTwo.close()
        engineTwo.dispose()
    except Exception as e:
        print(e)

    managedInstances["instances"] = sorted(managedInstances["instances"], key = lambda k: k["AgentType"])
    average = sumOfAverageRTT / numOfRTT if numOfRTT != 0 else 0
    summarySection['AverageRTT'] = round(average, 1)

    return managedInstances, summarySection, localManagers


def getViolationsAndCrashes(projId):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    violationsAndCrashes = type('', (), {})()
    violationsAndCrashes.testcases = []
    violationsAndCrashes.project = project
    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()

        result = connection.execute(GET_VIOLATIONS_AND_CRASHES)

        for row in result:
            testcase = type('', (), {})()
            testcase.count = row["count(*)"]
            testcase.footprint = row["CrashFootprint"]
            testcase.type = row["TestCaseType"]
            violationsAndCrashes.testcases.append(testcase)
        
        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)        

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
            
            connection.close()
            engine.dispose()
        except Exception as e:
            print(e)
            return "Error: Could not edit module", "error"            
    else:
        return "Error: Could not find project", "error"

    return "Edit module was successful", "success"


def insertSettings(projId, request, settingForm):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()

    if project is not None:        
        try:
            engine = create_engine(
                'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
            connection = engine.connect()

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

            connection.close()
            engine.dispose()
        except Exception as e:
            print(e)
            return "Error: Could not add setting", "error"            
    else:
        return "Error: Could not find project", "error"

    return "Added setting", "success"


def setNewBasicBlocks(targetFile, projId):
    basicBlocks = []
    targetFile.seek(0)
    data = targetFile.read()
    reader = csv.reader(data.decode('utf-8').splitlines())
    for row in reader:
        basicBlocks.append(row)

    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()

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

        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)
        return "Error: Failed to add new BasicBlocks to database", "error"        

    return "Added new BasicBlocks to database", "success"


def executeResetFuzzjobStmts(projId, deletePopulation):
    project = getProject(projId)

    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()

        for stmt in ResetFuzzjobStmts:
            connection.execute(stmt)

        if deletePopulation:
            connection.execute(DELETE_TESTCASES)
            connection.execute(RESET_RATING)
        else:
            connection.execute(DELETE_TESTCASES_WITHOUT_POPULATION)
            connection.execute(RESET_RATING)

        connection.close()
        engine.dispose()

        return "Reset Fuzzjob was successful", "success"
    except Exception as e:
        print(e)
        return "Error: Failed to reset fuzzjob", "error"        


def insertModules(projId, request):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    if project is None:
        return "Error: Project not found", "error"

    msg, category = "", ""    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()

        if 'targetModules' in request.files:
            for module in request.files.getlist("targetModules"):
                if module.filename:
                    data = {"ModuleName": module.filename, "ModulePath": "*", "RawBytes": module.read()}
                    statement = text(INSERT_MODULE)
                    connection.execute(statement, data)
                    msg, category = "Added Module(s)", "success"
                else:
                    msg, category = "Error: Filename cannot be empty", "error"
        
        connection.close()
        engine.dispose()        
    except Exception as e:
        msg, category = "Error: Failed to add module - {}".format(e), "error"                    

    return msg, category


def createByteIOTestcase(projId, testcaseId):
    filename = ""
    (guid, localId) = testcaseId.split(":")
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    byteIO = io.BytesIO()
    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
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

        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)
        abort(400)

    return byteIO, filename


def createByteIOForSmallestVioOrCrash(projId, footprint):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    byteIO = io.BytesIO()
    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()

        data = {"footprint": footprint}
        statement = text(GET_SMALLEST_VIO_OR_CRASH_TC)
        result = connection.execute(statement, data)

        rawBytes = result.fetchone()[0]
        byteIO.write(rawBytes)
        byteIO.seek(0)

        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)
        abort(400)        

    return byteIO


def addCommand(projId, guid, hostAndPort = None):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()

        if not hostAndPort:
            data = {"guid": guid}
            statement = text(MANAGED_INSTANCES_HOST_AND_PORT)
            result = connection.execute(statement, data)
            hostAndPort = result.fetchone()[0]

        command = models.CommandQueue(Command = "kill", Argument = hostAndPort, Done = 0)
        db.session.add(command)
        db.session.commit()

        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)


def addCommandToKillInstanceType(projId, myType):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()

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
        
        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)


def insertTestcases(projId, files):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()

    if project is None:
        return "Fuzzjob does not exist", "error"
    
    msg, category = "", ""
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()

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
        
        connection.close()
        engine.dispose()

        msg, category = "Added Testcase(s)", "success"
    except Exception as e:
        if "Duplicate entry" in str(e):
            msg, category = "SQLAlchemy Exception: Duplicate entry for key", "error"  
        else:     
            msg, category = "Error: Failed to add Testcase - {}".format(e), "error"  

    return msg, category


def updateSettings(projId, settingId, settingValue):
    if len(settingValue) > 0:
        project = models.Fuzzjob.query.filter_by(ID=projId).first()
        
        try:
            engine = create_engine(
                'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
            connection = engine.connect()

            statement = text(UPDATE_SETTINGS)
            data = {"ID": settingId, "SettingValue": settingValue}
            connection.execute(statement, data)
            message = "Setting modified"
            status = "OK"

            connection.close()
            engine.dispose()
        except Exception as e:
            print(e)
            message = "Could not modify setting"
            status = "Error"            
    else:
        message = "Value cannot be empty!"
        status = "Error"

    return message, status


def deleteElement(projId, elementName, query, data):
    project = models.Fuzzjob.query.filter_by(ID = projId).first()
    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()

        connection.execute(text(query), data)
        msg, category = elementName + " deleted", "success"

        connection.close()
        engine.dispose()
    except Exception as e:
        msg, category = "Error: Could not delete {}".format(elementName), "error"

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
            
            if agentType is not None:    
                arch = request.form.get(key + '_arch', None)     

                try:
                    valueAsInt = int(value)
                except ValueError:
                    valueAsInt = -1              
                 
                if agentType == 4:
                    fuzzjobId = None
                else: 
                    fuzzjobName = key[:-3]      
                    fj = models.Fuzzjob.query.filter_by(name=fuzzjobName).first()   
                    if fj is None:
                        continue
                    else:           
                        fuzzjobId = fj.ID       

                instance = models.SystemFuzzjobInstances.query.filter_by(System=sys.ID, Fuzzjob=fuzzjobId,
                                                                        AgentType=agentType).first()
                                                        
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
                        newInstance = models.SystemFuzzjobInstances(System=sys.ID, Fuzzjob=fuzzjobId,
                                                                    AgentType=agentType,
                                                                    InstanceCount=valueAsInt, Architecture=arch)
                        db.session.add(newInstance)
                db.session.commit()                                                
        return "Configured Instances!", "success"            
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
        return "Configured Instances!", "success"            
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
    
    try:
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()

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

        connection.close()
        engine.dispose()

        return ["Created new project", "success", project.ID]
    except Exception as e:
        print(e)
        return ["Error: " + str(e), "error"]
        


def uploadNewTargetZipHandler(projId, targetFile):
    try:
        targetFileData = targetFile.read()
        targetFileName = targetFile.filename
    except Exception as e:
        print(e)
        return "Error: Failed to read file!", "error"
    
    try:
        existingPackage = db.session.query(models.DeploymentPackages, models.FuzzjobDeploymentPackages).filter(
                models.DeploymentPackages.ID == models.FuzzjobDeploymentPackages.DeploymentPackage).filter(
                    models.FuzzjobDeploymentPackages.Fuzzjob == projId).first()                                            
    except Exception as e:
        print(e)
        return "Error: Failed to query existing packages!", "error"
    
    if existingPackage is not None:  
        try:
            FTP_CONNECTOR.saveTargetFileOnFTPServer(targetFileData, existingPackage.DeploymentPackages.name)
            return "Updated target!", "success"
        except Exception as e:
            print(e)  
            return "Error: Failed to update target!", "error"                                                     
    try:
        FTP_CONNECTOR.saveTargetFileOnFTPServer(targetFileData, targetFileName)
        
        newDeploymentPackage = models.DeploymentPackages(name=targetFileName)
        db.session.add(newDeploymentPackage)
        db.session.commit()
        
        newFjDeploymentPackage = models.FuzzjobDeploymentPackages(Fuzzjob=projId, DeploymentPackage=newDeploymentPackage.ID)
        db.session.add(newFjDeploymentPackage)
        db.session.commit()
        
        return "Uploaded new target!", "success"
    except Exception as e:
        print(e)
        return "Error: Failed saving target!", "error" 


def removeAgents(projId):
    instances = models.SystemFuzzjobInstances.query.filter_by(Fuzzjob=projId).all()
    for instance in instances:
        if instance is not None:
            db.session.delete(instance)
    db.session.commit()
    

def getGraphData(projId):
    project = models.Fuzzjob.query.filter_by(ID=projId).first()
    graphdata = dict()
    nodes = []    
    edges = []
    nodeIds = []
    
    try:
        engine = create_engine('mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, project.DBHost, project.DBName))
        connection = engine.connect()

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

        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)

    graphdata["nodes"] = nodes
    graphdata["edges"] = edges
    
    return graphdata


def getAllCoverageData():
    try:
        projects = models.Fuzzjob.query.all()
    except Exception as e:
        print(e)
        projects = []
        
    labels = []
    data = []
    colors = ["#3e95cd", "#8e5ea2","#3cba9f","#e8c3b9","#c45850"]
    
    for project in projects:
        try:
            myProject = models.Fuzzjob.query.filter_by(ID = project.ID).first()
            engine = create_engine(
                'mysql://%s:%s@%s/%s' % (myProject.DBUser, myProject.DBPass, fluffiResolve(myProject.DBHost), myProject.DBName))
            connection = engine.connect()

            result = connection.execute(GET_COUNT_OF_COVERED_BLOCKS).first()  
                                              
            if myProject.name and result["CoveredBlocks"]:
                labels.append(myProject.name )
                data.append(result["CoveredBlocks"])  
                colors.append(getRandomColor())
            
            connection.close()
            engine.dispose()
        except Exception as e:
            print(e)
            
    return {
        "labels": labels,
        "data": data,
        "colors": colors
    }


def getCoverageData(projId):
    labels = []
    data = []
    colors = ["#3e95cd", "#8e5ea2","#3cba9f","#e8c3b9","#c45850"]
    
    try:
        project = models.Fuzzjob.query.filter_by(ID = projId).first()        
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()

        result = connection.execute(GET_TARGET_MODULES)         
        for row in result:  
            if row["ModuleName"] and row["CoveredBlocks"]:      
                labels.append(row["ModuleName"])
                data.append(row["CoveredBlocks"])  
                colors.append(getRandomColor())
                    
        connection.close()
        engine.dispose()
    except Exception as e:
        print(e)
       
    return {
        "labels": labels,
        "data": data,
        "colors": colors
    }


def getCoverageDiffData(projId, testcaseId):    
    modules = []  
         
    try:
        project = models.Fuzzjob.query.filter_by(ID = projId).first()
        
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        
        data = { "ID": testcaseId }
        statement = text(GET_TESTCASE_AND_PARENT)
        result = connection.execute(statement, data).fetchone()
        
        tcLocalID = result["CreatorLocalID"]
        tcSdGuid = result["CreatorServiceDescriptorGUID"]
        
        parentLocalID = result["ParentLocalID"]                  
        parentSdGuid = result["ParentServiceDescriptorGUID"]   
                                             
        if result["NiceName"] is not None:
            tcNiceName = result["NiceName"]
        elif result["NiceNameMI"]:
            tcNiceName = "{}:{}".format(result["NiceNameMI"], parentLocalID) 
        else:
            tcNiceName = "{}:{}".format(tcSdGuid, tcLocalID)
            
        if result["ParentNiceName"] is not None:
            parentNiceName = result["ParentNiceName"]
        elif result["ParentNiceNameMI"]:
            parentNiceName = "{}:{}".format(result["ParentNiceNameMI"], parentLocalID) 
        else:
            parentNiceName = "{}:{}".format(parentSdGuid, parentLocalID) 

        data = { "ctID": testcaseId }
        statement = text(GET_COVERED_BLOCKS_OF_TESTCASE_FOR_EVERY_MODULE)
        result = connection.execute(statement, data)   

        for row in result:
            moduleName = row["ModuleName"] if row["ModuleName"] is not None else ""
            if moduleName:
                coveredBlocks = row["CoveredBlocks"] if row["CoveredBlocks"] is not None else 0
                modules.append({
                    "moduleName": moduleName, 
                    "data": {
                        "tcName": tcNiceName, 
                        "tcBlocks": coveredBlocks, 
                        "parentName": parentNiceName
                    }
                })
                    
        result = connection.execute(text(GET_PARENT_ID), { "parentID": parentLocalID, "parentSdGuid": parentSdGuid }).fetchone()    
        if "ID" in result:        
            parentID = result["ID"] if result["ID"] is not None else 0 
        else:
            parentID = 0
        
        data = { "ctID": parentID }
        
        statement = text(GET_COVERED_BLOCKS_OF_TESTCASE_FOR_EVERY_MODULE)
        result = connection.execute(statement, data)          
        for row in result:
            moduleName = row["ModuleName"] if row["ModuleName"] is not None else ""
            if moduleName:
                coveredBlocks = row["CoveredBlocks"] if row["CoveredBlocks"] is not None else 0
                for m in modules:
                    if m["moduleName"] == moduleName:
                        m["data"].update({ "parentBlocks": coveredBlocks })
                        m["data"].update({ "diff": m["data"]["tcBlocks"] - coveredBlocks })
                        break
                        
        connection.close()
        engine.dispose()
        status, msg = "OK", ""
    except Exception as e:
        print(e) 
        status, msg = "ERROR", str(e)
          
    return {
        "modules": modules,
        "status": status,
        "message": msg
    }       
        
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
