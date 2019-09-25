# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Junes Najah, Thomas Riedmaier, Pascal Eckmann, Abian Blome

INSERT_SETTINGS = (
    "INSERT INTO settings(SettingName, SettingValue) VALUES(:SettingName, :SettingValue)")
INSERT_MODULE = (
    "INSERT INTO target_modules(ModuleName, ModulePath) VALUES(:ModuleName, :ModulePath)")
INSERT_BLOCK_TO_COVER = (
    "INSERT INTO blocks_to_cover(ModuleID, Offset) VALUES(:ModuleID, :Offset) ON DUPLICATE KEY UPDATE ModuleID = :ModuleID, Offset = :Offset")
NUMBER_OF_NO_LONGER_LISTED = (
    "SELECT Amount FROM billing WHERE Resource='RunTestcasesNoLongerListed'")
COMPLETED_TESTCASES_COUNT = (
    "SELECT COUNT(*) FROM completed_testcases")
NUM_BLOCKS = (
    "SELECT COUNT(DISTINCT ModuleID, Offset) FROM covered_blocks")
GET_SETTINGS = (
    "SELECT Id, SettingName, SettingValue FROM settings")
GET_TARGET_MODULES = (
    "SELECT ID, ModuleName, ModulePath FROM target_modules")
DELETE_TESTCASES = (
    "DELETE FROM interesting_testcases WHERE CreatorServiceDescriptorGUID <> 'initial'")
RESET_RATING = (
    "UPDATE interesting_testcases SET Rating = 10000")
DELETE_TESTCASES_WITHOUT_POPULATION = (
    "DELETE FROM interesting_testcases WHERE TestCaseType <> 0")
RESET_INITIAL_RATING = (
    "UPDATE interesting_testcases SET Rating = 10000 WHERE CreatorServiceDescriptorGUID = 'initial'")
GET_MI_HOST_AND_PORT = (
    "SELECT managed_instances.ServiceDescriptorHostAndPort FROM managed_instances;")
GET_MAX_LOCALID = (
    "SELECT MAX(CreatorLocalID) FROM interesting_testcases WHERE CreatorServiceDescriptorGUID='initial'")
UPDATE_SETTINGS = (
    "UPDATE settings SET SettingValue=:SettingValue WHERE ID=:Id")
GET_RAWBYTES_FROM_IT_WITH_LOCALID = (
    "SELECT RawBytes FROM interesting_testcases WHERE CreatorServiceDescriptorGUID=:guid AND CreatorLocalID=:localId")
UPDATE_NICE_NAME_TESTCASE = (
    "UPDATE nice_names_testcase SET NiceName=:newName WHERE TestcaseID=:testcaseID")
UPDATE_NICE_NAME_MANAGED_INSTANACE = (
    "UPDATE nice_names_managed_instance SET NiceName=:newName WHERE ServiceDescriptorGUID=:sdguid")
DELETE_TC_WTIH_LOCALID = (
    "DELETE FROM interesting_testcases WHERE CreatorServiceDescriptorGUID=:guid AND CreatorLocalID=:localId")
DELETE_MODULE_BY_ID = (
    "DELETE FROM target_modules WHERE id=:Id")
DELETE_SETTING_BY_ID = (
    "DELETE FROM settings WHERE id=:Id")
GET_TOTAL_CPU_SECONDS = (
    "SELECT Amount FROM billing WHERE Resource = 'RunnerSeconds'")
GET_TESTCASE_ID = (
    "SELECT ID FROM interesting_testcases WHERE CreatorServiceDescriptorGUID='initial' "
    "AND CreatorLocalID=:creatorlocalID")
GET_LOCAL_MANAGERS = (
    "SELECT ServiceDescriptorGUID, ServiceDescriptorHostAndPort FROM localmanagers WHERE FuzzJob=:fuzzjobID")
GET_MANAGED_INSTANCES = (
    "SELECT managed_instances.ServiceDescriptorGUID, managed_instances.ServiceDescriptorHostAndPort, "
    "managed_instances.AgentType, managed_instances.Location, mis.TimeOfStatus, mis.Status,"
    "nice_names_managed_instance.NiceName FROM managed_instances "
    "LEFT JOIN (SELECT ServiceDescriptorGUID, Status, TimeOfStatus FROM  managed_instances_statuses t1 "
    "WHERE TimeOfStatus = (SELECT MAX(TimeOfStatus) "
    "FROM managed_instances_statuses "
    "WHERE t1.ServiceDescriptorGUID = managed_instances_statuses.ServiceDescriptorGUID) "
    "GROUP BY ServiceDescriptorGUID "
    "ORDER BY TimeOfStatus DESC) AS mis "
    "ON managed_instances.ServiceDescriptorGUID = mis.ServiceDescriptorGUID "
    "LEFT JOIN nice_names_managed_instance "
    "ON managed_instances.ServiceDescriptorGUID = nice_names_managed_instance.ServiceDescriptorGUID "
    "ORDER BY managed_instances.AgentType;")

GET_VIOLATIONS_AND_CRASHES = (
    "SELECT count(*), cd.CrashFootprint, it.TestCaseType, MIN(it.ID) AS group_min FROM interesting_testcases AS it "
    "JOIN crash_descriptions AS cd ON it.ID = cd.CreatorTestcaseID "
    "GROUP BY cd.CrashFootprint, it.TestCaseType ORDER BY group_min;")

GET_SMALLEST_VIO_OR_CRASH_TC = (
    "SELECT it.RawBytes "
    "FROM interesting_testcases AS it "
    "JOIN crash_descriptions AS cd ON it.ID = cd.CreatorTestcaseID "
    "WHERE cd.CrashFootprint=:footprint ORDER BY LENGTH(it.RawBytes) ASC LIMIT 1;")

NUM_UNIQUE_ACCESS_VIOLATION = (
    "SELECT count(*) FROM "
    "(SELECT cd.CrashFootprint, it.TestCaseType "
    "FROM interesting_testcases AS it "
    "JOIN crash_descriptions AS cd ON it.ID = cd.CreatorTestcaseID "
    "WHERE it.TestCaseType=2 Group by cd.CrashFootprint) "
    "violations")

UNIQUE_ACCESS_VIOLATION = (
    "SELECT av.CrashFootprint, av.TestCaseType, av.CreatorServiceDescriptorGUID, av.CreatorLocalID, av.Rating, "
    "av.TimeOfInsertion, av.ID, av.RawBytes, nn.NiceName "
    "FROM "
    "(SELECT cd.CrashFootprint, it.TestCaseType, it.CreatorServiceDescriptorGUID, it.CreatorLocalID, it.Rating, "
    "it.TimeOfInsertion, it.ID, it.RawBytes"
    " FROM interesting_testcases AS it"
    " JOIN crash_descriptions AS cd ON it.ID = cd.CreatorTestcaseID "
    " WHERE it.TestCaseType=2 Group by cd.CrashFootprint) av "     
    "LEFT JOIN nice_names_testcase AS nn ON av.ID = nn.TestcaseID;")

NUM_UNIQUE_CRASH = (
    "SELECT count(*) "
    "FROM (SELECT cd.CrashFootprint, it.TestCaseType FROM interesting_testcases AS it "
    "JOIN crash_descriptions AS cd ON it.ID = cd.CreatorTestcaseID "
    "GROUP BY cd.CrashFootprint) " 
    "observedCrashes WHERE TestCaseType=3")

UNIQUE_CRASHES = (
    "SELECT oc.CrashFootprint, oc.ID, oc.TestCaseType, oc.RawBytes, oc.CreatorServiceDescriptorGUID, "
    "oc.CreatorLocalID, oc.Rating, oc.TimeOfInsertion, nn.NiceName "
    "FROM "
    "(SELECT cd.CrashFootprint, it.TestCaseType, it.CreatorServiceDescriptorGUID, it.CreatorLocalID, "
    "it.Rating, it.TimeOfInsertion, it.ID, it.RawBytes"
    " FROM interesting_testcases AS it"
    " JOIN crash_descriptions AS cd"
    " ON it.ID = cd.CreatorTestcaseID "
    " GROUP BY cd.CrashFootprint) oc " 
    "LEFT JOIN nice_names_testcase AS nn ON oc.ID = nn.TestcaseID "
    "WHERE TestCaseType=3;")

NUM_UNIQUE_ACCESS_VIOLATION_TYPE_2 = (
    "SELECT count(*) "
    "FROM (SELECT cd.CrashFootprint, it.TestCaseType FROM interesting_testcases AS it "
    "JOIN crash_descriptions AS cd ON it.ID = cd.CreatorTestcaseID "
    "GROUP BY cd.CrashFootprint) "
    "observedCrashes WHERE TestCaseType=2")

MANAGED_INSTANCES_HOST_AND_PORT_AGENT_TYPE = (
    "SELECT managed_instances.ServiceDescriptorHostAndPort "
    "FROM managed_instances "
    "WHERE managed_instances.AgentType =:myType;")

MANAGED_INSTANCES_HOST_AND_PORT = (
    "SELECT managed_instances.ServiceDescriptorHostAndPort "
    "FROM managed_instances "
    "WHERE managed_instances.ServiceDescriptorGUID =:guid LIMIT 1;")

INSERT_TESTCASE_POPULATION = (
    "INSERT INTO interesting_testcases(CreatorServiceDescriptorGUID, CreatorLocalID, ParentServiceDescriptorGUID, "
    "ParentLocalID, RawBytes, Rating, TestCaseType) "
    "VALUES('initial', :localId, 'initial', :localId, :rawData, 10000, 0)")

INSERT_NICE_NAME_TESTCASE = (
    "INSERT INTO nice_names_testcase(NiceName, TestcaseID) "
    "VALUES(:newName, :testcaseID)")

INSERT_NICE_NAME_MANAGED_INSTANACE = (
    "INSERT INTO nice_names_managed_instance(NiceName, ServiceDescriptorGUID) "
    "VALUES(:newName, :sdguid)")

GET_POPULATION_DETAILS = (
    "SELECT DISTINCT it.CreatorServiceDescriptorGUID, it.CreatorLocalID, it.ParentServiceDescriptorGUID, "
    "it.ParentLocalID, nnt.NiceName as NiceNameTC, nnmi.NiceName as NiceNameMI, it.TestCaseType "
    "FROM interesting_testcases as it "
    "LEFT JOIN nice_names_testcase as nnt on it.ID = nnt.TestcaseID "
    "LEFT JOIN nice_names_managed_instance as nnmi on it.CreatorServiceDescriptorGUID = nnmi.ServiceDescriptorGUID "
    "WHERE (it.TestCaseType=0 OR it.TestCaseType=5);")

GET_CHECKED_RATING = (
    "SELECT it.Rating "
    "FROM interesting_testcases AS it "
    "WHERE TestCaseType=0 AND Rating > (SELECT o.value FROM fluffi_gm.gm_options AS o WHERE setting = 'checkrating')"
    "LIMIT 1")

GET_CRASH_DETAILS = (
    "SELECT DISTINCT cd.CrashFootprint FROM crash_descriptions as cd;")

GET_CRASH_PARENTS = (
    "SELECT it.ParentServiceDescriptorGUID, it.ParentLocalID, COUNT(*) AS NumberEdges FROM interesting_testcases as it "
    "JOIN crash_descriptions as cd ON it.ID = cd.CreatorTestcaseID "
    "WHERE cd.CrashFootprint = :CrashFootprint AND (it.TestCaseType=3 OR it.TestCaseType=2) "
    "GROUP BY it.ParentServiceDescriptorGUID, it.ParentLocalID;")

GET_NN_TESTCASE_RAWBYTES = (
    "SELECT it.RawBytes, nnt.NiceName FROM interesting_testcases as it LEFT JOIN nice_names_testcase as nnt "
    "ON it.ID = nnt.TestcaseID "
    "WHERE CreatorServiceDescriptorGUID=:guid AND CreatorLocalID=:localId;")
	
GET_PROJECTS = (
    "SELECT"
        "(SELECT COUNT(*) FROM completed_testcases),"
        "(SELECT Amount FROM billing WHERE Resource='RunTestcasesNoLongerListed'),"
        "SUM(CASE WHEN TestCaseType = 0 THEN 1 ELSE 0 END),"
        "SUM(CASE WHEN TestCaseType = 1 THEN 1 ELSE 0 END),"
        "SUM(CASE WHEN TestCaseType = 2 THEN 1 ELSE 0 END),"
        "SUM(CASE WHEN TestCaseType = 3 THEN 1 ELSE 0 END),"
        "SUM(CASE WHEN TestCaseType = 4 THEN 1 ELSE 0 END),"
        "(SELECT Rating FROM interesting_testcases WHERE TestCaseType=0 AND Rating > "
        "(SELECT o.value FROM fluffi_gm.gm_options AS o WHERE setting = 'checkrating') LIMIT 1)"
    "FROM interesting_testcases;"
)

ResetFuzzjobStmts = [
    "UPDATE billing SET amount = 0",
    "TRUNCATE TABLE completed_testcases",
    "TRUNCATE TABLE covered_blocks",
    "TRUNCATE TABLE crash_descriptions",
    "TRUNCATE TABLE managed_instances",
    "TRUNCATE TABLE managed_instances_statuses",
    "TRUNCATE TABLE nice_names_managed_instance"]


def getCrashesOrViosOfFootprint(footprint):
    return (
        "SELECT it.CreatorServiceDescriptorGUID, it.RawBytes, it.ID, nn.NiceName "
        "FROM interesting_testcases AS it "
        "JOIN crash_descriptions AS cd ON it.ID = cd.CreatorTestcaseID "
        "LEFT JOIN nice_names_testcase AS nn ON it.ID = nn.TestcaseID "
        "WHERE cd.CrashFootprint='{}' AND (it.TestCaseType=2 OR it.TestCaseType=3);".format(footprint)
    )


def getCrashesQuery(footprint, testCaseType):
    return (
        "SELECT it.CreatorServiceDescriptorGUID, it.RawBytes, it.ID, nn.NiceName "
        "FROM interesting_testcases AS it "
        "JOIN crash_descriptions AS cd ON it.ID = cd.CreatorTestcaseID "
        "LEFT JOIN nice_names_testcase AS nn ON it.ID = nn.TestcaseID "
        "WHERE cd.CrashFootprint='{}' AND it.TestCaseType={};".format(footprint, testCaseType)
    )


def getITCountOfTypeQuery(n):
    return "SELECT COUNT(*) FROM interesting_testcases WHERE TestCaseType=" + str(n)


def getITQueryOfType(n):
    return (
        "SELECT it.ID, it.RawBytes, it.CreatorServiceDescriptorGUID, it.CreatorLocalID, it.Rating, it.TimeOfInsertion, "
        "nn.NiceName "
        "FROM interesting_testcases AS it "
        "LEFT JOIN nice_names_testcase AS nn ON it.ID = nn.TestcaseID "
        "WHERE TestCaseType={};".format(n)
    )


def getITQueryOfTypeNoRaw(n):
    return (
        "SELECT it.ID, it.CreatorServiceDescriptorGUID, it.CreatorLocalID, it.Rating, it.TimeOfInsertion, "
        "nn.NiceName "
        "FROM interesting_testcases AS it "
        "LEFT JOIN nice_names_testcase AS nn ON it.ID = nn.TestcaseID "
        "WHERE TestCaseType={} LIMIT 1000;".format(n)
    )


def getMICountOfTypeQuery(n):
    return "SELECT COUNT(*) FROM managed_instances WHERE AgentType=" + str(n)
