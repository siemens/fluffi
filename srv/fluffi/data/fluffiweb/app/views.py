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
# Author(s): Pascal Eckmann, Junes Najah, Michael Kraus, Abian Blome, Fabian Russwurm, Thomas Riedmaier

from flask import flash, get_flashed_messages, redirect, url_for, request, send_file, Markup
from werkzeug.exceptions import HTTPException
from functools import wraps

from .controllers import *
from .queries import *
from .helpers import *
from .forms import *
from .constants import *
from .utils.sync import *

import json
import os
import requests
import socket

lock = DownloadArchiveLockFile()


@app.before_first_request
def updateSystems():
    try:
        ANSIBLE_REST_CONNECTOR.execHostAlive()
    except Exception as e:
        print("Ansible Connector failed! " + str(e))


@app.before_first_request
@app.route("/syncSystems")
def syncSystems():
    loadSystems()
    return redirect("/")


def checkDBConnection(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        try:
            _ = models.Locations.query.all()
        except Exception as e:
            print(e)
            return renderTemplate("index.html",
                          title="DB Error",
                          fuzzjobs=[],
                          locations=[],
                          inactivefuzzjobs=[],
                          errorMessage="Database connection failed! " + str(e),
                          footer=FOOTER_SIEMENS)        
            
        return f(*args, **kwargs)
    return wrapper


def checkSystemsLoaded(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        locationsCount = len(models.Locations.query.all())

        if getSyncStatus() and locationsCount == 0:
            newFlash = 0
            for messages in get_flashed_messages(with_categories=True):
                if "addLocation" == messages[0]:
                    newFlash += 1
            if newFlash < 2:
                flash("A location is necessary to add systems to the database.", "addLocation")
        elif getSyncStatus() and locationsCount > 0:
            loadSystems()
            if not checkSyncedFile():
                newFlash = 0
                for messages in get_flashed_messages(with_categories=True):
                    if "syncSystems" == messages[0]:
                        newFlash += 1
                if newFlash < 2:
                    flash("Something went wrong at the inital synchronization between PoleMarch and the database. Please retry!", "syncSystems")
            
        return f(*args, **kwargs)
    return wrapper


@app.route("/")
@app.route("/index")
@checkDBConnection
@checkSystemsLoaded
def index():
    inactivefuzzjobs = []
    activefuzzjobs = []

    fuzzjobs = listFuzzJobs()  

    for project in fuzzjobs:
        if ((int(project.numLM) > 0) and
                (int(project.numTE) > 0) and
                (int(project.numTR) > 0) and
                (int(project.numTG) > 0)):
            activefuzzjobs.append(project)
        else:
            inactivefuzzjobs.append(project)

    locations = getLocations()

    return renderTemplate("index.html",
                          title="Home",
                          fuzzjobs=activefuzzjobs,
                          locations=locations,
                          inactivefuzzjobs=inactivefuzzjobs,
                          errorMessage="",
                          footer=FOOTER_SIEMENS)


@app.route("/projects")
@checkDBConnection
@checkSystemsLoaded
def projects():
    projects = getProjects()

    return renderTemplate("projects.html",
                          title="Projects",
                          projects=projects)


@app.route("/projects/archive/<int:projId>", methods=["POST"])
def archiveProject(projId):
    project = models.Fuzzjob.query.filter_by(ID=projId).first()
    nice_name = project.name
    removeAgents(projId)

    if lock.check_file():
        proc = subprocess.Popen(["/usr/bin/python3.6", "./app/utils/downloader.py", lock.file_path, str(projId), nice_name, "archive"])
        pid_ = proc.pid
        time_to_wait = 20
        time_counter = 0
        while not os.path.exists(lock.file_path):
            time.sleep(0.25)
            time_counter += 1
            if time_counter > time_to_wait:
                break
        lock.change_file_entry("PID", str(pid_))

    return redirect("/statusDownload")


@app.route('/progressArchiveFuzzjob')
def progressArchiveFuzzjob():
    if not lock.check_file():
        file_content = lock.read_file()
        if file_content['STATUS'] == "0" or file_content['STATUS'] == "1":
            return file_content['MESSAGE']
        elif file_content['STATUS'] == "2" or file_content['STATUS'] == "3":
            error_message = file_content['MESSAGE']
            subprocess.Popen(["kill", file_content['PID']])
            lock.delete_file()
            return error_message
        else:
            return file_content['MESSAGE']
    else:
        return ""


@app.route("/locations/<int:locId>/delProject/<int:projId>")
def locationRemoveProject(locId, projId):
    try:
        location = models.LocationFuzzjobs.query.filter_by(Location=locId, Fuzzjob=projId).first()
        if location is not None:
            db.session.delete(location)
            db.session.commit()
        else:
            flash("Project in location not found", "error")
            return redirect("/locations/view/%s" % locId)
    except HTTPException as error:
        return error("")

    flash("Project deleted from location deleted", "success")

    return redirect("/locations/view/%s" % locId)


@app.route("/projects/<int:projId>/setModuleBinaryAndPath/<int:moduleId>", methods=["GET", "POST"])
@checkDBConnection
@checkSystemsLoaded
def setModuleBinaryAndPath(projId, moduleId):       
    if request.method == 'POST' and "moduleBinaryFile" in request.files:         
        moduleName = request.form.get("moduleName")
        modulePath = request.form.get("modulePath")
        moduleBinaryFile = request.files["moduleBinaryFile"]

        if moduleName or modulePath or moduleBinaryFile:
            formData = { "ModuleName": moduleName, "ModulePath": modulePath }  
            rawBytesData = { "RawBytes": moduleBinaryFile.read() }    
            msg, category = updateModuleBinaryAndPath(projId, moduleId, formData, rawBytesData)
        else:
            msg, category = "Error: Form was empty.", "error"

        flash(msg, category)
        return redirect("/projects/view/%d" % projId)

    project = getProject(projId)
    locationForm = getLocationFormWithChoices(projId, AddProjectLocationForm())
    moduleForm = CreateProjectModuleForm()

    return renderTemplate("viewProject.html",
                          title="Project details",
                          project=project,
                          locationForm=locationForm)


@app.route("/projects/<int:projId>/addSetting", methods=["GET", "POST"])
@checkDBConnection
@checkSystemsLoaded
def createSetting(projId):
    settingForm = CreateProjectSettingForm()

    if request.method == 'POST' and settingForm.is_submitted:
        msg, category = insertSettings(projId, request, settingForm)
        flash(msg, category)
        return redirect("/projects/view/%d" % projId)

    project = getProject(projId)
    locationForm = getLocationFormWithChoices(projId, AddProjectLocationForm())
    moduleForm = CreateProjectModuleForm()

    return renderTemplate("viewProject.html",
                          title="Project details",
                          project=project,
                          locationForm=locationForm,
                          settingForm=settingForm,
                          moduleForm=moduleForm)


@app.route("/projects/<int:projId>/uploadNewTargetZip", methods=["GET", "POST"])
def uploadNewTargetZip(projId):
    if request.method == "POST":
        targetFile = request.files["uploadFile"]
        targetFileName = targetFile.filename
        
        if not targetFile:
            flash("Invalid file", "error")
            return redirect(request.url)
        
        if not ('.' in targetFile.filename and targetFile.filename.rsplit('.', 1)[1].lower() in set(["zip"])):
            flash("Only *.zip files are allowed!", "error")
            return redirect(request.url)
               
        msg, category = uploadNewTargetZipHandler(projId, targetFile)        
        flash(msg, category)
        return redirect("/projects/view/%d" % projId)
    
    return redirect("/projects/view/%d" % projId)


@app.route("/projects/<int:projId>/uploadNewBasicBlocks", methods=["GET", "POST"])
def uploadNewBasicBlocks(projId):
    if request.method == "POST":
        targetFile = request.files["uploadFile"]
        if not targetFile:
            flash("Invalid file", "error")
            return redirect(request.url)
        if not ('.' in targetFile.filename and targetFile.filename.rsplit('.', 1)[1].lower() in {"txt", "csv"}):
            flash("Only *.txt or *.csv files are allowed!", "error")
            return redirect(request.url)
        msg, category = setNewBasicBlocks(targetFile, projId)
        flash(msg, category)
        return redirect("/projects/view/%d" % projId)

    return redirect("/projects/view/%d" % projId)


@app.route("/projects/<int:projId>/resetFuzzjob", methods=["GET", "POST"])
def resetFuzzjob(projId):
    deletePopulation = request.form.get("deletePopulation") == "delete"
    msg, category = executeResetFuzzjobStmts(projId, deletePopulation)
    flash(msg, category)

    return redirect("/projects/view/%d" % projId)


@app.route("/projects/<int:projId>/addModule", methods=["GET", "POST"])
@checkDBConnection
@checkSystemsLoaded
def createProjectModule(projId):
    moduleForm = CreateProjectModuleForm()

    if request.method == "POST" and moduleForm.is_submitted:
        msg, category = insertModules(projId, request)
        flash(msg, category)
        return redirect("/projects/view/%d" % projId)

    project = getProject(projId)
    locationForm = getLocationFormWithChoices(projId, AddProjectLocationForm())
    settingForm = CreateProjectSettingForm()

    return renderTemplate("viewProject.html",
                          title="Project details",
                          project=project,
                          locationForm=locationForm,
                          settingForm=settingForm,
                          moduleForm=moduleForm)


@app.route("/locations/<int:locId>/addProject", methods=["GET", "POST"])
@checkDBConnection
@checkSystemsLoaded
def addLocationProject(locId):
    projects = []

    for project in models.Fuzzjob.query.all():
        if models.LocationFuzzjobs.query.filter_by(Location=locId, Fuzzjob=project.ID).first() is None:
            data = (project.ID, project.name)
            projects.append(data)

    form = AddLocationProjectForm()
    form.project.choices = projects

    if request.method == 'POST':
        location = models.LocationFuzzjobs(Location=locId, Fuzzjob=form.project.data)
        db.session.add(location)
        db.session.commit()
        flash("Added project to location", "success")
        return redirect("/locations/view/%s" % locId)

    return renderTemplate("addLocationProject.html",
                          title="Add new project to location",
                          form=form)


@app.route("/locations/createLocation", methods=["GET", "POST"])
@checkDBConnection
@checkSystemsLoaded
def createLocation():
    form = CreateLocationForm()

    if form.validate_on_submit():
        location = models.Locations(Name=form.name.data)
        db.session.add(location)
        db.session.commit()
        loadSystems()
        flash("Added Location", "success")
        return redirect("/locations")

    return renderTemplate("createLocation.html",
                          title="Add new Location",
                          form=form)


@app.route("/locations/removeLocation/<int:locId>")
def removeLocation(locId):
    location = models.Locations.query.filter_by(ID=locId).first()
    sysLocation = models.SystemsLocation.query.filter_by(Location=location.ID).first()
    location_fuzzjobs = models.LocationFuzzjobs.query.filter_by(Location=location.ID)
    if sysLocation is None:
        db.session.delete(location)
        for location_fuzzjob in location_fuzzjobs:
            db.session.delete(location_fuzzjob)
        db.session.commit()
        flash("Location deleted", "success")
        return redirect("/locations")
    else:
        flash("Location couldn't be deleted, because it's being used", "error")
        return redirect("/locations")


@app.route("/projects/<int:projId>/renameElement", methods=["POST"])
def renameElement(projId):
    if not request.json:
        abort(400)

    message, status = insertOrUpdateNiceName(projId, request.json["myGUID"], request.json["myLocalID"], request.json["newName"],
                                             request.json["command"], request.json["elemType"])

    return json.dumps({"message": message, "status": status, "command": request.json["command"]})


@app.route("/projects/<int:projId>/updateInfo", methods=["POST"])
def updateInfo(projId):
    if not request.json:
        abort(400)

    infoType = request.json.get("infoType")
    msg, info, status = updateInfoHandler(projId, infoType)

    return json.dumps({"message": msg, "status": status, "info": info})


@app.route("/projects/<int:projId>/download")
def downloadTestcaseSet(projId):
    project = models.Fuzzjob.query.filter_by(ID=projId).first()
    nice_name = project.name + ": Testcase Set"

    return createZipArchive(projId, nice_name, "testcaseSet")


def createZipArchive(projId, nice_name, type):
    if lock.check_file():
        tmp_path = os.getcwd() + "/temp"
        proc = subprocess.Popen([
            "/usr/bin/python3.6", "./app/utils/downloader.py",
            lock.file_path,
            str(projId),
            nice_name,
            "download",
            type,
            tmp_path
        ])
        pid_ = proc.pid
        time_to_wait = 20
        time_counter = 0
        while not os.path.exists(lock.file_path):
            time.sleep(0.25)
            time_counter += 1
            if time_counter > time_to_wait:
                break
        lock.change_file_entry("PID", str(pid_))

    return redirect("/statusDownload")


@app.route('/statusDownload')
def statusDownload():
    if not lock.check_file():
        file_content = lock.read_file()

        if file_content['TYPE'] == "archive":
            return renderTemplate("progressArchiveFuzzjob.html",
                                  nice_name=file_content['NICE_NAME'])
        else:
            return renderTemplate("progressDownload.html",
                                  max_val=file_content['MAX_VAL'],
                                  nice_name=file_content['NICE_NAME'],
                                  download=file_content['DOWNLOAD_PATH'])
    else:
        return redirect(url_for("projects"))


@app.route('/progressDownload')
def progressDownload():
    if not lock.check_file():
        file_content = lock.read_file()
        download_path = file_content['DOWNLOAD_PATH']

        if len(download_path) > 1:
            path = download_path
        else:
            path = getDownloadPath() + "testcase_set.zip"

        if os.path.isfile(path) and file_content['STATUS'] == "1":
            subprocess.Popen(["kill", file_content['PID']])
            lock.delete_file()
            return str(-1)
        elif file_content['STATUS'] == "2":
            error_message = file_content['MESSAGE']
            subprocess.Popen(["kill", file_content['PID']])
            lock.delete_file()
            return error_message
        else:
            return file_content['PROGRESS']

    else:
        return "NONE"


@app.route('/stopProcess')
def stopProcess():
    if not lock.check_file():
        lock.change_file_entry("END", "1")
        subprocess.Popen(["kill", lock.read_file_entry('PID')])
        lock.delete_file()
        return redirect(url_for("projects"))
    else:
        return redirect(url_for("statusDownload"))


@app.route('/download/<string:archive>')
def downloadArchive(archive):
    return send_file(getDownloadPath() + archive, as_attachment=True, cache_timeout=0)


@app.route("/projects/<int:projId>/population/<int:page>")
@checkDBConnection
@checkSystemsLoaded
def viewPopulation(projId, page):
    count = getRowCount(projId, getITCountOfTypeQuery(TESTCASE_TYPES["population"]))
    if count % 1000 == 0:
        count = count // 1000
    else:
        count = (count // 1000) + 1
    if page > count:
        page = 1
    statement = getITQueryOfTypeNoRaw(TESTCASE_TYPES["population"])[:-1] + " LIMIT " + str((page - 1) * 1000) + ", 1000;"
    data = getGeneralInformationData(projId, statement)
    data.name = "Population"
    data.redirect = "population"
    data.downloadName = "downloadPopulation"

    return renderTemplate("viewTCsTempl.html",
                          title="View Population",
                          data=data,
                          show_rating=True,
                          actual_page=page,
                          page_count=count,
                          base_link="/projects/" + str(projId) + "/population/")


@app.route("/projects/<int:projId>/population/download")
def downloadPopulation(projId):
    project = models.Fuzzjob.query.filter_by(ID=projId).first()
    type = "population"
    nice_name = project.name + ": Population"

    return createZipArchive(projId, nice_name, type)


@app.route("/projects/<int:projId>/accessVioTotal/<int:page>")
@checkDBConnection
@checkSystemsLoaded
def viewAccessVioTotal(projId, page):
    count = getRowCount(projId, getITCountOfTypeQuery(TESTCASE_TYPES["accessViolations"]))
    if count % 1000 == 0:
        count = count // 1000
    else:
        count = (count // 1000) + 1
    if page > count:
        page = 1
    statement = getITQueryOfTypeNoRaw(TESTCASE_TYPES["accessViolations"])[:-1] + " LIMIT " + str((page - 1) * 1000) + ", 1000;"
    data = getGeneralInformationData(projId, statement)
    data.name = "Access Violations"
    data.redirect = "accessVioTotal"
    data.downloadName = "downloadAccessVioTotal"

    return renderTemplate("viewTCsTempl.html",
                          title="View Access Violations",
                          data=data,
                          actual_page=page,
                          page_count=count,
                          base_link="/projects/" + str(projId) + "/accessVioTotal/")


@app.route("/projects/<int:projId>/accessVioTotal/download")
def downloadAccessVioTotal(projId):
    project = models.Fuzzjob.query.filter_by(ID=projId).first()
    type = "accessViolations"
    nice_name = project.name + ": Access Violations (Total)"

    return createZipArchive(projId, nice_name, type)


@app.route("/projects/<int:projId>/accessVioUnique")
@checkDBConnection
@checkSystemsLoaded
def viewAccessVioUnique(projId):
    data = getGeneralInformationData(projId, UNIQUE_ACCESS_VIOLATION_NO_RAW)
    data.name = "Unique Access Violations"
    data.redirect = "accessVioUnique"
    data.downloadName = "downloadAccessVioUnique"

    return renderTemplate("viewTCsTempl.html",
                          title="View Unique Access Violations",
                          data=data,
                          actual_page=0,
                          page_count=0,
                          show_occurences=True)


@app.route("/projects/<int:projId>/accessVioUnique/download")
def downloadAccessVioUnique(projId):
    project = models.Fuzzjob.query.filter_by(ID=projId).first()
    type = "accessViolationsUnique"
    nice_name = project.name + ": Access Violations (Unique)"

    return createZipArchive(projId, nice_name, type)


@app.route("/projects/<int:projId>/totalCrashes/<int:page>")
@checkDBConnection
@checkSystemsLoaded
def viewTotalCrashes(projId, page):
    count = getRowCount(projId, getITCountOfTypeQuery(TESTCASE_TYPES["crashes"]))
    if count % 1000 == 0:
        count = count // 1000
    else:
        count = (count // 1000) + 1
    if page > count:
        page = 1
    statement = getITQueryOfTypeNoRaw(TESTCASE_TYPES["crashes"])[:-1] + " LIMIT " + str((page - 1) * 1000) + ", 1000;"
    data = getGeneralInformationData(projId, statement)
    data.name = "Crashes"
    data.redirect = "totalCrashes"
    data.downloadName = "downloadTotalCrashes"

    return renderTemplate("viewTCsTempl.html",
                          title="View Total Crashes",
                          data=data,
                          actual_page = page,
                          page_count = count,
                          base_link = "/projects/" + str(projId) + "/totalCrashes/")


@app.route("/projects/<int:projId>/totalCrashes/download")
def downloadTotalCrashes(projId):
    project = models.Fuzzjob.query.filter_by(ID=projId).first()
    type = "crashes"
    nice_name = project.name + ": Crashes (Total)"

    return createZipArchive(projId, nice_name, type)


@app.route("/projects/<int:projId>/uniqueCrashes")
@checkDBConnection
@checkSystemsLoaded
def viewUniqueCrashes(projId):
    data = getGeneralInformationData(projId, UNIQUE_CRASHES_NO_RAW)
    data.name = "Unique Crashes"
    data.redirect = "uniqueCrashes"
    data.downloadName = "downloadUniqueCrashes"

    return renderTemplate("viewTCsTempl.html",
                          title="View Unique Crashes",
                          data=data,
                          actual_page=0,
                          page_count=0,
                          show_occurences=True)


@app.route("/projects/<int:projId>/uniqueCrashes/download")
def downloadUniqueCrashes(projId):
    project = models.Fuzzjob.query.filter_by(ID=projId).first()
    type = "crashesUnique"
    nice_name = project.name + ": Crashes (Unique)"

    return createZipArchive(projId, nice_name, type)


@app.route("/projects/<int:projId>/hangs/<int:page>")
@checkDBConnection
@checkSystemsLoaded
def viewHangs(projId, page):
    count = getRowCount(projId, getITCountOfTypeQuery(TESTCASE_TYPES["hangs"]))
    if count % 1000 == 0:
        count = count // 1000
    else:
        count = (count // 1000) + 1
    if page > count:
        page = 1
    statement = getITQueryOfTypeNoRaw(TESTCASE_TYPES["hangs"])[:-1] + " LIMIT " + str((page - 1) * 1000) + ", 1000;"
    data = getGeneralInformationData(projId, statement)
    data.name = "Hangs"
    data.redirect = "hangs"
    data.downloadName = "downloadHangs"

    return renderTemplate("viewTCsTempl.html",
                          title="View Hangs",
                          data=data,
                          actual_page = page,
                          page_count = count,
                          base_link = "/projects/" + str(projId) + "/hangs/")


@app.route("/projects/<int:projId>/hangs/download")
def downloadHangs(projId):
    project = models.Fuzzjob.query.filter_by(ID=projId).first()
    type = "hangs"
    nice_name = project.name + ": Hangs"

    return createZipArchive(projId, nice_name, type)


@app.route("/projects/<int:projId>/noResponse/<int:page>")
@checkDBConnection
@checkSystemsLoaded
def viewNoResponses(projId, page):
    count = getRowCount(projId, getITCountOfTypeQuery(TESTCASE_TYPES["noResponses"]))
    if count % 1000 == 0:
        count = count // 1000
    else:
        count = (count // 1000) + 1
    if page > count:
        page = 1
    statement = getITQueryOfTypeNoRaw(TESTCASE_TYPES["noResponses"])[:-1] + " LIMIT " + str((page - 1) * 1000) + ", 1000;"
    data = getGeneralInformationData(projId, statement)
    data.name = "No Responses"
    data.redirect = "noResponse"
    data.downloadName = "downloadNoResponses"

    return renderTemplate("viewTCsTempl.html",
                          title="View No Responses",
                          data=data,
                          actual_page=page,
                          page_count=count,
                          base_link="/projects/" + str(projId) + "/noResponse/")


@app.route("/projects/<int:projId>/noResponse/download")
def downloadNoResponses(projId):
    project = models.Fuzzjob.query.filter_by(ID=projId).first()
    type = "noResponses"
    nice_name = project.name + ": No Response"

    return createZipArchive(projId, nice_name, type)


@app.route("/projects/<int:projId>/violations")
@checkDBConnection
@checkSystemsLoaded
def viewViolations(projId):
    violationsAndCrashes = getViolationsAndCrashes(projId)

    return renderTemplate("viewAccessViolations.html",
                          title="View AccessViolations",
                          violationsAndCrashes=violationsAndCrashes)


@app.route("/projects/<int:projId>/crashes/download", methods=["GET", "POST"])
def downloadCrashes(projId):
    footprint = request.form.get("footprint", None)
    testcaseType = request.form.get("testcaseType", None)
    
    if footprint is not None and testcaseType is not None:        
        project = models.Fuzzjob.query.filter_by(ID=projId).first()
        
        if project:            
            type = "crashesFootprintTestcase " + str(footprint) + " " + str(testcaseType)
            nice_name = project.name + ": Crashes (Footprint)"
            return createZipArchive(projId, nice_name, type)
         
        flash("Project does not exist!", "error")
        return redirect(url_for('viewViolations', projId=projId))
    
    flash("Footprint or testcaseType does not exist!", "error")
    return redirect(url_for('viewViolations', projId=projId))


@app.route("/projects/<int:projId>/crashesorvio/download/<string:footprint>")
def downloadCrashesOrViolations(projId, footprint):
    project = models.Fuzzjob.query.filter_by(ID=projId).first()
    type = "crashesOrViolationFootprint " + str(footprint)
    nice_name = project.name + ": Crashes and Violations (Footprint)"

    return createZipArchive(projId, nice_name, type)


@app.route("/projects/<int:projId>/getTestcase/<string:testcaseId>")
def getTestcase(projId, testcaseId):
    byteIO, filename = createByteIOTestcase(projId, testcaseId)

    return send_file(byteIO,
                     attachment_filename=filename,
                     as_attachment=True)


@app.route("/projects/<int:projId>/getSmallestVioOrCrashTestcase", methods=["GET", "POST"])
def getSmallestVioOrCrashTestcase(projId):
    
    footprint = request.form.get("footprint", None)
    
    if footprint is None:
        flash("Footprint does not exist!", "error")
        return redirect(url_for('viewViolations', projId=projId))
    
    byteIO = createByteIOForSmallestVioOrCrash(projId, footprint)
    
    return send_file(byteIO,
                    attachment_filename=footprint.replace(":", "_"),
                    as_attachment=True)        


@app.route("/projects/<int:projId>/managedInstances")
@checkDBConnection
@checkSystemsLoaded
def viewManagedInstances(projId):
    managedInstances, summarySection, localManagers = getManagedInstancesAndSummary(projId)

    return renderTemplate("viewManagedInstances.html",
                          title="View ManagedInstances",
                          managedInstances=managedInstances,
                          summarySection=summarySection,
                          localManagers=localManagers
                          )


@app.route("/projects/<int:projId>/managedInstanceLogs", methods=["POST"])
def getLogsOfManagedInstance(projId):
    sdguid = ""
    rowCount = 0    
    limit = 10
    offset = 0   
    pageCount = 1 
    
    if "sdguid" in request.json:
        sdguid = request.json["sdguid"]
    else:
        print("No ServiceDescriptorGuid in request.json")
    
    if "offset" in request.json:
        offset = request.json["offset"]

    if "init" in request.json:
        rowCount = getRowCount(projId, GET_COUNT_OF_MANAGED_INSTANCE_LOGS, {"sdguid": sdguid}) 
        pageCount = (rowCount // 10) + 1 if rowCount % 10 != 0 else rowCount // 10

    result = getResultOfStatement(projId, GET_MANAGED_INSTANCE_LOGS, {"sdguid": sdguid, "limit": limit, "offset": offset})
    miLogs = [ row["LogMessage"] for row in result ]  

    return json.dumps({"status": "OK", "pageCount": pageCount, "miLogs": miLogs})


@app.route("/hexdump/<int:projId>/<int:testcaseId>/<int:offset>/<int:comp>", methods=["GET"])
def getTestcaseHexdump(projId, testcaseId, offset, comp):
    diffFileName = "diffTemp"
    deleteOldHexdumpFiles(diffFileName)

    testcaseDumpFileName = "testcaseHexTemp"
    deleteOldHexdumpFiles(testcaseDumpFileName)

    testcaseHexdumpFile = "/" + testcaseDumpFileName + "." + str(projId) + "." + str(testcaseId)
    parentTestcaseId = ""
    parentTestcaseGuid = ""
    pageCount = 0

    if comp is 1 or not os.path.isfile(testcaseHexdumpFile):
        parentTestcaseId, parentTestcaseGuid, pageCount = loadHexInFile(projId, testcaseId, testcaseHexdumpFile)
    else:
        pageCount = int(os.path.getsize(testcaseHexdumpFile) / 960) + (os.path.getsize(testcaseHexdumpFile) % 960 > 0)
        parentTestcaseId, parentTestcaseGuid = getTestcaseParentInfo(projId, testcaseId)

    if comp is 1 and parentTestcaseId is not "" and parentTestcaseGuid is not "":
        resultParentIdRes = getResultOfStatement(projId, GET_TESTCASE_PARENT_ID,
                                              {"parentID": parentTestcaseId, "parentGUID": parentTestcaseGuid})
        if resultParentIdRes is not None:
            rows = resultParentIdRes.fetchall()
            resultParentId = rows[0][0]

            testcaseParentHexdumpFile = "/" + testcaseDumpFileName + "." + str(projId) + "." + str(resultParentId)
            if not os.path.exists(testcaseParentHexdumpFile):
                loadHexInFile(projId, resultParentId, testcaseParentHexdumpFile)

            diffResultFile = "/" + diffFileName + "." + str(projId) + "." + str(testcaseId) + "." + str(parentTestcaseId)
            if not os.path.exists(diffResultFile):
                writeHexDiffFile(testcaseParentHexdumpFile, testcaseHexdumpFile, diffResultFile)

            diff = getHexDiff(diffResultFile)
            newDiff, insSpaceT, insSpacePT = getScaledDiff(diff)
            pageCount = getPageCountList(diff, newDiff, insSpaceT)

            if offset not in pageCount:
                for pageNum, page in enumerate(pageCount):
                    if len(pageCount) > pageNum + 1:
                        if page < offset < pageCount[pageNum + 1]:
                            offset = page
                            break

            offsetP, testcaseHexList, testcaseParentHexList = getTestcaseHexFromFileDiff(diff, newDiff, insSpaceT,
                                                                                         insSpacePT, pageCount,
                                                                                         testcaseHexdumpFile,
                                                                                         testcaseParentHexdumpFile,
                                                                                         offset)

            if len(testcaseHexList) >= len(testcaseParentHexList):
                targetLen = (int(len(testcaseHexList) / 8) + (len(testcaseHexList) % 8 > 0)) * 8
                for _ in range(len(testcaseParentHexList), targetLen):
                    testcaseParentHexList.append("  ")
            else:
                targetLen = (int(len(testcaseParentHexList) / 8) + (len(testcaseParentHexList) % 8 > 0)) * 8
                for _ in range(len(testcaseHexList), targetLen):
                    testcaseHexList.append("  ")

            textOutputT, decodedDataT = getTextByHex(testcaseHexList, 8)
            textOutputP, decodedDataP = getTextByHex(testcaseParentHexList, 8)

            retDiff = []
            retNewDiff = []
            insertionsPT = 0
            insertionsT = 0

            for num, row in enumerate(newDiff):
                x = set(range(int(row[1]), int(row[2])))
                y = set(range(int(row[3]), int(row[4])))
                z = set(range(offset, offset + 960))

                if len(list(set.intersection(x, z))) > 0 or len(list(set.intersection(y, z))) > 0:
                    retDiff.append(diff[num])
                    newDiffRow = [newDiff[num][0]]
                    if int(newDiff[num][1]) < offset:
                        newDiffRow.append("0")
                    else:
                        newDiffRow.append(str(int(newDiff[num][1]) - offset - insertionsPT))
                    newDiffRow.append(str(int(newDiff[num][2]) - offset - insertionsPT))
                    if int(newDiff[num][3]) < offset:
                        newDiffRow.append("0")
                    else:
                        newDiffRow.append(str(int(newDiff[num][1]) - offset - insertionsT))
                    newDiffRow.append(str(int(newDiff[num][2]) - offset - insertionsT))
                    retNewDiff.append(newDiffRow)

            return json.dumps({"status": "OK", "pageCount": pageCount, "offsetP": offsetP, "hexT": textOutputT, "decodedT": decodedDataT,
                               "hexP": textOutputP, "decodedP": decodedDataP, "diff": retNewDiff, "realDiff": retDiff})
        else:
            return json.dumps({"status": "ERROR"})
    else:
        if offset % 960 != 0:
            offset = int(offset / 960) * 960
        testcaseHexList = getTestcaseHexFromFile(testcaseHexdumpFile, offset, 960)
        textOutputF, decodedDataF = getTextByHex(testcaseHexList, 16)

        if parentTestcaseGuid != "initial":
            return json.dumps({"status": "OK", "pageCount": pageCount, "hexT": textOutputF, "decodedT": decodedDataF, "parent": parentTestcaseGuid})
        else:
            return json.dumps({"status": "OK", "pageCount": pageCount, "hexT": textOutputF, "decodedT": decodedDataF, "parent": ""})


@app.route("/projects/<int:projId>/configSystemInstances")
@checkDBConnection
@checkSystemsLoaded
def viewConfigSystemInstances(projId):
    # initialize forms
    systemInstanceConfigForm = SystemInstanceConfigForm()
    fuzzjob = db.session.query(models.Fuzzjob).filter_by(ID=projId).first()
    sysList = []
    systems = db.session.query(models.Systems).all()    

    for sys in systems:
        sysList.append(sys)

    lmCount = 0
    sysInstanceList = []
    lemmingInstanceList = []
    dbconfiguredFuzzjobInstances = db.session.query(models.SystemFuzzjobInstances.System,
                                                    models.SystemFuzzjobInstances.Fuzzjob,
                                                    models.SystemFuzzjobInstances.AgentType,
                                                    models.SystemFuzzjobInstances.InstanceCount,
                                                    models.SystemFuzzjobInstances.Architecture).filter_by(Fuzzjob=projId).all()
        
    settingArch = getSettingArchitecture(projId)
    
    for system in sysList:
        s = {'name': system.Name, 'tg': 0, 'tr': 0, 'te': 0, 'tgarch': "", 'trarch': "", 'tearch': "", 'settingArch': settingArch}
        if 'lemming' not in system.Name:
            sysInstanceList.append(s)
        else:
            lemmingInstanceList.append(s)
        for conf in dbconfiguredFuzzjobInstances:
            if system.ID == conf.System:
                if conf.AgentType == 0:
                    s['tg'] = conf.InstanceCount
                    s['tgarch'] = conf.Architecture
                if conf.AgentType == 1:
                    s['tr'] = conf.InstanceCount
                    s['trarch'] = conf.Architecture
                if conf.AgentType == 2:
                    s['te'] = conf.InstanceCount
                    s['tearch'] = conf.Architecture
                        
    for conf in dbconfiguredFuzzjobInstances:
        if conf.AgentType == 4:
            lmCount = lmCount + conf.InstanceCount
            
    return renderTemplate("viewConfigSystemInstances.html",
                          title="View Instance Configuration",
                          systemInstanceConfigForm=systemInstanceConfigForm,
                          configSystems=sysInstanceList,
                          lemmingSystems=lemmingInstanceList,
                          lmCount=lmCount,
                          fuzzjob=fuzzjob
                          )


@app.route("/projects/<int:projId>/killInstance/<string:guid>")
def killManagedInstance(projId, guid):
    addCommand(projId, guid)

    return redirect("/projects/%d/managedInstances" % projId)


@app.route("/projects/<int:projId>/killInstance/localManager/<string:guid>/<string:hostAndPort>")
def killLocalManager(projId, guid, hostAndPort):
    addCommand(projId, guid, hostAndPort)

    return redirect("/projects/%d/managedInstances" % projId)


@app.route("/projects/<int:projId>/killInstanceType/<int:myType>")
def killInstanceType(projId, myType):
    addCommandToKillInstanceType(projId, myType)

    return redirect("/projects/%d/managedInstances" % projId)


@app.route("/projects/<int:projId>/addLocation", methods=["GET", "POST"])
@checkDBConnection
@checkSystemsLoaded
def addProjectLocation(projId):
    locationForm = getLocationFormWithChoices(projId, AddProjectLocationForm())

    if request.method == 'POST' and locationForm.is_submitted:
        locations = request.form.getlist('location')
        for loc in locations:
            location = models.Locations.query.filter_by(Name=loc).first()
            location_fuzzjob = models.LocationFuzzjobs(Location=location.ID, Fuzzjob=projId)
            db.session.add(location_fuzzjob)
            db.session.commit()
        flash("Added location(s)", "success")
        return redirect("/projects/view/%d" % projId)

    project = getProject(projId)
    moduleForm = CreateProjectModuleForm()
    settingForm = CreateProjectSettingForm()

    return renderTemplate("viewProject.html",
                          title="Project details",
                          project=project,
                          locationForm=locationForm,
                          moduleForm=moduleForm,
                          settingForm=settingForm
                          )


@app.route("/projects/<int:projId>/addTestcase", methods=["POST"])
def addTestcase(projId):
    if 'addTestcase' in request.files:
        files = request.files.getlist('addTestcase')
        if not files or not any(f for f in files):
            msg, category = "Please select a file or multiple files", "error"
        else:
            msg, category = insertTestcases(projId, files)

        flash(msg, category)
        return redirect("/projects/{}/population/1".format(projId))
    else:
        flash("Please select a file or multiple files", "error")
        return redirect("/projects/{}/population/1".format(projId))


@app.route("/locations/assignWorker/<string:workerSdGUID>/<int:fuzzjobId>/<int:locationId>", methods=["GET", "POST"])
def assignWorker(workerSdGUID, fuzzjobId, locationId):             
    if fuzzjobId == 0:
        flash("Cannot assign to 'Unassigned'", "error")
        return redirect(url_for('viewLocation', locId=locationId))
        
    try:        
        worker = models.Workers.query.filter_by(Servicedescriptorguid=workerSdGUID).first()
        if worker:
            worker.Fuzzjob = fuzzjobId
            db.session.commit()
            message, category = "Successfully assigned worker", "success"
        else:
            message, category = "Worker with Servicedescriptorguid {} does not exist".format(workerSdGUID), "error"
    except Exception as e:
        print(e)
        message, category = "Failed to assign worker! " + str(e), "error"

    flash(message, category)
    return redirect(url_for('viewLocation', locId=locationId))


@app.route("/projects/<int:projId>/changeSetting/<int:settingId>", methods=["POST"])
def changeProjectSetting(projId, settingId):
    if not request.json:
        abort(400)

    message, status = updateSettings(projId, settingId, request.json["SettingValue"])

    return json.dumps({"message": message, "status": status})


@app.route("/projects/<int:projId>/delLocation/<string:locName>", methods=["GET"])
def removeProjectLocation(projId, locName):
    location = models.Locations.query.filter_by(Name=locName).first()
    location_fuzzjob = models.LocationFuzzjobs.query.filter_by(Fuzzjob=projId, Location=location.ID).first()
    db.session.delete(location_fuzzjob)
    db.session.commit()
    flash("Location deleted", "success")

    return redirect("/projects/view/%d" % projId)


@app.route("/projects/<int:projId>/delTestcase/<string:testcaseId>/<string:tcType>", methods=["GET"])
def removeProjectTestcase(projId, testcaseId, tcType):
    (guid, localId) = testcaseId.split(":")
    msg, category = deleteElement(projId, "Testcase", DELETE_TC_WTIH_LOCALID, {"guid": guid, "localId": localId})
    flash(msg, category)
    print(tcType)
    if "unique" in tcType or "Unique" in tcType:        
        return redirect("/projects/{}/{}".format(projId, tcType))
    else:
        return redirect("/projects/{}/{}/1".format(projId, tcType))


@app.route("/projects/<int:projId>/delModule/<int:moduleId>", methods=["GET"])
def removeProjectModule(projId, moduleId):
    msg, category = deleteElement(projId, "Module", DELETE_MODULE_BY_ID, {"ID": moduleId})
    flash(msg, category)

    return redirect("/projects/view/%d" % projId)


@app.route("/projects/<int:projId>/delSetting/<int:settingId>", methods=["GET"])
def removeProjectSetting(projId, settingId):
    msg, category = deleteElement(projId, "Setting", DELETE_SETTING_BY_ID, {"ID": settingId})
    flash(msg, category)

    return redirect("/projects/view/%d" % projId)


@app.route("/projects/createProject", methods=["GET", "POST"])
@checkDBConnection
@checkSystemsLoaded
def createProject():
    form = CreateProjectForm()
    # msg, category = "", "" 

    if request.method == 'POST' and form.is_submitted:
        result = []
        result.extend(insertFormInputForProject(form, request))

        flash(result[0], result[1])
        if result[1] == "error":
            return redirect(url_for("projects"))
        else:
            return redirect("/projects/view/%d" % result[2])
    else:
        form = CreateProjectForm()
        form.location.choices = [l.Name for l in db.session.query(models.Locations.Name)]
        sortedRunnerTypeOptions = sorted(JSON_CONFIG["RunnerTypeOptions"])
        return renderTemplate("createProject.html",
                              title="Create custom project",
                              subTypesMutator=JSON_CONFIG["GeneratorTypes"],
                              subTypesEvaluator=JSON_CONFIG["EvaluatorTypes"],
                              defaultSubTypesOfGenerator=createDefaultSubtypes(JSON_CONFIG["GeneratorTypes"]),
                              defaultSubTypesOfEvaluator=createDefaultSubtypes(JSON_CONFIG["EvaluatorTypes"]),
                              defaultSubTypesOfGeneratorCount=createDefaultSubtypesList(JSON_CONFIG["GeneratorTypes"]),
                              defaultSubTypesOfEvaluatorCount=createDefaultSubtypesList(JSON_CONFIG["EvaluatorTypes"]),
                              RunnerTypeOptions=sortedRunnerTypeOptions,
                              form=form)


@app.route("/projects/createCustomProject", methods=["GET", "POST"])
@checkDBConnection
@checkSystemsLoaded
def createCustomProject():
    form = CreateCustomProjectForm()

    if form.validate_on_submit():
        project = models.Fuzzjob(name=form.name.data, DBHost=form.DBHost.data, DBUser=form.DBUser.data,
                                 DBPass=form.DBPass.data, DBName=form.DBName.data)
        db.session.add(project)
        db.session.commit()
        flash("Created new project", "success")
        return redirect(url_for("projects"))

    return renderTemplate("createCustomProject.html",
                          title="Create custom project",
                          form=form)


@app.route("/projects/diagrams/<int:projId>", methods=["GET", "POST"])
def viewProjectGraph(projId):
    project = models.Fuzzjob.query.filter_by(ID=projId).first()
    fuzzjobname = project.name

    return renderTemplate("viewProjectGraph.html",
                          title="Create custom project",
                          fuzzjobname=fuzzjobname)


@app.route("/projects/<int:projId>/testcaseGraph", methods=["GET"])
def viewTestcaseGraph(projId):
    graphdata = getGraphData(projId)

    return json.dumps(graphdata)


@app.route("/locations/view/<int:locId>")
@checkDBConnection
@checkSystemsLoaded
def viewLocation(locId):
    location = getLocation(locId)

    return renderTemplate("viewLocation.html",
                          title="Location details",
                          locationId=locId,
                          location=location)


@app.route("/projects/view/<int:projId>")
@checkDBConnection
@checkSystemsLoaded
def viewProject(projId):
    project = getProject(projId)
    locationForm = getLocationFormWithChoices(projId, AddProjectLocationForm())
    settingForm = CreateProjectSettingForm()
    moduleForm = CreateProjectModuleForm()

    return renderTemplate("viewProject.html",
                          title="Project details",
                          project=project,
                          locationForm=locationForm,
                          settingForm=settingForm,
                          moduleForm=moduleForm)


@app.route("/locations")
@checkDBConnection
@checkSystemsLoaded
def locations():
    locations = models.Locations.query.all()

    return renderTemplate("locations.html",
                          title="Locations",
                          locations=locations)


@app.route("/commands")
@checkDBConnection
@checkSystemsLoaded
def commands():
    commands = models.CommandQueue.query.all()

    return renderTemplate("viewCommandQueue.html",
                          title="Commands",
                          commands=commands)


@app.route("/logs")
@checkDBConnection
@checkSystemsLoaded
def logs():
    result = getResultOfStatementForGlobalManager(GET_LOCALMANAGER_LOGS)
    logs = [ row for row in result ]
    pages = list(chunks(logs, 10))
    return renderTemplate("viewLogs.html",
                            pages=pages)


@app.route("/systems")
@checkDBConnection
@checkSystemsLoaded
def systems():
    ansibleConnectionWorks = True
    ftpConnectionWorks = True  

    groups = []
    addNewSystemToPolemarchForm = AddNewSystemForm()
    projIds = models.Fuzzjob.query.all()

    managedInstancesAll = []
    actualAgentStarterMode = models.GmOptions.query.filter_by(setting="agentstartermode").first().value.upper()
    changeAgentStarterModeForm = ChangeAgentStarterModeForm()
    changePXEForm = ChangePXEForm()
    bootsystemdir = models.GmOptions.query.filter_by(setting="bootsystemdir").first().value

    try:
        changePXEForm.pxesystem.choices = FTP_CONNECTOR.getListOfFilesOnFTPServer("tftp-roots/")              
    except Exception as e:
        print("Error", e)
        ftpConnectionWorks = False
        changePXEForm.pxesystem.choices = []

    for project in projIds:
        managedInstances, summarySection, localmanagers = getManagedInstancesAndSummary(project.ID)
        managedInstancesAll.append(managedInstances)

    # Not assigned LocalManagers
    workers = db.session.query(models.Workers).all()
    newWorkers = []

    for worker in workers:
        yetInList = False
        for mi in managedInstancesAll:
            for i in mi["instances"]:
                if i["ServiceDescriptorGUID"].strip() == worker.Servicedescriptorguid.strip():
                    yetInList = True
        if not yetInList:
            newWorkers.append(worker)

    try:
        groups = ANSIBLE_REST_CONNECTOR.getHostAliveState()
        instancesum = db.session.query(models.SystemFuzzjobInstances,
                                       func.sum(models.SystemFuzzjobInstances.InstanceCount).label('total'),
                                       models.Systems).group_by(models.SystemFuzzjobInstances.System).join(
            models.Systems).filter(
            models.SystemFuzzjobInstances.System == models.Systems.ID).all()
        instanceLM = db.session.query(models.SystemFuzzjobInstances,
                                      func.sum(models.SystemFuzzjobInstances.InstanceCount).label('lm'),
                                      models.Systems).group_by(models.SystemFuzzjobInstances.System).join(
            models.Systems).filter(and_(models.SystemFuzzjobInstances.System == models.Systems.ID),
                                   (models.SystemFuzzjobInstances.AgentType == 4)).all()

        localMan = db.session.query(models.Localmanagers).all()
        systemLocations = db.session.query(models.SystemsLocation, models.Systems, models.Locations).join(
            models.Systems).filter(models.SystemsLocation.System == models.Systems.ID).join(models.Locations).filter(
            models.SystemsLocation.Location == models.Locations.ID).all()

        for system in systemLocations:
            for group in groups:
                for h in group.hosts:
                    h.confLM = 0
                    h.InstanceCount = 0
                    if h.Name == system[1].Name:
                        h.Location = system[2].Name
                    for s in instancesum:
                        if h.Name.strip() == s[2].Name.strip():
                            h.InstanceCount = s[1]
                            for lm in instanceLM:
                                if s[2].Name.strip() == lm[2].Name.strip() and lm[1] > 0:
                                    h.confLM = lm[1]

        for group in groups:
            for h in group.hosts:
                h.lms = 0
                ManagedInstancesThisSystemCount = 0
                for s in instancesum:
                    if h.Name.strip() == s[2].Name.strip():
                        h.InstanceCount = s[1]
                for mi in managedInstancesAll:
                    for i in mi["instances"]:
                        if i["ServiceDescriptorHostAndPort"].split(".")[0].strip() == h.Name.strip():
                            ManagedInstancesThisSystemCount += 1
                for lm in localMan:
                    if lm.Servicedescriptorhostandport.split(".")[0].strip() == h.Name.strip():
                        h.lms = h.lms + 1
                h.InstanceRun = ManagedInstancesThisSystemCount + h.lms
                for w in newWorkers:
                    if w.ServiceDescriptorhostandport.split(".")[0].strip() == h.Name.strip():
                        h.InstanceRun = h.InstanceRun + 1
                        if w.Agenttype == 4:
                            h.lms = h.lms + 1
                if not hasattr(h, 'confLM'):
                    h.confLM = 0
                print(h.Name)
                if hasattr(h, 'Status'):
                    if h.Status == "OK":
                        h.IP = str(socket.gethostbyname(h.Name))
                    else:
                        h.IP = "Offline"
                else:
                    h.IP = "---"

        locations = models.Locations.query.all()        
    except Exception as e:
        print(e)
        ansibleConnectionWorks = False
        locations = []
        try:
            ANSIBLE_REST_CONNECTOR.execHostAlive()
        except Exception as e:
            print(e)        

    return renderTemplate("systems.html",
                          title="Systems",
                          systems=groups,
                          locations=locations,
                          addNewSystemToPolemarchForm=addNewSystemToPolemarchForm,
                          ftpConnectionWorks=ftpConnectionWorks,
                          ansibleConnectionWorks=ansibleConnectionWorks,
                          changePXEForm=changePXEForm,
                          bootsystemdir=bootsystemdir,
                          agentStarterMode=actualAgentStarterMode,
                          changeAgentStarterModeForm=changeAgentStarterModeForm)


@app.route("/systems/changePXE", methods=["POST"])
def changePXEBootSystemPolemarch():
    try:
        newPXESystem = str(request.form.getlist('pxesystem')[0])
        arguments = {}
        arguments['OSRootDirNameArgument'] = newPXESystem
        pxeResponse = ANSIBLE_REST_CONNECTOR.executePlaybook('changePXEBoot.yml', 'master', arguments)
        bootsystemdirEntry = models.GmOptions.query.filter_by(setting="bootsystemdir").first()
        bootsystemdirEntry.value = newPXESystem
        db.session.commit()
        successMessage = Markup("Changed PXE-Boot system to <b>" + newPXESystem.upper() + "</b>!")
        flash(successMessage, "success")
        return redirect(url_for('systems'))
    except Exception as e:
        print(e)
        flash("Error changing PXE-Boot system!", "error")

    return redirect(url_for('systems'))


@app.route("/systems/addSystem", methods=["POST"])
def addNewSystemToPolemarch():
    hostname = str(request.form.getlist('hostname')[0])
    hostgroup = str(request.form.getlist('hostgroup')[0])

    if len(models.Systems.query.filter_by(Name=hostname).all()) > 0:
        flash("Error adding new system! System already exists (or check database)!", "error")
        return redirect(url_for('systems'))

    loc = models.Locations.query.first()
    if loc is None:
        flash("Error adding new system! No location exists!", "error")
        return redirect(url_for('systems'))
    else:
        addResult = ANSIBLE_REST_CONNECTOR.addNewSystem(hostname, hostgroup)
        if not addResult:
            flash("Error adding new system! PoleMarch does not accept special characters like _underscore!", "error")
            return redirect(url_for('systems'))

    sys = models.Systems(Name=hostname)
    if sys is None:
        flash("Error adding new system: " + hostname + " does not exist in systems!", "error")
        return redirect(url_for('systems'))

    try:
        db.session.add(sys)
        db.session.commit()
        sysloc = models.SystemsLocation(System=sys.ID, Location=loc.ID)
        db.session.add(sysloc)
        db.session.commit()
        updateSystems()
        flash("Added new system!", "success")
        return redirect(url_for('systems'))
    except AttributeError:
        flash("Error adding new system: system and/or location has no attribute ID!", "error")
        return redirect(url_for('systems'))


@app.route("/systems/changeAgentStarterMode", methods=["POST"])
def changeAgentStarterMode():
    try:
        newMode = str(request.form.getlist('mode')[0])
        agentStarterModeEntry = models.GmOptions.query.filter_by(setting="agentstartermode").first()
        agentStarterModeEntry.value = newMode
        db.session.commit()
        successMessage = Markup("Changed Agent-Starter-Mode to <b>" + newMode.upper() + "</b>!")
        flash(successMessage, "success")
        return redirect(url_for('systems'))
    except Exception as e:
        print(e)
        flash("Error changing agent-starter-mode!", "error")

    return redirect(url_for('systems'))


@app.route("/systems/removeSystem/<string:hostName>", methods=["GET"])
def removeSystemFromPolemarch(hostName):
    success = ANSIBLE_REST_CONNECTOR.removeSystem(hostName) 

    if not success:
        flash("Error removing system! Failed to connect to polemarch", "error")
        return redirect(url_for('systems'))
    
    try:
        sys = models.Systems.query.filter_by(Name=hostName).first()
        sysloc = models.SystemsLocation.query.filter_by(System=sys.ID).first()
        db.session.delete(sysloc)
        db.session.delete(sys)
        db.session.commit()
        flash("Removed " + hostName + "!", "success")
        return redirect(url_for('systems'))
    except Exception as e:
        print(e)
        flash("Failed to remove " + hostName + " !", "error")
        return redirect(url_for('systems'))


@app.route("/systems/<string:hostName>/updateSystemLocation/<int:locationId>", methods=["POST"])
def updateSystemLocation(hostName, locationId):
    print(hostName, locationId)
    if (len(hostName) > 0) and (locationId > 0):
        try:
            sys = models.Systems.query.filter_by(Name=hostName).first()
            if sys is not None:
                systemloc = models.SystemsLocation.query.filter_by(System=sys.ID).first()
                systemloc.Location = locationId
            db.session.commit()
            message = "Setting modified"
            flash("Changed location for " + hostName, "success")
            return json.dumps({"message": message, "status": "OK"})
        except Exception as e:
            message = "Could not modify setting"
            flash("Error changing location for " + hostName, "error")
            return json.dumps({"message": message, "status": "Error"})
    else:
        message = "Value cannot be empty!"
        return json.dumps({"message": message, "status": "Error"})


@app.route("/systems/view/<string:hostname>/<string:group>", methods=["GET"])
@checkDBConnection
@checkSystemsLoaded
def viewSystem(hostname, group):
    if group == "odroids":
        group = "linux"
    # initialize forms
    initialSetupForm = ExecuteInitialSetupForm()
    fluffiDeployForm = ExecuteDeployFluffiForm()
    syncRamdiskForm = SyncToRamdiskForm()
    deployPackageForm = ExecuteDeployInstallPackageForm()
    deployFuzzjobPackageForm = ExecuteDeployFuzzjobInstallPackageForm()
    startFluffiComponentForm = StartFluffiComponentForm()
    systemInstanceConfigForm = SystemInstanceConfigForm()
    
    try:
        availableInstallPackagesFromFTP = FTP_CONNECTOR.getListOfFilesOnFTPServer("SUT/")
    except Exception as e:
        print(e)
        availableInstallPackagesFromFTP = []
        
    deployPackageForm.installPackage.choices = availableInstallPackagesFromFTP
    
    try:
        availableArchitecturesFromFTP = FTP_CONNECTOR.getListOfArchitecturesOnFTPServer("fluffi/" + group + "/", group)
    except Exception as e:
        print(e)
        availableArchitecturesFromFTP = []
    
    fluffiDeployForm.architecture.choices = availableArchitecturesFromFTP
    
    try:
        availableArchitecturesFromFTP = FTP_CONNECTOR.getListOfArchitecturesOnFTPServer("fluffi/" + group + "/", group)
    except Exception as e:
        print(e)
        availableArchitecturesFromFTP = []
    
    startFluffiComponentForm.architecture.choices = availableArchitecturesFromFTP
    system = ANSIBLE_REST_CONNECTOR.getSystemObjectByName(hostname)
    jobList = []
    fuzzjobs = db.session.query(models.SystemsLocation, models.Systems, models.LocationFuzzjobs, models.Fuzzjob).join(
        models.Systems).filter(models.SystemsLocation.System == models.Systems.ID).filter_by(Name=hostname).join(
        models.LocationFuzzjobs).filter(models.SystemsLocation.Location == models.LocationFuzzjobs.Location).join(
        models.Fuzzjob).filter(models.LocationFuzzjobs.Fuzzjob == models.Fuzzjob.ID)

    for job in fuzzjobs:
        jobList.append(job[3].name)

    if not jobList:
        dbJobs = models.Fuzzjob.query.all()
        for job in dbJobs:
            jobList.append(job.name)

    deployFuzzjobPackageForm.fuzzingJob.choices = zip(jobList, jobList)
    managed = {'generators': 0, 'runners': 0, 'evaluators': 0, 'localmanagers': 0}
    lmCount = 0
    instancesum = db.session.query(models.SystemFuzzjobInstances.AgentType, models.Systems,
                                   func.sum(models.SystemFuzzjobInstances.InstanceCount).label('total')).join(
        models.Systems).filter(models.SystemFuzzjobInstances.System == models.Systems.ID).group_by(
        models.SystemFuzzjobInstances.System, models.SystemFuzzjobInstances.AgentType).filter_by(Name=hostname).all()

    for instancetype in instancesum:
        if instancetype[0] == 0:
            managed['generators'] = instancetype[2]
        if instancetype[0] == 1:
            managed['runners'] = instancetype[2]
        if instancetype[0] == 2:
            managed['evaluators'] = instancetype[2]

    configFuzzjobs = listFuzzJobs()
    fuzzjobList = []
    dbconfiguredFuzzjobInstances = db.session.query(models.SystemFuzzjobInstances.System,
                                                    models.SystemFuzzjobInstances.Fuzzjob,
                                                    models.SystemFuzzjobInstances.AgentType,
                                                    models.SystemFuzzjobInstances.InstanceCount,
                                                    models.SystemFuzzjobInstances.Architecture, models.Systems).join(
        models.Systems).filter(models.SystemFuzzjobInstances.System == models.Systems.ID).filter_by(Name=hostname).all()

    for fuzzjob in configFuzzjobs:
        settingArch = getSettingArchitecture(fuzzjob.ID)
        fj = {'name': fuzzjob.name, 'tg': 0, 'tr': 0, 'te': 0, 'tgarch': "", 'trarch': "", 'tearch': "", 'settingArch': settingArch}
        fuzzjobList.append(fj)
        for conf in dbconfiguredFuzzjobInstances:
            if fuzzjob.ID == conf.Fuzzjob:
                if conf.AgentType == 0:
                    fj['tg'] = conf.InstanceCount
                    fj['tgarch'] = conf.Architecture
                if conf.AgentType == 1:
                    fj['tr'] = conf.InstanceCount
                    fj['trarch'] = conf.Architecture
                if conf.AgentType == 2:
                    fj['te'] = conf.InstanceCount
                    fj['tearch'] = conf.Architecture

    for conf in dbconfiguredFuzzjobInstances:
        if conf.AgentType == 4:
            lmCount = lmCount + conf.InstanceCount

    return renderTemplate("viewSystem.html",
                          title="System details",
                          system=system,
                          initialSetupForm=initialSetupForm,
                          fluffiDeployForm=fluffiDeployForm,
                          syncRamdiskForm=syncRamdiskForm,
                          deployPackageForm=deployPackageForm,
                          deployFuzzjobPackageForm=deployFuzzjobPackageForm,
                          startFluffiComponentForm=startFluffiComponentForm,
                          managed=managed,
                          systemInstanceConfigForm=systemInstanceConfigForm,
                          configFuzzjobs=fuzzjobList,
                          lmCount=lmCount)


@app.route("/systems/configureSystemInstances/<string:system>", methods=["POST"])
def configureSystemInstances(system):
    form = SystemInstanceConfigForm()
    msg, category = "", ""

    if request.method == 'POST' and form.is_submitted:
        msg, category = insertFormInputForConfiguredInstances(request, system)
    
    flash(msg, category)
    return redirect(url_for("systems"))


@app.route("/systems/configureFuzzjobInstances/<string:fuzzjob>", methods=["POST"])
def configureFuzzjobInstances(fuzzjob):
    form = SystemInstanceConfigForm()
    projId = db.session.query(models.Fuzzjob).filter_by(name=fuzzjob).first()
    msg, category = "", ""

    if request.method == 'POST' and form.is_submitted:
        msg, category = insertFormInputForConfiguredFuzzjobInstances(request, fuzzjob)
    
    flash(msg, category)
    return redirect(url_for("viewConfigSystemInstances", projId=projId.ID))


@app.route("/systems/removeConfiguredInstances", methods=["POST"])
def removeConfiguredInstances():    
    systemName = request.json.get("systemName", "")
    fuzzjobName = request.json.get("fuzzjobName", "")
    typeFromReq = request.json.get("type", "")
    
    status, msg = deleteConfiguredInstance(systemName, fuzzjobName, typeFromReq)
    return json.dumps({"status": status, "message": msg})        
                

@app.route("/systems/reboot/<string:hostname>", methods=["GET"])
def rebootSystem(hostname):
    historyURL = ANSIBLE_REST_CONNECTOR.executePlaybook('rebootSystems.yml', hostname)

    if historyURL is not None:
        return redirect(historyURL, code=302)
    else:
        return redirect(url_for("systems"))


@app.route("/systems/view/<string:hostname>/initialSetup", methods=["POST"])
def viewSystemInitialSetup(hostname):
    # validate and execute playbook
    ramDiskSize = request.form.getlist('ram_disk_size')[0] + "M"
    arguments = {'RamDiskSize': ramDiskSize}
    historyURL = ANSIBLE_REST_CONNECTOR.executePlaybook('initialSetup.yml', hostname, arguments)

    if historyURL is not None:
        return redirect(historyURL, code=302)
    else:
        return redirect(url_for('viewSystem', hostname=hostname))


@app.route("/systems/view/<string:hostname>/deployFluffi", methods=["POST"])
def viewDeployFluffi(hostname):
    # validate and execute playbook
    architecture = str(request.form.getlist('architecture')[0])
    system_object = ANSIBLE_REST_CONNECTOR.getSystemObjectByName(hostname)
    print(system_object)
    location = "{"
    if system_object.IsGroup:
        systems_list = ANSIBLE_REST_CONNECTOR.getSystemsOfGroup(hostname)
        for num, system in enumerate(systems_list):
            location = location + "\"" + system + "\":\"" + (db.session.query(models.SystemsLocation, models.Systems, models.Locations).join(
                models.Systems).filter(models.SystemsLocation.System == models.Systems.ID).filter_by(Name=system).join(
                models.Locations).filter(models.SystemsLocation.Location == models.Locations.ID).first())[2].Name + "\""
            if num < len(systems_list) - 1:
                location = location + ","
    else:
        location = location + "\"" + hostname + "\":\"" + (db.session.query(models.SystemsLocation, models.Systems, models.Locations).join(
            models.Systems).filter(models.SystemsLocation.System == models.Systems.ID).filter_by(Name=hostname).join(
            models.Locations).filter(models.SystemsLocation.Location == models.Locations.ID).first())[2].Name + "\""
    location = location + "}"

    arguments = {'architecture': architecture.split("-")[1], 'location': location}
    historyURL = ANSIBLE_REST_CONNECTOR.executePlaybook('deployFluffi.yml', hostname, arguments)

    if historyURL is not None:
        return redirect(historyURL, code=302)
    else:
        return redirect(url_for('viewSystem', hostname=hostname))


@app.route("/systems/view/<string:hostname>/syncRamdisk", methods=["POST"])
def viewSyncRamdisk(hostname):
    # validate and execute playbook 
    historyURL = ANSIBLE_REST_CONNECTOR.executePlaybook('syncToRamdisk.yml', hostname)

    if historyURL is not None:
        return redirect(historyURL, code=302)
    else:
        return redirect(url_for('viewSystem', hostname=hostname))


@app.route("/systems/view/<string:hostname>/deployPackage", methods=["POST"])
def viewInstallPackage(hostname):
    # validate and execute playbook
    packageFileName = "ftp://" + config.FTP_URL + "/SUT/" + str(request.form.getlist('installPackage')[0])
    arguments = {'filePathToFetchFromFTP': packageFileName}
    historyURL = ANSIBLE_REST_CONNECTOR.executePlaybook('deploySUT.yml', hostname, arguments)

    if historyURL is not None:
        return redirect(historyURL, code=302)
    else:
        return redirect(url_for('viewSystem', hostname=hostname))


@app.route("/systems/view/<string:hostname>/deployFuzzPackage", methods=["POST"])
def viewInstallFuzzjobPackage(hostname):
    # validate and execute playbook 
    selectedFuzzJob = str(request.form.getlist('fuzzingJob')[0])

    historyURL = None
    packages = []
    
    fuzzjob = db.session.query(models.Fuzzjob).filter_by(name=selectedFuzzJob).first()
        
    if fuzzjob is None:
        flash("Fuzzjob {} does not exist in db.".format(selectedFuzzJob), "error")
        return redirect(url_for('systems'))
    

    result = (db.session.query(models.FuzzjobDeploymentPackages, models.DeploymentPackages)
                    .join(models.DeploymentPackages, models.FuzzjobDeploymentPackages.DeploymentPackage == models.DeploymentPackages.ID)
                    .filter(models.FuzzjobDeploymentPackages.Fuzzjob == fuzzjob.ID)
                    .all())        

    for row in result:        
        packages.append(row[1].name)

    if len(packages) == 0:
        flash("No package found to deploy!", "error")
        return redirect(url_for('systems'))

    for packageFileName in packages:
        ftpPath = "ftp://" + config.FTP_URL + "/SUT/" + packageFileName
        arguments = {'filePathToFetchFromFTP': ftpPath}
        historyURL = ANSIBLE_REST_CONNECTOR.executePlaybook('deploySUT.yml', hostname, arguments)

    if historyURL is not None:
        return redirect(historyURL, code=302)
    else:
        return redirect(url_for('viewSystem', hostname=hostname))

@app.route("/systems/view/<string:hostname>/<string:groupName>/startFluffi", methods=["POST"])
def startFluffi(hostname, groupName):
    relevantHostnames = []
    if not groupName == '-':
        try:
            groups = ANSIBLE_REST_CONNECTOR.getHostAliveState()
            for group in groups:
                if group.Name == groupName:
                    for h in group.hosts:
                        relevantHostnames.append(h.Name)
        except Exception as e:
            try:
                ANSIBLE_REST_CONNECTOR.execHostAlive()
            except Exception as e:
                print(e)
            print(e)
    else:
        relevantHostnames.append(hostname)

    amount = request.form.getlist('numberOfComponents')[0]
    component = request.form.getlist('component')[0]
    architecture = str(request.form.getlist('architecture')[0])

    instanceType = None

    if int(component) == 0:
        instanceType = "TestcaseGenerator"
    elif int(component) == 1:
        instanceType = "TestcaseRunner"
    elif int(component) == 2:
        instanceType = "TestcaseEvaluator"
    elif int(component) == 4:
        instanceType = "LocalManager"

    if instanceType is None:
        flash("Error starting instance!", "error")
        return redirect(url_for('systems'))

    for host in relevantHostnames:
        requestCount = 0
        arguments = {}
        location = db.session.query(models.SystemsLocation, models.Systems, models.Locations).join(
            models.Systems).filter(models.SystemsLocation.System == models.Systems.ID).filter_by(Name=host).join(
            models.Locations).filter(models.SystemsLocation.Location == models.Locations.ID).first()
        arguments['component'] = component
        arguments['location'] = location[2].Name
        arguments['architecture'] = architecture.split("-")[1]
        url = "http://" + hostname + ":9000/start?cmd=" + architecture.split("-")[1] + "/" + str(
            instanceType) + " " + str(location[2].Name)
        for i in range(0, int(amount)):
            try:
                response = requests.get(url)
            except requests.exceptions.RequestException as e:
                print(e)
                flash("Start Server not available!" + str(e), "error")
                break
            requestCount += 1

        sys = models.Systems.query.filter_by(Name=host).first()
        instance = models.SystemFuzzjobInstances.query.filter_by(System=sys.ID, AgentType=int(component)).first()
        oldCount = 0
        if instance is not None:
            oldCount = instance.InstanceCount
            db.session.delete(instance)
            db.session.commit()
        newinstances = models.SystemFuzzjobInstances(System=sys.ID, AgentType=component,
                                                     InstanceCount=requestCount + oldCount)
        db.session.add(newinstances)
        db.session.commit()

    return redirect(url_for('systems'))


@app.route("/dashboardtrigger")
def dashboardTrigger():
    fuzzjobs = listFuzzJobs()
    inactivefuzzjobs = []
    activefuzzjobs = []
    
    for project in fuzzjobs:
        if (int(project.numLM) > 0) and (int(project.numTE) > 0) and (int(project.numTR) > 0) and (
                int(project.numTG) > 0):
            project.numMI = int(project.numTE) + int(project.numTR) + int(project.numTG)
            activefuzzjobs.append(project)
        else:
            inactivefuzzjobs.append(project)
       
    fuzzjobLocations = getLocations()
    
    return renderTemplate("dashboardTrigger.html",
                          title="Home",
                          fuzzjobs=activefuzzjobs,
                          locations=fuzzjobLocations,
                          inactivefuzzjobs=inactivefuzzjobs,
                          footer=FOOTER_SIEMENS)


@app.route("/dashboard")
def dashboard():
    return renderTemplate("dashboard.html",
                          title="Home")

