# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Junes Najah, Michael Kraus, Abian Blome, Pascal Eckmann, Fabian Russwurm, Thomas Riedmaier

from flask import flash, redirect, url_for, request, send_file, Markup
from werkzeug.exceptions import HTTPException

from .controllers import *
from .queries import *
from .helpers import *
from .forms import *
from .constants import *

import json, os
import requests

archive_threads = {}


@app.route("/")
@app.route("/index")
def index():
    fuzzjobs = listFuzzJobs()
    inactivefuzzjobs = []
    activefuzzjobs = []

    for project in fuzzjobs:
        if ((int(project.numLM) > 0) and
                (int(project.numTE) > 0) and
                (int(project.numTR) > 0) and
                (int(project.numTG) > 0)):
            activefuzzjobs.append(project)
        else:
            inactivefuzzjobs.append(project)

    locations = getLocations()
    user = {"nickname": "amb"}

    return renderTemplate("index.html",
                          title="Home",
                          user=user,
                          fuzzjobs=activefuzzjobs,
                          locations=locations,
                          inactivefuzzjobs=inactivefuzzjobs,
                          footer=FOOTER_SIEMENS)


@app.route("/projects")
def projects():
    projects = getProjects()

    return renderTemplate("projects.html",
                          title="Projects",
                          projects=projects)


@app.route("/projects/archive/<int:projId>", methods=["POST"])
def archiveProject(projId):
    global archive_threads
    project = models.Fuzzjob.query.filter_by(id=projId).first()
    nice_name = project.name
    print(nice_name)

    if len(archive_threads) is 0:
        thread_id = 0
        archive_threads[thread_id] = ArchiveProject(projId, nice_name)
        archive_threads[thread_id].start()
        return renderTemplate("progressArchiveFuzzjob.html",
                              thread_id=thread_id,
                              nice_name=nice_name)
    else:
        return renderTemplate("progressArchiveFuzzjob.html",
                              thread_id=-1,
                              nice_name=nice_name)


@app.route('/progressArchiveFuzzjob/<int:thread_id>')
def progressArchiveFuzzjob(thread_id):
    global archive_threads

    if len(archive_threads) > 0:
        if archive_threads[thread_id].status[0] is 0 or archive_threads[thread_id].status[0] == 1:
            return str(archive_threads[thread_id].status[1])
        elif archive_threads[thread_id].status[0] is 2:
            errorMessage = "Error: " + archive_threads[thread_id].status[1]
            stopProcess(thread_id)
            return errorMessage
        elif archive_threads[thread_id].status[0] is 3:
            message = str(archive_threads[thread_id].status[1])
            archive_threads.clear()
            return message
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


@app.route("/projects/<int:projId>/addSetting", methods=["GET", "POST"])
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
        if not targetFile:
            flash("Invalid file", "error")
            return redirect(request.url)
        if not ('.' in targetFile.filename and targetFile.filename.rsplit('.', 1)[1].lower() in set(["zip"])):
            flash("Only *.zip files are allowed!", "error")
            return redirect(request.url)
        msg, category = uploadNewTarget(targetFile)
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
def createProjectModule(projId):
    moduleForm = CreateProjectModuleForm()

    if request.method == "POST" and moduleForm.is_submitted:
        msg, category = insertModules(projId, request.form)
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
def addLocationProject(locId):
    projects = []

    for project in models.Fuzzjob.query.all():
        if models.LocationFuzzjobs.query.filter_by(Location=locId, Fuzzjob=project.id).first() is None:
            data = (project.id, project.name)
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
def createLocation():
    form = CreateLocationForm()

    if form.validate_on_submit():
        location = models.Locations(Name=form.name.data)
        db.session.add(location)
        db.session.commit()
        flash("Added Location", "success")
        return redirect("/locations")

    return renderTemplate("createLocation.html",
                          title="Add new Location",
                          form=form)


@app.route("/locations/removeLocation/<int:locId>")
def removeLocation(locId):
    location = models.Locations.query.filter_by(id=locId).first()
    location_fuzzjobs = models.LocationFuzzjobs.query.filter_by(Location=location.id)
    db.session.delete(location)

    for location_fuzzjob in location_fuzzjobs:
        db.session.delete(location_fuzzjob)

    db.session.commit()
    flash("Location deleted", "success")

    return redirect("/locations")


@app.route("/projects/<int:projId>/renameElement", methods=["POST"])
def renameElement(projId):
    if not request.json:
        abort(400)

    message, status = insertOrUpdateNiceName(projId, request.json["myId"], request.json["newName"],
                                             request.json["command"], request.json["elemType"])

    return json.dumps({"message": message, "status": status, "command": request.json["command"]})


@app.route("/projects/<int:projId>/download")
def downloadTestcaseSet(projId):
    project = models.Fuzzjob.query.filter_by(id=projId).first()
    name = [
        "population",
        "accessViolations",
        "accessViolationsUnique",
        "crashes",
        "crashesUnique",
        "hangs",
        "noResponses"
    ]
    nice_name = project.name + ": Testcase Set"
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

    return createZipArchive(projId, name, nice_name, statement_count, statement)


def createZipArchive(projId, name, nice_name, count_statement, statement):
    global archive_threads

    if len(archive_threads) is 0:
        thread_id = 0
        archive_threads[thread_id] = CreateTestcaseArchive(projId, nice_name, name, count_statement, statement)
        archive_threads[thread_id].setMaxVal()
        archive_threads[thread_id].start()

        max_val = 0
        for val in archive_threads[thread_id].max_val_list:
            max_val = max_val + val

        if len(archive_threads[thread_id].name_list) == 1:
            download = archive_threads[thread_id].name_list[0] + ".zip"
        else:
            download = "testcase_set.zip"

        return renderTemplate("progressDownload.html",
                              thread_id=thread_id,
                              max_val=max_val,
                              nice_name=nice_name,
                              download=download)

    else:
        return renderTemplate("progressDownload.html",
                              thread_id=-1,
                              max_val=10,
                              nice_name=nice_name,
                              download="error")


@app.route('/statusDownload')
def statusDownload():
    if len(archive_threads) > 0:
        thread_id = 0
        max_val = 0
        nice_name = archive_threads[thread_id].nice_name

        if isinstance(archive_threads[thread_id], ArchiveProject):

            return renderTemplate("progressArchiveFuzzjob.html",
                                  thread_id=thread_id,
                                  nice_name=nice_name)
        else:
            for val in archive_threads[thread_id].max_val_list:
                max_val = max_val + val

            if len(archive_threads[thread_id].name_list) == 1:
                download = archive_threads[thread_id].name_list[0] + ".zip"
            else:
                download = "testcase_set.zip"

            return renderTemplate("progressDownload.html",
                                  thread_id=thread_id,
                                  max_val=max_val,
                                  nice_name=nice_name,
                                  download=download)
    else:
        return redirect(url_for("projects"))


@app.route('/progressDownload/<int:thread_id>')
def progressDownload(thread_id):
    global archive_threads

    if len(archive_threads) > 0:
        if len(archive_threads[thread_id].name_list) == 1:
            path = getDownloadPath() + archive_threads[thread_id].name_list[0] + ".zip"
        else:
            path = getDownloadPath() + "testcase_set.zip"

        if os.path.isfile(path) and archive_threads[thread_id].status[0] is 1:
            archive_threads.clear()
            return str(-1)
        elif archive_threads[thread_id].status[0] is 2:
            errorMessage = "Error: " + archive_threads[thread_id].status[1]
            stopProcess(thread_id)
            return errorMessage
        else:
            return str(archive_threads[thread_id].progress)
    else:
        return "NONE"


@app.route('/stopProcess/<int:thread_id>')
def stopProcess(thread_id):
    global archive_threads
    if thread_id is -1:
        return redirect(url_for("statusDownload"))
    elif len(archive_threads) > 0:
        archive_threads[thread_id].end()
        archive_threads.clear()
        return redirect(url_for("projects"))
    else:
        return redirect(url_for("statusDownload"))


@app.route('/download/<string:archive>')
def downloadArchive(archive):
    return send_file(getDownloadPath() + archive, as_attachment=True, cache_timeout=0)


@app.route("/projects/<int:projId>/population")
def viewPopulation(projId):
    data = getGeneralInformationData(projId, getITQueryOfTypeNoRaw(TESTCASE_TYPES["population"]))
    data.name = "Population"
    data.redirect = "population"
    data.downloadName = "downloadPopulation"

    return renderTemplate("viewTCsTempl.html",
                          title="View Population",
                          data=data)


@app.route("/projects/<int:projId>/population/download")
def downloadPopulation(projId):
    project = models.Fuzzjob.query.filter_by(id=projId).first()
    name = ["population"]
    nice_name = project.name + ": Population"

    return createZipArchive(projId, name, nice_name, [getITCountOfTypeQuery(TESTCASE_TYPES[name[0]])],
                            [getITQueryOfType(TESTCASE_TYPES[name[0]])])


@app.route("/projects/<int:projId>/accessVioTotal")
def viewAccessVioTotal(projId):
    data = getGeneralInformationData(projId, getITQueryOfTypeNoRaw(TESTCASE_TYPES["accessViolations"]))
    data.name = "Access Violations"
    data.redirect = "accessVioTotal"
    data.downloadName = "downloadAccessVioTotal"

    return renderTemplate("viewTCsTempl.html",
                          title="View Access Violations",
                          data=data)


@app.route("/projects/<int:projId>/accessVioTotal/download")
def downloadAccessVioTotal(projId):
    project = models.Fuzzjob.query.filter_by(id=projId).first()
    name = ["accessViolations"]
    nice_name = project.name + ": Access Violations (Total)"

    return createZipArchive(projId, name, nice_name, [getITCountOfTypeQuery(TESTCASE_TYPES[name[0]])],
                            [getITQueryOfType(TESTCASE_TYPES[name[0]])])


@app.route("/projects/<int:projId>/accessVioUnique")
def viewAccessVioUnique(projId):
    data = getGeneralInformationData(projId, UNIQUE_ACCESS_VIOLATION_NO_RAW)
    data.name = "Unique Access Violations"
    data.redirect = "accessVioUnique"
    data.downloadName = "downloadAccessVioUnique"

    return renderTemplate("viewTCsTempl.html",
                          title="View Unique Access Violations",
                          data=data)


@app.route("/projects/<int:projId>/accessVioUnique/download")
def downloadAccessVioUnique(projId):
    project = models.Fuzzjob.query.filter_by(id=projId).first()
    name = ["accessViolationsUnique"]
    nice_name = project.name + ": Access Violations (Unique)"

    return createZipArchive(projId, name, nice_name, [NUM_UNIQUE_ACCESS_VIOLATION], [UNIQUE_ACCESS_VIOLATION])


@app.route("/projects/<int:projId>/totalCrashes")
def viewTotalCrashes(projId):
    data = getGeneralInformationData(projId, getITQueryOfTypeNoRaw(TESTCASE_TYPES["crashes"]))
    data.name = "Crashes"
    data.redirect = "totalCrashes"
    data.downloadName = "downloadTotalCrashes"

    return renderTemplate("viewTCsTempl.html",
                          title="View Total Crashes",
                          data=data)


@app.route("/projects/<int:projId>/totalCrashes/download")
def downloadTotalCrashes(projId):
    project = models.Fuzzjob.query.filter_by(id=projId).first()
    name = ["crashes"]
    nice_name = project.name + ": Crashes (Total)"

    return createZipArchive(projId, name, nice_name, [getITCountOfTypeQuery(TESTCASE_TYPES[name[0]])],
                            [getITQueryOfType(TESTCASE_TYPES[name[0]])])


@app.route("/projects/<int:projId>/uniqueCrashes")
def viewUniqueCrashes(projId):
    data = getGeneralInformationData(projId, UNIQUE_CRASHES_NO_RAW)
    data.name = "Unique Crashes"
    data.redirect = "uniqueCrashes"
    data.downloadName = "downloadUniqueCrashes"

    return renderTemplate("viewTCsTempl.html",
                          title="View Unique Crashes",
                          data=data)


@app.route("/projects/<int:projId>/uniqueCrashes/download")
def downloadUniqueCrashes(projId):
    project = models.Fuzzjob.query.filter_by(id=projId).first()
    name = ["crashesUnique"]
    nice_name = project.name + ": Crashes (Unique)"

    return createZipArchive(projId, name, nice_name, [NUM_UNIQUE_CRASH], [UNIQUE_CRASHES])


@app.route("/projects/<int:projId>/hangs")
def viewHangs(projId):
    data = getGeneralInformationData(projId, getITQueryOfTypeNoRaw(TESTCASE_TYPES["hangs"]))
    data.name = "Hangs"
    data.redirect = "hangs"
    data.downloadName = "downloadHangs"

    return renderTemplate("viewTCsTempl.html",
                          title="View Hangs",
                          data=data)


@app.route("/projects/<int:projId>/hangs/download")
def downloadHangs(projId):
    project = models.Fuzzjob.query.filter_by(id=projId).first()
    name = ["hangs"]
    nice_name = project.name + ": Hangs"

    return createZipArchive(projId, name, nice_name, [getITCountOfTypeQuery(TESTCASE_TYPES[name[0]])],
                            [getITQueryOfType(TESTCASE_TYPES[name[0]])])


@app.route("/projects/<int:projId>/noResponse")
def viewNoResponses(projId):
    data = getGeneralInformationData(projId, getITQueryOfTypeNoRaw(TESTCASE_TYPES["noResponses"]))
    data.name = "No Responses"
    data.redirect = "noResponse"
    data.downloadName = "downloadNoResponses"

    return renderTemplate("viewTCsTempl.html",
                          title="View No Responses",
                          data=data)


@app.route("/projects/<int:projId>/noResponse/download")
def downloadNoResponses(projId):
    project = models.Fuzzjob.query.filter_by(id=projId).first()
    name = ["noResponses"]
    nice_name = project.name + ": No Response"

    return createZipArchive(projId, name, nice_name, [getITCountOfTypeQuery(TESTCASE_TYPES[name[0]])],
                            [getITQueryOfType(TESTCASE_TYPES[name[0]])])


@app.route("/projects/<int:projId>/violations")
def viewViolations(projId):
    violationsAndCrashes = getViolationsAndCrashes(projId)

    return renderTemplate("viewAccessViolations.html",
                          title="View AccessViolations",
                          violationsAndCrashes=violationsAndCrashes)


@app.route("/projects/<int:projId>/crashes/download/<string:footprint>/<int:testCaseType>")
def downloadCrashes(projId, footprint, testCaseType):
    project = models.Fuzzjob.query.filter_by(id=projId).first()
    name = ["crashes"]
    nice_name = project.name + ": Crashes (Footprint)"

    return createZipArchive(projId, name, nice_name, [getCrashesQueryCount(footprint, testCaseType)],
                            [getCrashesQuery(footprint, testCaseType)])


@app.route("/projects/<int:projId>/crashesorvio/download/<string:footprint>")
def downloadCrashesOrViolations(projId, footprint):
    project = models.Fuzzjob.query.filter_by(id=projId).first()
    name = ["crashes_or_vio_of_footprint"]
    nice_name = project.name + ": Crashes and Violations (Footprint)"

    return createZipArchive(projId, name, nice_name, [getCrashesOrViosOfFootprintCount(footprint)],
                            [getCrashesOrViosOfFootprint(footprint)])


@app.route("/projects/<int:projId>/getTestcase/<string:testcaseId>")
def getTestcase(projId, testcaseId):
    byteIO, filename = createByteIOTestcase(projId, testcaseId)

    return send_file(byteIO,
                     attachment_filename=filename,
                     as_attachment=True)


@app.route("/projects/<int:projId>/getSmallestVioOrCrashTestcase/<string:footprint>")
def getSmallestVioOrCrashTestcase(projId, footprint):
    byteIO = createByteIOForSmallestVioOrCrash(projId, footprint)

    return send_file(byteIO,
                     attachment_filename=footprint.replace(":", "_"),
                     as_attachment=True)


@app.route("/projects/<int:projId>/managedInstances")
def viewManagedInstances(projId):
    managedInstances, summarySection, localManagers = getManagedInstancesAndSummary(projId)

    return renderTemplate("viewManagedInstances.html",
                          title="View ManagedInstances",
                          managedInstances=managedInstances,
                          summarySection=summarySection,
                          localManagers=localManagers
                          )


@app.route("/projects/<int:projId>/configSystemInstances")
def viewConfigSystemInstances(projId):
    # initialize forms
    systemInstanceConfigForm = SystemInstanceConfigForm()
    fuzzjob = db.session.query(models.Fuzzjob).filter_by(id=projId).first()
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
                                                    models.SystemFuzzjobInstances.Architecture).filter_by(
        Fuzzjob=projId).all()
    for system in sysList:
        s = {'name': system.Name, 'tg': 0, 'tr': 0, 'te': 0, 'tgarch': "", 'trarch': "", 'tearch': ""}
        if 'lemming' not in system.Name:
            sysInstanceList.append(s)
        else:
            lemmingInstanceList.append(s)
        for conf in dbconfiguredFuzzjobInstances:
            if system.id == conf.System:
                if conf.AgentType == 0:
                    s['tg'] = conf.InstanceCount
                    s['tgarch'] = "(" + conf.Architecture + ")"
                if conf.AgentType == 1:
                    s['tr'] = conf.InstanceCount
                    s['trarch'] = "(" + conf.Architecture + ")"
                if conf.AgentType == 2:
                    s['te'] = conf.InstanceCount
                    s['tearch'] = "(" + conf.Architecture + ")"

    for conf in dbconfiguredFuzzjobInstances:
        if conf.AgentType == 4:
            lmCount = lmCount + conf.InstanceCount

    return renderTemplate("viewConfigSystemInstances.html",
                          title="View Instance Configuration",
                          system=system,
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
def addProjectLocation(projId):
    locationForm = getLocationFormWithChoices(projId, AddProjectLocationForm())

    if request.method == 'POST' and locationForm.is_submitted:
        locations = request.form.getlist('location')
        for loc in locations:
            location = models.Locations.query.filter_by(Name=loc).first()
            location_fuzzjob = models.LocationFuzzjobs(Location=location.id, Fuzzjob=projId)
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
        msg, category = insertTestcases(projId, request.files.getlist('addTestcase'))
        flash(msg, category)
        return redirect("/projects/{}/population".format(projId))
    else:
        flash("Please select files to add ...", "error")
        return redirect("/projects/{}/population".format(projId))


@app.route("/locations/assignWorker/<string:workerID>", methods=["POST"])
def assignWorker(workerID):
    message = ""

    if not request.json:
        abort(400)

    try:
        worker = models.Workers.query.filter_by(Servicedescriptorguid=workerID).first()
        worker.Fuzzjob = request.json["ProjectID"]
        db.session.commit()
    except Exception as e:
        print(e)
        message = "Could not assign worker"

    return json.dumps({"message": message})


@app.route("/projects/<int:projId>/changeSetting/<int:settingId>", methods=["POST"])
def changeProjectSetting(projId, settingId):
    if not request.json:
        abort(400)

    message, status = updateSettings(projId, settingId, request.json["SettingValue"])

    return json.dumps({"message": message, "status": status})


@app.route("/projects/<int:projId>/delLocation/<string:locName>", methods=["GET"])
def removeProjectLocation(projId, locName):
    location = models.Locations.query.filter_by(Name=locName).first()
    location_fuzzjob = models.LocationFuzzjobs.query.filter_by(Fuzzjob=projId, Location=location.id).first()
    db.session.delete(location_fuzzjob)
    db.session.commit()
    flash("Location deleted", "success")

    return redirect("/projects/view/%d" % projId)


@app.route("/projects/<int:projId>/delTestcase/<string:testcaseId>/<string:tcType>", methods=["GET"])
def removeProjectTestcase(projId, testcaseId, tcType):
    (guid, localId) = testcaseId.split(":")
    msg, category = deleteElement(projId, "Testcase", DELETE_TC_WTIH_LOCALID, {"guid": guid, "localId": localId})
    flash(msg, category)

    return redirect("/projects/{}/{}".format(projId, tcType))


@app.route("/projects/<int:projId>/delModule/<int:moduleId>", methods=["GET"])
def removeProjectModule(projId, moduleId):
    msg, category = deleteElement(projId, "Module", DELETE_MODULE_BY_ID, {"Id": moduleId})
    flash(msg, category)

    return redirect("/projects/view/%d" % projId)


@app.route("/projects/<int:projId>/delSetting/<int:settingId>", methods=["GET"])
def removeProjectSetting(projId, settingId):
    msg, category = deleteElement(projId, "Setting", DELETE_SETTING_BY_ID, {"Id": settingId})
    flash(msg, category)

    return redirect("/projects/view/%d" % projId)


@app.route("/projects/createProject", methods=["GET", "POST"])
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
    project = models.Fuzzjob.query.filter_by(id=projId).first()
    fuzzjobname = project.name

    return renderTemplate("viewProjectGraph.html",
                          title="Create custom project",
                          fuzzjobname=fuzzjobname)


@app.route("/projects/<int:projId>/testcaseGraph", methods=["GET"])
def viewTestcaseGraph(projId):
    graphdata = getGraphData(projId)

    return json.dumps(graphdata)


@app.route("/locations/view/<int:locId>")
def viewLocation(locId):
    location = getLocation(locId)

    return renderTemplate("viewLocation.html",
                          title="Location details",
                          location=location)


@app.route("/projects/view/<int:projId>")
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
                          moduleForm=moduleForm
                          )


@app.route("/locations")
def locations():
    locations = models.Locations.query.all()

    return renderTemplate("locations.html",
                          title="Locations",
                          locations=locations)


@app.route("/commands")
def commands():
    commands = models.CommandQueue.query.all()

    return renderTemplate("viewCommandQueue.html",
                          title="Commands",
                          commands=commands)


@app.route("/systems")
def systems():
    groups = []
    addNewSystemToPolemarchForm = AddNewSystemForm()
    projIds = models.Fuzzjob.query.all()

    managedInstancesAll = []
    actualAgentStarterMode = models.GmOptions.query.filter_by(setting="agentstartermode").first().value.upper()
    changeAgentStarterModeForm = ChangeAgentStarterModeForm()
    changePXEForm = ChangePXEForm()
    bootsystemdir = models.GmOptions.query.filter_by(setting="bootsystemdir").first().value
    availablePXEsystems = FTP_CONNECTOR.getListOfFilesOnFTPServer("tftp-roots/")
    changePXEForm.pxesystem.choices = availablePXEsystems

    for project in projIds:
        managedInstances, summarySection, localmanagers = getManagedInstancesAndSummary(project.id)
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
            models.SystemFuzzjobInstances.System == models.Systems.id).all()
        instanceLM = db.session.query(models.SystemFuzzjobInstances,
                                      func.sum(models.SystemFuzzjobInstances.InstanceCount).label('lm'),
                                      models.Systems).group_by(models.SystemFuzzjobInstances.System).join(
            models.Systems).filter(and_(models.SystemFuzzjobInstances.System == models.Systems.id),
                                   (models.SystemFuzzjobInstances.AgentType == 4)).all()

        localMan = db.session.query(models.Localmanagers).all()
        systemLocations = db.session.query(models.SystemsLocation, models.Systems, models.Locations).join(
            models.Systems).filter(models.SystemsLocation.System == models.Systems.id).join(models.Locations).filter(
            models.SystemsLocation.Location == models.Locations.id).all()

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

        for group in groups:
            for h in group.hosts:
                if h.Name.startswith('dev-'):
                    h.isNotPersistentDevSys = True
                    h.Name = h.Name[4:]
        locations = models.Locations.query.all()
    except Exception as e:
        print(e)
        flash(
            "Error retrieving or parsing data from polemarch! Check polemarch history if polemarch is busy or other "
            "error occured. Attach to webui docker container....",
            "error")
        locations = []

    return renderTemplate("systems.html",
                          title="Systems",
                          systems=groups,
                          locations=locations,
                          addNewSystemToPolemarchForm=addNewSystemToPolemarchForm,
                          changePXEForm=changePXEForm,
                          bootsystemdir=bootsystemdir,
                          availablePXEsystems=availablePXEsystems,
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
    numberOfexistingSystems = models.Systems.query.filter_by(Name="dev-" + hostname).all()

    if (len(numberOfexistingSystems) > 0):
        flash("Error adding new system! System already exists (or check database)!", "error")
        return redirect(url_for('systems'))

    addResult = ANSIBLE_REST_CONNECTOR.addNewSystem(hostname, hostgroup)

    if addResult is False:
        flash("Error adding new system! PoleMarch does not accept special characters like _underscore!", "error")
        return redirect(url_for('systems'))
    else:
        loc = models.Locations.query.first()
        sys = models.Systems(Name="dev-" + hostname)
        db.session.add(sys)
        db.session.commit()
        sysloc = models.SystemsLocation(System=sys.id, Location=loc.id)
        db.session.add(sysloc)
        db.session.commit()
        flash("Added new system!", "success")
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


@app.route("/systems/removeDevSystem/<string:hostName>/<string:hostId>", methods=["GET"])
def removeDevSystemFromPolemarch(hostName, hostId):
    removeResult = ANSIBLE_REST_CONNECTOR.removeDevSystem(hostId)

    if removeResult is False:
        flash("Error removing dev system!", "error")
        return redirect(url_for('systems'))
    else:
        sys = models.Systems.query.filter_by(Name="dev-" + hostName).first()
        sysloc = models.SystemsLocation.query.filter_by(System=sys.id).first()
        db.session.delete(sysloc)
        db.session.delete(sys)
        db.session.commit()
        flash("Removed " + hostName + "!", "success")
        return redirect(url_for('systems'))


@app.route("/systems/<string:hostName>/updateSystemLocation/<int:locationId>", methods=["POST"])
def updateSystemLocation(hostName, locationId):
    if (len(hostName) > 0) and (locationId > 0):
        try:
            sys = models.Systems.query.filter_by(Name=hostName).first()
            if sys is not None:
                systemloc = models.SystemsLocation.query.filter_by(System=sys.id).first()
            if sys is None:
                sys = models.Systems.query.filter_by(Name="dev-" + hostName).first()
                systemloc = models.SystemsLocation.query.filter_by(System=sys.id).first()
            systemloc.Location = locationId
            db.session.commit()
            message = "Setting modified"
            flash("Changed Location for " + hostName, "success")
            return json.dumps({"message": message, "status": "OK"})
        except Exception as e:
            print(e)
            message = "Could not modify setting"
            flash("Error changing Location for " + hostName, "error")
    else:
        message = "Value cannot be empty!"

    return json.dumps({"message": message, "status": "Error"})


@app.route("/systems/view/<string:hostname>/<string:group>", methods=["GET"])
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
    availableInstallPackagesFromFTP = FTP_CONNECTOR.getListOfFilesOnFTPServer("SUT/")
    deployPackageForm.installPackage.choices = availableInstallPackagesFromFTP
    availableArchitecturesFromFTP = FTP_CONNECTOR.getListOfArchitecturesOnFTPServer("fluffi/" + group + "/", group)
    fluffiDeployForm.architecture.choices = availableArchitecturesFromFTP
    availableArchitecturesFromFTP = FTP_CONNECTOR.getListOfArchitecturesOnFTPServer("fluffi/" + group + "/", group)
    startFluffiComponentForm.architecture.choices = availableArchitecturesFromFTP
    system = ANSIBLE_REST_CONNECTOR.getSystemObjectByName(hostname)
    jobList = []
    fuzzjobs = db.session.query(models.SystemsLocation, models.Systems, models.LocationFuzzjobs, models.Fuzzjob).join(
        models.Systems).filter(models.SystemsLocation.System == models.Systems.id).filter_by(Name=hostname).join(
        models.LocationFuzzjobs).filter(models.SystemsLocation.Location == models.LocationFuzzjobs.Location).join(
        models.Fuzzjob).filter(models.LocationFuzzjobs.Fuzzjob == models.Fuzzjob.id)

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
        models.Systems).filter(models.SystemFuzzjobInstances.System == models.Systems.id).group_by(
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
        models.Systems).filter(models.SystemFuzzjobInstances.System == models.Systems.id).filter_by(
        Name=hostname).all()

    for fuzzjob in configFuzzjobs:
        fj = {'name': fuzzjob.name, 'tg': 0, 'tr': 0, 'te': 0, 'tgarch': "", 'trarch': "", 'tearch': ""}
        fuzzjobList.append(fj)
        for conf in dbconfiguredFuzzjobInstances:
            if fuzzjob.id == conf.Fuzzjob:
                if conf.AgentType == 0:
                    fj['tg'] = conf.InstanceCount
                    fj['tgarch'] = "(" + conf.Architecture + ")"
                if conf.AgentType == 1:
                    fj['tr'] = conf.InstanceCount
                    fj['trarch'] = "(" + conf.Architecture + ")"
                if conf.AgentType == 2:
                    fj['te'] = conf.InstanceCount
                    fj['tearch'] = "(" + conf.Architecture + ")"

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
                          lmCount=lmCount
                          )


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
    msg = ""
    category = ""

    if request.method == 'POST' and form.is_submitted:
        msg, category = insertFormInputForConfiguredFuzzjobInstances(request, fuzzjob)
    flash(msg, category)

    return redirect(url_for("viewConfigSystemInstances", projId=projId.id))


@app.route("/systems/removeConfiguredInstances/<string:system>/<string:fuzzjob>/<string:type>", methods=["POST"])
def removeConfiguredInstances(system, fuzzjob, type):
    status = 'OK'
    try:
        sys = models.Systems.query.filter_by(Name=system).first()
        agenttype = 0
        fuzzjobId = None
        if "tr" in type:
            agenttype = 1
        if "te" in type:
            agenttype = 2
        if "lm" in type:
            agenttype = 4
        if agenttype != 4:
            fj = models.Fuzzjob.query.filter_by(name=fuzzjob).first()
            fuzzjobId = fj.id

        instance = models.SystemFuzzjobInstances.query.filter_by(System=sys.id, Fuzzjob=fuzzjobId,
                                                                 AgentType=int(agenttype)).first()
        if instance is not None:
            db.session.delete(instance)
            db.session.commit()
    except Exception as e:
        status = 'error'

    return json.dumps({"status": status})


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
    location = db.session.query(models.SystemsLocation, models.Systems, models.Locations).join(
        models.Systems).filter(models.SystemsLocation.System == models.Systems.id).filter_by(Name=hostname).join(
        models.Locations).filter(models.SystemsLocation.Location == models.Locations.id).first()
    arguments = {'architecture': architecture.split("-")[1], 'location': location[2].Name}
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
    historyURL = None
    selectedFuzzJob = str(request.form.getlist('fuzzingJob')[0])
    packages = []
    res = db.session.query(models.Fuzzjob, models.FuzzjobDeploymentPackages, models.DeploymentPackages).filter_by(
        name=selectedFuzzJob).join(models.FuzzjobDeploymentPackages).filter(
        models.Fuzzjob.id == models.FuzzjobDeploymentPackages.Fuzzjob).join(models.DeploymentPackages).filter(
        models.FuzzjobDeploymentPackages.DeploymentPackage == models.DeploymentPackages.id)

    for package in res:
        packages.append(package[2].name)

    if len(packages) < 1:
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
        groups = ANSIBLE_REST_CONNECTOR.getHostAliveState()
        for group in groups:
            if group.Name == groupName:
                for h in group.hosts:
                    relevantHostnames.append(h.Name)
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
            models.Systems).filter(models.SystemsLocation.System == models.Systems.id).filter_by(Name=host).join(
            models.Locations).filter(models.SystemsLocation.Location == models.Locations.id).first()
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
        instance = models.SystemFuzzjobInstances.query.filter_by(System=sys.id, AgentType=int(component)).first()
        oldCount = 0
        if instance is not None:
            oldCount = instance.InstanceCount
            db.session.delete(instance)
            db.session.commit()
        newinstances = models.SystemFuzzjobInstances(System=sys.id, AgentType=component,
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
    user = {"nickname": "amb"}
    return renderTemplate("dashboardTrigger.html",
                          title="Home",
                          user=user,
                          fuzzjobs=activefuzzjobs,
                          locations=fuzzjobLocations,
                          inactivefuzzjobs=inactivefuzzjobs,
                          footer=FOOTER_SIEMENS)


@app.route("/dashboard")
def dashboard():
    user = {"nickname": "amb"}
    return renderTemplate("dashboard.html",
                          title="Home",
                          user=user)
