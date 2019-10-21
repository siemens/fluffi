# Copyright 2017-2019 Siemens AG
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# Author(s): Pascal Eckmann

from sqlalchemy import *
from app import db, models
from app.queries import *
from app.helpers import *
from app.constants import *

import subprocess
import sys
import time


lock_path = ""


class CreateTestcaseArchive:
    def __init__(self, projId, nice_name, name_list, count_statement_list, statement_list, tmp_path):
        self.tmp_path = tmp_path
        self.progress = 0
        self.projId = projId
        self.nice_name = nice_name
        self.name_list = name_list
        self.count_statement_list = count_statement_list
        self.statement_list = statement_list
        self.max_val_list = []
        # status: 0 = default/running, 1 = success, 2 = error
        self.status = (0, "")
        self.type = "download"

    def set_values(self):
        project = models.Fuzzjob.query.filter_by(id=self.projId).first()
        engine = create_engine(
            'mysql://%s:%s@%s/%s' % (project.DBUser, project.DBPass, fluffiResolve(project.DBHost), project.DBName))
        connection = engine.connect()
        os.makedirs(self.tmp_path, exist_ok=True)
        try:
            for count_statement in self.count_statement_list:
                result = connection.execute(count_statement)
                self.max_val_list.append(int((result.fetchone()[0]) / 20) + 1)

            max_val = 0
            for val in self.max_val_list:
                max_val = max_val + val

            if len(self.name_list) == 1:
                download = self.name_list[0] + ".zip"
            else:
                download = "testcase_set.zip"

            lock_write_file(self.type, self.nice_name, max_val, download)
        except Exception as e:
            lock_change_file_entry("STATUS", "2")
            lock_change_file_entry("MESSAGE", str(e))
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
            if lock_read_file_entry("END") == "0":
                for num, statement in enumerate(self.statement_list):
                    if lock_read_file_entry("END") == "0":
                        tmp_full_path = str(self.tmp_path + "/" + self.name_list[num])
                        if os.path.exists(tmp_full_path):
                            shutil.rmtree(tmp_full_path, ignore_errors=True)
                        os.makedirs(tmp_full_path)
                        for block in range(0, self.max_val_list[num]):
                            if lock_read_file_entry("END") == "0":
                                self.progress = self.progress + 1
                                lock_change_file_entry("PROGRESS", str(self.progress))
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
                                    try:
                                        f = open(tmp_full_path + "/" + fileName, "wb+")
                                        f.write(rawData)
                                        f.close()
                                    except Exception as e:
                                        print("Error Write File to " + tmp_full_path + " " + str(e))

                        if len(self.statement_list) == num + 1:
                            if len(self.statement_list) > 1:
                                filename = shutil.make_archive("testcase_set", "zip", self.tmp_path)
                            else:
                                filename = shutil.make_archive(self.name_list[num], "zip", tmp_full_path)

                            lock_change_file_entry("STATUS", "1")
                            lock_change_file_entry("MESSAGE", "File " + filename + " created.")
                            lock_change_file_entry("END", "1")
                            self.status = (1, "File " + filename + " created.")
                            for name in self.name_list:
                                if os.path.exists(self.tmp_path + "/" + name):
                                    shutil.rmtree(self.tmp_path + "/" + name, ignore_errors=True)

        except Exception as e:
            print("Error Thread:", str(e))
            lock_change_file_entry("STATUS", "2")
            lock_change_file_entry("MESSAGE", str(e))
            lock_change_file_entry("END", "1")
            self.status = (2, str(e))
        finally:
            connection.close()
            engine.dispose()


class ArchiveProject:
    def __init__(self, projId, nice_name):
        self.projId = projId
        self.nice_name = nice_name
        # status: 0 = default/running, 1 = success, 2 = error, 3 = finish
        self.status = (0, "Step 0/4: Start archiving fuzzjob.")
        self.type = "archive"

    def set_values(self):
        lock_write_file(self.type, self.nice_name)
        lock_change_file_entry("STATUS", "0")
        lock_change_file_entry("MESSAGE", "Step 0/4: Start archiving fuzzjob.")

    def run(self):
        fuzzjob = models.Fuzzjob.query.filter_by(id=self.projId).first()

        if fuzzjob or (lock_read_file_entry("END") == "0"):
            try:
                scriptFile = os.path.join(app.root_path, "archiveDB.sh")
                returncode = subprocess.call(
                    [scriptFile, config.DBUSER, config.DBPASS, config.DBPREFIX, fuzzjob.name.lower(), config.DBHOST])
                if returncode != 0:
                    lock_change_file_entry("STATUS", "2")
                    lock_change_file_entry("MESSAGE", "Error Step 1/4: Database couldn't be archived.")
                    lock_change_file_entry("END", "1")
                    self.status = (2, str("Error Step 1/4: Database couldn't be archived."))
                else:
                    lock_change_file_entry("STATUS", "1")
                    lock_change_file_entry("MESSAGE", "Step 1/4: Database archived.")
                    self.status = (1, str("Step 1/4: Database archived."))
            except Exception as e:
                print(e)
                lock_change_file_entry("STATUS", "2")
                lock_change_file_entry("MESSAGE", str(e))
                lock_change_file_entry("END", "1")
                self.status = (2, str(e))

            if self.status[0] is not 2 or (lock_read_file_entry("END") == "0"):
                fileName = config.DBPREFIX + fuzzjob.name.lower() + ".sql.gz"
                success = FTP_CONNECTOR.saveArchivedProjectOnFTPServer(fileName)
                if success:
                    lock_change_file_entry("STATUS", "1")
                    lock_change_file_entry("MESSAGE", "Step 2/4: Archived project saved on ftp server.")
                    self.status = (1, "Step 2/4: Archived project saved on ftp server.")
                else:
                    lock_change_file_entry("STATUS", "2")
                    lock_change_file_entry("MESSAGE",
                                                "Step 2/4: Archived project couldn't be saved on ftp server.")
                    lock_change_file_entry("END", "1")
                    self.status = (2, "Step 2/4: Archived project couldn't be saved on ftp server.")

                os.remove(fileName)

                if self.status[0] == 1 or (lock_read_file_entry("END") == "0"):
                    try:
                        if models.Fuzzjob.query.filter_by(id=self.projId).first() is not None:
                            fuzzjob = models.Fuzzjob.query.filter_by(id=self.projId).first()
                            db.session.delete(fuzzjob)
                            db.session.commit()
                            lock_change_file_entry("STATUS", "1")
                            lock_change_file_entry("MESSAGE", "Step 3/4: Fuzzjob sucessfully deleted.")
                            self.status = (1, 'Step 3/4: Fuzzjob sucessfully deleted.')
                        else:
                            lock_change_file_entry("STATUS", "2")
                            lock_change_file_entry("MESSAGE", "Step 3/4: Fuzzjob not found.")
                            lock_change_file_entry("END", "1")
                            self.status = (2, 'Step 3/4: Fuzzjob not found.')
                    except Exception as e:
                        lock_change_file_entry("STATUS", "2")
                        lock_change_file_entry("MESSAGE", str(e))
                        lock_change_file_entry("END", "1")
                        self.status = (2, str(e))

                    if self.status[0] == 1 or lock_read_file_entry("END") == "0":
                        engine = create_engine(
                            'mysql://%s:%s@%s/%s' % (
                                config.DBUSER, config.DBPASS, fluffiResolve(config.DBHOST), config.DEFAULT_DBNAME))
                        connection = engine.connect()
                        try:
                            connection.execute("DROP DATABASE {};".format(config.DBPREFIX + fuzzjob.name.lower()))
                            lock_change_file_entry("STATUS", "3")
                            lock_change_file_entry("MESSAGE", "Step 4/4: Database deleted.")
                            lock_change_file_entry("END", "1")
                            self.status = (3, 'Step 4/4: Database deleted.')
                        except Exception as e:
                            lock_change_file_entry("STATUS", "2")
                            lock_change_file_entry("MESSAGE", str(e))
                            self.status = (2, str(e))
                        finally:
                            connection.close()
                            engine.dispose()


def get_testcase_data():
    name = [
        "population",
        "accessViolations",
        "accessViolationsUnique",
        "crashes",
        "crashesUnique",
        "hangs",
        "noResponses"
    ]
    statement_count = [
        getITCountOfTypeQuery(TESTCASE_TYPES[name[0]]),
        getITCountOfTypeQuery(TESTCASE_TYPES[name[1]]),
        NUM_UNIQUE_ACCESS_VIOLATION,
        getITCountOfTypeQuery(TESTCASE_TYPES[name[3]]),
        NUM_UNIQUE_CRASH,
        getITCountOfTypeQuery(TESTCASE_TYPES[name[5]]),
        getITCountOfTypeQuery(TESTCASE_TYPES[name[6]]),
    ]
    statement = [
        getITQueryOfType(TESTCASE_TYPES[name[0]]),
        getITQueryOfType(TESTCASE_TYPES[name[1]]),
        UNIQUE_ACCESS_VIOLATION,
        getITQueryOfType(TESTCASE_TYPES[name[3]]),
        UNIQUE_CRASHES,
        getITQueryOfType(TESTCASE_TYPES[name[5]]),
        getITQueryOfType(TESTCASE_TYPES[name[6]]),
    ]
    return name, statement_count, statement


def get_download_path():
    return app.root_path[:-3]


def remove_zips(name):
    for entry in name:
        if len(name) == 1:
            zip_file_path = get_download_path() + entry + ".zip"
        else:
            zip_file_path = get_download_path() + "testcase_set.zip"
        if os.path.isfile(zip_file_path):
            os.remove(zip_file_path)
            
            
def lock_check_file():
    file_available = False
    for x in range(0, 2):
        if os.access(lock_path, os.F_OK):
            file_available = True
        time.sleep(0.01)

    return not file_available


def lock_write_file(type, nice_name, max_val=0, download="", pid=0):
    if lock_check_file():
        file_w = open(lock_path, "a")
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


def lock_read_file():
    if os.access(lock_path + ".bak", os.R_OK):
        try:
            os.rename(lock_path + ".bak", lock_path)
        except Exception as e:
            print(str(e))
    if os.access(lock_path, os.R_OK):
        file_r = open(lock_path, "r")
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


def lock_read_file_entry(entry):
    if os.access(lock_path + ".bak", os.R_OK):
        try:
            os.rename(lock_path + ".bak", lock_path)
        except Exception as e:
            print(str(e))
    if os.access(lock_path, os.R_OK):
        file_r = open(lock_path, "r")
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


def lock_change_file_entry(entry, value):
    if os.access(lock_path, os.R_OK):
        file = open(lock_path, 'r')
        lines = file.readlines()
        file.close()
        for num, line in enumerate(lines):
            if entry in line:
                lines[num] = entry + "=" + value + "\n"
        out = open(lock_path + ".bak", 'w+')
        out.writelines(lines)
        out.close()
        os.rename(lock_path + ".bak", lock_path)


def lock_delete_file():
    if os.access(lock_path, os.R_OK):
        os.remove(lock_path)
    else:
        if (os.access(lock_path, os.R_OK) and
                lock_read_file_entry("END") == "1"):
            os.remove(lock_path)


#   Number              Values

#   1                   absolute path of lock file
#   2                   projId
#   3                   nice name
#   4                   download/archive: what should this script do
#   5  if download      type, what to download -> generate name, count_statement, statement
#   6  if download      tmp_path


def main():
    global lock_path
    lock_path = sys.argv[1]
    proj_id = sys.argv[2]
    nice_name = sys.argv[3]
    
    if sys.argv[4] == "download":
        name, statement_count, statement = get_testcase_data()
        position = -1
        data = sys.argv[5].split(" ")
        if data[0] == "population":
            position = 0
        elif data[0] == "accessViolations":
            position = 1
        elif data[0] == "accessViolationsUnique":
            position = 2
        elif data[0] == "crashes":
            position = 3
        elif data[0] == "crashesUnique":
            position = 4
        elif data[0] == "hangs":
            position = 5
        elif data[0] == "noResponses":
            position = 6
        elif data[0] == "crashesFootprintTestcase":
            footprint = data[1]
            testcase = data[2]
            name = [data[0]]
            statement_count = [getCrashesQueryCount(footprint, testcase)]
            statement = [getCrashesQuery(footprint, testcase)]
        elif data[0] == "crashesOrViolationFootprint":
            footprint = data[1]
            name = [data[0]]
            statement_count = [getCrashesOrViosOfFootprintCount(footprint)]
            statement = [getCrashesOrViosOfFootprint(footprint)]
            
        if position >= 0:
            name = [name[position]]
            statement_count = [statement_count[position]]
            statement = [statement[position]]
        
        tmp_path = sys.argv[6]

        process = CreateTestcaseArchive(proj_id, nice_name, name, statement_count, statement, tmp_path)
        process.set_values()
        process.run()

    elif sys.argv[4] == "archive":
        process = ArchiveProject(proj_id, nice_name)
        process.set_values()
        process.run()


if __name__ == "__main__":
    main()
