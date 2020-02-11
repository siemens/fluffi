# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Junes Najah, Thomas Riedmaier

import unittest
from app import app, models

class TestURLs(unittest.TestCase):
    def setUp(self):        
        self.client = app.test_client()
        fuzzjob = models.Fuzzjob.query.first()        
        if fuzzjob and hasattr(fuzzjob, "ID"):
            self.projId = fuzzjob.ID
        else:
            print("Error: No fuzzjob exists for testing!")            

    def test_root_redirect(self):
        """ Tests if the root URL gives a 200 """

        result = self.client.get("/")
        self.assertEqual(result.status_code, 200)  
        
    def test_projects_redirect(self):
        """ Tests if the projects URL gives a 200 """

        result = self.client.get("/projects")
        self.assertEqual(result.status_code, 200)  

    def test_locations_redirect(self):
        """ Tests if the locations URL gives a 200 """

        result = self.client.get("/locations")
        self.assertEqual(result.status_code, 200)  

    def test_commands_redirect(self):
        """ Tests if the commands URL gives a 200 """

        result = self.client.get("/commands")
        self.assertEqual(result.status_code, 200) 

    def test_hangs_redirect(self):
        """ Tests if the hangs URL gives a 200 """

        result = self.client.get("/projects/{}/hangs/1".format(self.projId))
        self.assertEqual(result.status_code, 200)  

    def test_population_redirect(self):
        """ Tests if the population URL gives a 200 """

        result = self.client.get("/projects/{}/population/1".format(self.projId))
        self.assertEqual(result.status_code, 200)  

    def test_violations_redirect(self):
        """ Tests if the violations URL gives a 200 """

        result = self.client.get("/projects/{}/violations".format(self.projId))
        self.assertEqual(result.status_code, 200) 

    def test_view_projects_redirect(self):
        """ Tests if the view projects URL gives a 200 """

        result = self.client.get("/projects/view/{}".format(self.projId))
        self.assertEqual(result.status_code, 200) 

    def test_accessVioTotal_redirect(self):
        """ Tests if the accessVioTotal view URL gives a 200 """

        result = self.client.get("/projects/{}/accessVioTotal/1".format(self.projId))
        self.assertEqual(result.status_code, 200)

    def test_accessVioUnique_redirect(self):
        """ Tests if the accessVioUnique view URL gives a 200 """

        result = self.client.get("/projects/{}/accessVioUnique".format(self.projId))
        self.assertEqual(result.status_code, 200)

    def test_totalCrashes_redirect(self):
        """ Tests if the totalCrashes view URL gives a 200 """

        result = self.client.get("/projects/{}/totalCrashes/1".format(self.projId))
        self.assertEqual(result.status_code, 200) 

    def test_noResponse_redirect(self):
        """ Tests if the noResponse view URL gives a 200 """

        result = self.client.get("/projects/{}/noResponse/1".format(self.projId))
        self.assertEqual(result.status_code, 200) 


if __name__ == '__main__':
    unittest.main()
