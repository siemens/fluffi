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
# Author(s): Pascal Eckmann

from app import db, models
from app.constants import *

import time


def loadSystems():
    if SYNC and not checkSyncedFile() and not checkSyncingFile():
        createSyncingFile()
        try:
            systemsDB = [s for s, in models.Systems.query.with_entities(models.Systems.Name).all()]
            if ANSIBLE_REST_CONNECTOR.checkFluffiInventory():
                systemsPM = [system[0] for system in ANSIBLE_REST_CONNECTOR.getSystems()]

                for persSystem in systemsPM:
                    if persSystem not in systemsDB:
                        sys = models.Systems(Name=persSystem)
                        loc = models.Locations.query.first()
                        if sys is not None and loc is not None:
                            try:
                                db.session.add(sys)
                                db.session.commit()
                                sysloc = models.SystemsLocation(System=sys.ID, Location=loc.ID)
                                db.session.add(sysloc)
                                db.session.commit()
                            except AttributeError as e:
                                print("Error adding new system: ", e)
                for system in systemsDB:
                    if system not in systemsPM:
                        try:
                            sys = models.Systems.query.filter_by(Name=system).first()
                            sysloc = models.SystemsLocation.query.filter_by(System=sys.ID).first()
                            db.session.delete(sysloc)
                            db.session.delete(sys)
                            db.session.commit()
                        except AttributeError as e:
                            print("Error removing system: ", e)
                if ANSIBLE_REST_CONNECTOR.getFluffiInventoryID() != -1:
                    if set([s for s, in models.Systems.query.with_entities(models.Systems.Name).all()]) == set(
                            [system[0] for system in ANSIBLE_REST_CONNECTOR.getSystems()]):
                        createSyncedFile()
        except Exception as e:
            print("Cannot get systems from database or PoleMarch.", e)
        finally:
            deleteSyncingFile()


def checkSyncedFile():
    fileAvailable = False
    for x in range(0, 2):
        if os.access(SYNCED_FILEPATH, os.F_OK):
            fileAvailable = True
        time.sleep(0.01)
    return fileAvailable


def checkSyncingFile():
    fileAvailable = False
    for x in range(0, 2):
        if os.access(SYNCING_FILEPATH, os.F_OK):
            fileAvailable = True
        time.sleep(0.01)
    return fileAvailable


def deleteSyncingFile():
    for x in range(0, 2):
        if os.access(SYNCING_FILEPATH, os.F_OK):
            os.remove(SYNCING_FILEPATH)
        time.sleep(0.01)
    if os.access(SYNCING_FILEPATH, os.F_OK):
        print("Syncing lock couldn't be deleted.")


def createSyncedFile():
    if not os.access(SYNCED_FILEPATH, os.F_OK):
        open(SYNCED_FILEPATH, 'a').close()


def createSyncingFile():
    if not os.access(SYNCING_FILEPATH, os.F_OK):
        open(SYNCING_FILEPATH, 'a').close()


def getSyncStatus():
    if SYNC and not checkSyncedFile() and not checkSyncingFile():
        return True
    else:
        return False
