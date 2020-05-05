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
# Author(s): Junes Najah, Michael Kraus, Fabian Russwurm, Abian Blome, Thomas Riedmaier, Pascal Eckmann

from flask_wtf import FlaskForm
from flask_wtf.file import FileField, FileRequired
from wtforms import FieldList, FormField, IntegerField, SelectField, StringField, SubmitField
from wtforms.validators import DataRequired, NumberRange


class AddLocationProjectForm(FlaskForm):
    project = SelectField('Project', choices = [])


class TargetForm(FlaskForm):
    # this forms is never exposed so we can user the non CSRF version
    target_moduleC = StringField('Target Module', validators = [DataRequired()])
    target_module_pathC = StringField('Module Path')


class OptionForm(FlaskForm):
    # this forms is never exposed so we can user the non CSRF version
    option_moduleC = StringField('Option Module', validators = [DataRequired()])
    option_module_pathC = StringField('Module Value')


class CreateProjectForm(FlaskForm):
    name = StringField("name", validators = [DataRequired()])
    location = SelectField('location', choices = [])
    subtype = SelectField('subtype', choices = [])
    targetCMDLine = StringField("targetCMDLine", validators = [DataRequired()])
    hangTimeout = StringField("hangTimeout", validators = [DataRequired()])
    target_module = StringField("target_module", validators = [DataRequired()])
    target_module_path = StringField("target_module_path", validators = [DataRequired()])
    option_module = StringField("target_module", validators = [DataRequired()])
    option_module_value = StringField("target_module_path", validators = [DataRequired()])
    filename = FileField(validators = [FileRequired()])
    targetfile = FileField(validators = [FileRequired()])
    basicBlockFile = FileField()
    targets = FieldList(FormField(TargetForm))
    options = FieldList(FormField(OptionForm))


class CreateCustomProjectForm(FlaskForm):
    name = StringField("name", validators = [DataRequired()])
    DBHost = StringField("DBHost", validators = [DataRequired()])
    DBUser = StringField("DBUser", validators = [DataRequired()])
    DBPass = StringField("DBPass", validators = [DataRequired()])
    DBName = StringField("DBName", validators = [DataRequired()])


class CreateProjectSettingForm(FlaskForm):
    option_module = StringField("option_module", validators = [DataRequired()])
    option_module_value = StringField("option_module_value", validators = [])


class CreateProjectModuleForm(FlaskForm):
    target_module = StringField("target_module", validators = [DataRequired()])
    target_module_path = StringField("target_module_path", validators = [], default = "*")


class AddProjectLocationForm(FlaskForm):
    location = SelectField('location', choices = [])


class CreateLocationForm(FlaskForm):
    name = StringField("name", validators = [DataRequired()])


class ExecuteInitialSetupForm(FlaskForm):
    ram_disk_size = IntegerField(label = 'Size of Ramdisk: ', default = 1024,
                                 validators = [DataRequired(), NumberRange(min = 512, max = 128000)])
    execute_initial_submit = SubmitField('Start Setup')


class SyncToRamdiskForm(FlaskForm):
    execute_sync_ramdisk = SubmitField('Syncronize')


class ExecuteDeployFluffiForm(FlaskForm):
    architecture = SelectField(label = "Architecture: ", choices = [])
    execute_deploy_fluffi = SubmitField('Deploy')


class ExecuteDeployInstallPackageForm(FlaskForm):
    installPackage = SelectField(label = "Select Package: ", choices = [])
    execute_deploy_package = SubmitField('Deploy')


class ExecuteDeployFuzzjobInstallPackageForm(FlaskForm):
    fuzzingJob = SelectField(label = "Select Fuzzjob: ", choices = [])
    execute_deploy_package = SubmitField('Deploy')


class StartFluffiComponentForm(FlaskForm):
    component = SelectField(label = "Select Component: ",
                            choices = [("0", "Testcase Generator"), ("1", "Testcase Runner"),
                                       ("2", "Testcase Evaluator"),
                                       ("4", "Local Manager")])
    architecture = SelectField(label = "Architecture: ", choices = [])
    numberOfComponents = IntegerField(label = 'Number: ', default = 1, validators = [DataRequired()])
    execute_start_component = SubmitField('Start Additional Instances')


class AddNewSystemForm(FlaskForm):
    hostname = StringField("Hostname:", validators = [DataRequired()])
    hostgroup = SelectField(label = "Select group: ", choices = [("1", "windows"), ("2", "linux"), ("3", "odroids")])
    add_host = SubmitField('Add')


class ChangeAgentStarterModeForm(FlaskForm):
    mode = SelectField(label = "Select mode: ", choices = [("active", "ACTIVE"), ("inactive", "INACTIVE"), ("kill", "KILL")])
    execute_change_mode = SubmitField('Change')


class ChangePXEForm(FlaskForm):
    pxesystem = SelectField(label = "PXE System: ", choices = [])
    execute_change_pxe = SubmitField('Change')


class InstanceConfigForm(FlaskForm):
    instance_count = StringField('Instance Count')


class InstanceArchConfigForm(FlaskForm):
    architecture = SelectField(label = "Architecture: ", choices = [])


class SystemInstanceConfigForm(FlaskForm):
    instances = FieldList(FormField(InstanceConfigForm))
    architectures = FieldList(FormField(InstanceArchConfigForm))
