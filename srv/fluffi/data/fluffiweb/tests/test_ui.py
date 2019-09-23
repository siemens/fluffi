§§# Copyright 2017-2019 Siemens AG
§§# 
§§# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§# 
§§# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§# 
§§# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§# 
§§# Author(s): Junes Najah, Thomas Riedmaier
§§
§§import unittest, os, shutil
§§from selenium import webdriver
§§from app import app, models
§§
§§class TestUI(unittest.TestCase):
§§    def setUp(self):
§§        self.driver = webdriver.Firefox()
§§        fuzzjob = models.Fuzzjob.query.first()
§§        if fuzzjob:
§§            self.projId = fuzzjob.id
§§        else:
§§            print("Error: No fuzzjob exists for testing!")  
§§
§§    def tearDown(self):
§§        self.driver.close()
§§    
§§    def test_create_project_and_remove(self):
§§        """ Tests if the create project form works """
§§
§§        testProject="testProject"
§§
§§        self.driver.get("http://localhost:5000/projects/createProject")
§§
§§        project_name_field = self.driver.find_element_by_name("name")
§§        project_name_field.send_keys(testProject)
§§
§§        targetCMDLine_field = self.driver.find_element_by_name("targetCMDLine")
§§        targetCMDLine_field.send_keys("C:testCMDLine")
§§        
§§        target_module_field = self.driver.find_element_by_name("1_targetname")
§§        target_module_field.send_keys("example.dll")
§§
§§        population_file = self.driver.find_element_by_name("filename")
§§        population_file.send_keys("C:\\TestDev\\test_files\\example1.dll")
§§
§§        login_button = self.driver.find_element_by_id("fuzzButton")
§§        login_button.click()
§§
§§        self.driver.get("http://localhost:5000/projects")
§§        self.assertIn(testProject, self.driver.page_source)      
§§
§§        # needs connection to FTP!
§§        archive_btn = self.driver.find_element_by_id(testProject)  
§§        archive_btn.click()
§§        self.assertNotIn(testProject, self.driver.page_source)
§§            
§§
§§    def test_update_and_delete(self):
§§        """ Tests if the update and delete works """
§§
§§        testName="testName"
§§        testValue="testValue"
§§
§§        self.driver.get("http://localhost:5000/projects/view/{}".format(self.projId))    
§§
§§        update_name_field = self.driver.find_element_by_id("option_module")      
§§        update_name_field.send_keys(testName)
§§
§§        update_value_field = self.driver.find_element_by_id("option_module_value")      
§§        update_value_field.send_keys(testValue)
§§
§§        add_settings_button = self.driver.find_element_by_id("addSettings")
§§        add_settings_button.click()
§§
§§        self.assertIn(testName, self.driver.page_source)  
§§        self.assertIn(testValue, self.driver.page_source)  
§§
§§        delete_setting_btn = self.driver.find_element_by_id("deleteSetting{}{}".format(testName, self.projId))
§§        delete_setting_btn.click()
§§
§§        self.assertNotIn(testName, self.driver.page_source)  
§§        self.assertNotIn(testValue, self.driver.page_source) 
§§
§§        update_name_field = self.driver.find_element_by_id("1_targetname")      
§§        update_name_field.send_keys(testName)        
§§
§§        add_target_modules_btn = self.driver.find_element_by_id("addTargetModules")
§§        add_target_modules_btn.click()
§§
§§        self.assertIn(testName, self.driver.page_source)  
§§
§§        delete_setting_btn = self.driver.find_element_by_id("deleteModule{}{}".format(testName, self.projId))
§§        delete_setting_btn.click()
§§
§§        self.assertNotIn(testName, self.driver.page_source)        
§§
§§
§§if __name__ == '__main__':
§§    unittest.main()
