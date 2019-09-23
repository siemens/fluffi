# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Michael Kraus, Junes Najah, Abian Blome, Thomas Riedmaier

from app import db
from sqlalchemy import ForeignKey

§§
class Fuzzjob(db.Model):
    __tablename__ = 'fuzzjob'
§§    id = db.Column(db.Integer, primary_key = True)
    name = db.Column(db.String(256))
    DBHost = db.Column(db.String(256))
    DBUser = db.Column(db.String(256))
    DBPass = db.Column(db.String(256))
    DBName = db.Column(db.String(256))

    def __repr__(self):
§§        return '<FuzzJob %r>' % self.name
§§

class Locations(db.Model):
    __tablename__ = 'locations'
§§    id = db.Column(db.Integer, primary_key = True)
    Name = db.Column(db.String(256))

    def __repr__(self):
§§        return '<Location %r>' % self.Name
§§

class Systems(db.Model):
    __tablename__ = 'systems'
§§    id = db.Column(db.Integer, primary_key = True)
    Name = db.Column(db.String(256))

    def __repr__(self):
§§        return '<System %r>' % self.Name
§§

class Localmanagers(db.Model):
    __tablename__ = 'localmanagers'
§§    Servicedescriptorguid = db.Column(db.String(50), primary_key = True)
    Servicedescriptorhostandport = db.Column(db.String(256))
    Location = db.Column(db.String(256))
    Fuzzjob = db.Column(db.Integer)

    def __repr__(self):
§§        return '<Localmanager %r>' % self.Servicedescriptorguid
§§

class LocalmanagersStatuses(db.Model):
    __tablename__ = 'localmanagers_statuses'
§§    Id = db.Column(db.Integer, primary_key = True)
    Servicedescriptorguid = db.Column(db.String(50))
    Timeofstatus = db.Column(db.DateTime)
    Status = db.Column(db.String(1000))

§§
class Workers(db.Model):
    __tablename__ = 'workers'
§§    Servicedescriptorguid = db.Column(db.String(50), primary_key = True)
    ServiceDescriptorhostandport = db.Column(db.String(50))
    Fuzzjob = db.Column(db.Integer)
    Location = db.Column(db.Integer)
    Timeoflastrequest = db.Column(db.DateTime)
    Agenttype = db.Column(db.Integer)
    Agentsubtypes = db.Column(db.String(1000))

§§
class LocationFuzzjobs(db.Model):
    __tablename__ = 'location_fuzzjobs'
§§    Location = db.Column(ForeignKey('locations.id'), primary_key = True)
§§    Fuzzjob = db.Column(ForeignKey('fuzzjob.id'), primary_key = True)

    def __repr__(self):
§§        return '<LocationFuzzJob %r>' % self.Location
§§

class CommandQueue(db.Model):
    __tablename__ = 'command_queue'
§§    Id = db.Column(db.Integer, primary_key = True)
    Command = db.Column(db.String(256))
    Argument = db.Column(db.String(4096))
    CreationDate = db.Column(db.DateTime)
    Done = db.Column(db.Integer)
    Error = db.Column(db.String(4096))

§§
class DeploymentPackages(db.Model):
    __tablename__ = 'deployment_packages'
§§    id = db.Column(db.Integer, primary_key = True)
    name = db.Column(db.String(150))

    def __repr__(self):
§§        return '<DeploymentPackage %r>' % self.name
§§

class FuzzjobDeploymentPackages(db.Model):
    __tablename__ = 'fuzzjob_deployment_packages'
§§    Fuzzjob = db.Column(ForeignKey('fuzzjob.id'), primary_key = True)
§§    DeploymentPackage = db.Column(ForeignKey('deployment_packages.id'), primary_key = True)

    def __repr__(self):
§§        return '<FuzzjobDeploymentPackage %r>' % self.DeploymentPackage
§§

class SystemsLocation(db.Model):
    __tablename__ = 'systems_location'
§§    System = db.Column(ForeignKey('systems.id'), primary_key = True)
§§    Location = db.Column(ForeignKey('locations.id'), ForeignKey('location_fuzzjobs.Location'), primary_key = False)

    def __repr__(self):
§§        return '<SystemsLocation %r>' % self.System
§§

§§class GmOptions(db.Model):
§§    __tablename__ = 'gm_options'
§§    id = db.Column(db.Integer, primary_key = True)
§§    setting = db.Column(db.String(150))
§§    value = db.Column(db.String(150))

    def __repr__(self):
§§        return '<gmOptions %r>' % self.name
§§

class SystemFuzzjobInstances(db.Model):
    __tablename__ = 'system_fuzzjob_instances'
§§    id = db.Column(db.Integer, primary_key = True)
    System = db.Column(ForeignKey('systems.id'))
    Fuzzjob = db.Column(ForeignKey('fuzzjob.id'))
    AgentType = db.Column(db.Integer)
    InstanceCount = db.Column(db.Integer)
    Architecture = db.Column(db.String(10))

    def __repr__(self):
§§        return '<SystemFuzzjobInstance %r>' % self.System
