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
# Author(s): Junes Najah, Thomas Riedmaier

COUNT_COVERED_BLOCKS="SELECT COUNT(DISTINCT ModuleID, Offset) FROM covered_blocks;"

COUNT_TCTYPE="SELECT COUNT(*) FROM interesting_testcases WHERE TestCaseType=:tcType;"

UNIQUE_TESTCASES=(
    "SELECT COUNT(*) FROM "
        "(SELECT cd.CrashFootprint, it.TestCaseType FROM interesting_testcases AS it "
        "JOIN crash_descriptions AS cd ON it.ID = cd.CreatorTestcaseID "
        "WHERE TestCaseType=:tcType GROUP BY cd.CrashFootprint) testcases;"
)

INSTANCE_PERFORMANCE_STATS=(
    "SELECT managed_instances_statuses.ID, managed_instances_statuses.ServiceDescriptorGUID, managed_instances.AgentType, "
    "managed_instances.ServiceDescriptorHostAndPort, managed_instances.Location, managed_instances_statuses.Status, managed_instances_statuses.TimeOfStatus "
    "FROM managed_instances_statuses "
    "LEFT JOIN managed_instances ON managed_instances_statuses.ServiceDescriptorGUID=managed_instances.ServiceDescriptorGUID;"
)
				

### GM SOURCE ###
RUNNING_PROJECTS="SELECT DISTINCT fuzzjob.name FROM fuzzjob RIGHT JOIN localmanagers on fuzzjob.id=localmanagers.fuzzjob;"

PROJECT="SELECT DBHost, DBUser, DBPass, DBName FROM fuzzjob WHERE name=:projectName;"


