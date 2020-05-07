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
# Author(s): Junes Najah, Thomas Riedmaier, Abian Blome, Pascal Eckmann, Michael Kraus

from flask_nav import Nav
from flask_nav.elements import Link, Navbar, Separator, Subgroup, View

from app import db, models

nav = Nav()


def createLocationsNav(listLinks):
    locations = []
    
    try:
        locations = db.session.query(models.Locations).distinct(models.Locations.Name)
        
        for location in locations:
            listLinks.append(Link(location.Name, '/locations/view/' + str(location.ID)))
             
    except Exception as e:
        print("Please check your database connection and make sure the hostname db.fluffi is available with user fluffi_gm. " + str(e))    

    return listLinks


def createFuzzjobsNav(listLinks):
    projects = []
    
    try:
        projects = models.Fuzzjob.query.all()
        
        for project in projects:
            listLinks.append(Link(project.name[:15], '/projects/view/%d' % project.ID))
        
    except Exception as e:
        print("Please check your database connection and make sure the hostname db.fluffi is available with user fluffi_gm. " + str(e))    

    return listLinks


def createLocationsLinks():
    locationsLinks = [Link('Overview', '/locations'), Separator(), Link('Create Location', '/locations/createLocation'),
                      Separator()]

    return createLocationsNav(locationsLinks)


def createFuzzjobLinks():
    fuzzjobLinks = [Link('Overview', '/projects'), Separator(), Link('Create Fuzzjob', '/projects/createProject'),
                    Separator()]
    return createFuzzjobsNav(fuzzjobLinks)


def registerElementDynamically():
    locationsLinks = createLocationsLinks()
    fuzzjobLinks = createFuzzjobLinks()
    nav.register_element('frontend_top', Navbar(
        View('FLUFFI', '.index'),
        View('Home', '.index'),
        Subgroup('Locations', *locationsLinks),
        Subgroup('Fuzzjobs', *fuzzjobLinks),
        View('LM Logs', '.logs'),
        View('Commands', '.commands'),
        View('Systems', '.systems')
    ))
