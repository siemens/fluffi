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
# Author(s): Fabian Russwurm, Pascal Eckmann, Michael Kraus, Thomas Riedmaier, Junes Najah

import requests, json


class AnsibleRESTConnector:

    def __init__(self, ansibleURL, username, password):
        self.SHOWN_GROUPS = {"windows", "linux", "odroids"}
        self.ansibleURL = ansibleURL
        self.auth = (username, password)

    def getFluffiInventoryID(self):
        url = self.ansibleURL + "inventory/"
        response = requests.get(url, auth=self.auth)
        jsonResults = json.loads(response.text)
        inventoryId=-1
        for result in jsonResults['results']:
            if result['name'].lower() == "fluffi":
                inventoryId = result['id']
        return inventoryId

    def getFluffiProjectURL(self):
        url = self.ansibleURL + "project/"
        response = requests.get(url, auth = self.auth)
        jsonResults = json.loads(response.text)
        for result in jsonResults['results']:
            if result['name'].lower() == "fluffi":
                return url + str(result['id']) + "/"

    def checkFluffiInventory(self):
        url = self.ansibleURL + "inventory/"
        response = requests.get(url, auth = self.auth)
        jsonResults = json.loads(response.text)
        for result in jsonResults['results']:
            if result['name'].lower() == "fluffi":
                return True
        return False

    def executePlaybook(self, playbookName, limit, arguments = None):
        inventoryID = self.getFluffiInventoryID()
        fluffiProjectURL = self.getFluffiProjectURL()
        fluffiProjectURL+="execute_playbook/"
        data = {}
        data['inventory'] = str(inventoryID)
        data['playbook'] = playbookName
        data['limit'] = limit
        if arguments is not None:
            extraVarsString = ""
            for key, value in arguments.items():
                extraVarsString += key + "=" + value + " "
            extraVarsString.strip()
            data['extra_vars'] = extraVarsString
        # Sending post request to execute a playbook
        response = requests.post(fluffiProjectURL, json = data, auth = self.auth)
        jsonResult = json.loads(response.text)
        if 'history_id' in jsonResult:
            historyID = jsonResult['history_id']
            return "http://" + self.ansibleURL.split("/")[2] + "/?history/" + str(historyID)
        else:
            return None

    def getSystemObjectByName(self, hostname):
        system = type('', (), {})()
        system.Name=hostname
        if system.Name not in self.SHOWN_GROUPS:
            system.IsGroup=False
        else:
            system.IsGroup=True

        return system

    def getSystems(self):
        systems = []
        url = self.ansibleURL + "group/"
        try:
            # Sending post request to execute playbook to add new system
            response = requests.get(url, auth=self.auth)
            jsonResult = json.loads(response.text)
            for entry in jsonResult['results']:
                if entry['name'] in self.SHOWN_GROUPS:
                    url = self.ansibleURL + "group/" + str(entry['id']) + "/host/"
                    response = requests.get(url, auth=self.auth)
                    jsonResult = json.loads(response.text)
                    for host in jsonResult['results']:
                        systems.append((host['name'], host['id']))
        except Exception as e:
            print(e)
        finally:
            return systems

    def getSystemsOfGroup(self, group):
        systems = []
        url = self.ansibleURL + "group/"

        try:
            # Sending post request to execute playbook to add new system
            response = requests.get(url, auth=self.auth)
            jsonResult = json.loads(response.text)
            for entry in jsonResult['results']:
                if entry['name'] == group:
                    url = self.ansibleURL + "group/" + str(entry['id']) + "/host/"
                    response = requests.get(url, auth=self.auth)
                    jsonResult = json.loads(response.text)
                    for entry in jsonResult['results']:
                        systems.append(entry['name'])

        except Exception as e:
            print("Exception: Cannot list hosts. Check group name.", str(e))
        finally:
            return systems

    # calls Polemarch REST API to add a new linked system to fluffi network
    # must assign to a group (linux or windows)
    def addNewSystem(self, hostname, group):
        url = self.ansibleURL + "group/"+group+"/host/"
        data = {}
        data['name'] = hostname
        data['type'] = "HOST"

        result = ""

        try:   
            # Sending post request to execute playbook to add new system
            response = requests.post(url, json = data, auth = self.auth)
            jsonResult = json.loads(response.text)

            # get id of newly created host
            newHostId = jsonResult['id']

            # Add variable to host
            url = self.ansibleURL + "group/"+group+"/host/"+str(newHostId)+"/variables/"
            data = {}
            data['key'] = "ansible_ssh_host"
            data['value'] = hostname + ".fluffi"
            response = requests.post(url, json = data, auth=self.auth)
            jsonResult = json.loads(response.text)

        except Exception as e:
            print("Exception: Cannot add new host. Check host name.")
            return False
        
        return True

    # calls Polemarch REST API to remove a system from fluffi network
    def removeSystem(self, hostName):
        systems = self.getSystems()
            
        url = ""
        for system in systems:
            if system[0] == hostName:
                url = self.ansibleURL + "host/" + str(system[1])
                break
            
        if url != "":
            try:
                response = requests.delete(url, auth = self.auth)
                return True
            except Exception as e:
                print(e)
                return False
        else:
            return False

    def execHostAlive(self):
        fluffiProjectURL = self.getFluffiProjectURL()
        fluffiPeriodicTasksURL = fluffiProjectURL + "periodic_task/"
        response = requests.get(fluffiPeriodicTasksURL, auth=self.auth,
                                headers={'User-Agent': 'Python', 'Connection': 'close'})
        jsonResults = json.loads(response.text)

        taskId = '0'
        for result in jsonResults['results']:
            if result['name'] == 'checkHostsAlive':
                taskId = str(result['id'])
                break

        if taskId != '0':
            execCheckHostAliveURL = fluffiPeriodicTasksURL + taskId + '/execute/'
            try:
                response = requests.post(execCheckHostAliveURL, auth=self.auth)
                return True
            except Exception as e:
                return False
        else:
            return False

    def getHostAliveState(self):
        url = self.ansibleURL + "history/?mode=checkHostAlive.yml"
        requests.session().close()
        response = requests.get(url, auth=self.auth, headers = {'User-Agent':'Python', 'Connection':'close'}) # Should already be sorted
        jsonResults = json.loads(response.text)
        lastHostCheckResultURL = ""
        for result in jsonResults['results']:
            if result['status'].lower() not in {"run", "running", "delay"}: # --> filter out some states, because no result available
                lastHostCheckResultURL = self.ansibleURL + "history/" + str(result['id']) + "/"
                break

        response = requests.get(lastHostCheckResultURL, auth=self.auth, headers = {'User-Agent':'Python', 'Connection':'close'})
        jsonResults = json.loads(response.text)
        getResultURL = jsonResults['raw_stdout']  # --> get result
        response = requests.get(getResultURL, auth = self.auth, headers = {'User-Agent':'Python', 'Connection':'close'})

        hosts = []
        resultHostData = response.text.split("PLAY")
        aliveProtocol = 0
        for p in resultHostData:
            if "RECAP" in p:
                break
            aliveProtocol += 1
        for i, line in enumerate(resultHostData[aliveProtocol].split("\n")):
            line = line.strip()
            parts = line.split()
            host = type('', (), {})()
            if i > 0 and len(line)>2:
                host.Id = i
                host.Name = parts[0]
                if "1" in parts[2]:
                    host.Ok = True
                else:
                    host.Ok = False
                if "1" in parts[4]:
                    host.Unreachable = True
                else:
                    host.Unreachable = False            
                if "1" in parts[5]:
                    host.Failed = True
                else:
                    host.Failed = False 
                hosts.append(host)
            
        url = self.ansibleURL + 'group/'
        res = requests.get(url, auth = self.auth, headers = {'User-Agent':'Python', 'Connection':'close'})
        groups = []
        if res.ok:
            data = res.json()
            for s in data['results']:
                group = type('', (), {})()
                group.Id = s['id']
                group.Name = s['name']
                group.URL = self.ansibleURL + 'group/' + str(group.Id) + "/host/"
                group.hosts=[]
                if group.Name.lower() not in self.SHOWN_GROUPS:
                    continue
                groups.append(group)
                url = group.URL
                resHosts = requests.get(url, auth = self.auth, headers = {'User-Agent':'Python', 'Connection':'close'})
                if resHosts.ok:
                    dataHosts = resHosts.json()
                    for h in dataHosts['results']:
                        host = type('', (), {})()
                        host.Id = h['id']
                        host.Name = h['name']
                        host.Type = h['type']
                        host.URL = self.ansibleURL + 'host/' + str(host.Id)
                        host.Location = ""
                        for singleHost in hosts:
                            if singleHost.Name == host.Name:
                                if singleHost.Ok == True:
                                    host.Status = "OK"
                                elif singleHost.Unreachable == True:
                                    host.Status = "Unreachable"
                                else:
                                    host.Status = "Failed"
                        group.hosts.append(host)
                resHosts.close()
            response.close()
            res.close()
            requests.session().close()
            return groups
