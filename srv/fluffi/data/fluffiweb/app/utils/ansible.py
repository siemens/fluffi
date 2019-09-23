§§# Copyright 2017-2019 Siemens AG
§§# 
§§# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§# 
§§# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§# 
§§# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§# 
§§# Author(s): Fabian Russwurm, Michael Kraus, Pascal Eckmann, Thomas Riedmaier, Junes Najah
§§
§§import requests, json
§§
§§class AnsibleRESTConnector:
§§
§§    def __init__(self, ansibleURL, username, password):
§§        self.SHOWN_GROUPS = {"windows", "linux", "odroids"}
§§        self.ansibleURL = ansibleURL
§§        self.auth = (username, password)
§§
§§    def getFluffiInventoryID(self):
§§        url = self.ansibleURL + "inventory/"
§§        response = requests.get(url, auth=self.auth)
§§        jsonResults = json.loads(response.text)
§§        inventoryId=-1
§§        for result in jsonResults['results']:
§§            if result['name'].lower() == "fluffi":
§§                inventoryId = result['id']
§§        return inventoryId
§§
§§    def getFluffiProjectURL(self):
§§        url = self.ansibleURL + "project/"
§§        response = requests.get(url, auth = self.auth)
§§        jsonResults = json.loads(response.text)
§§        for result in jsonResults['results']:
§§            if result['name'].lower() == "fluffi":
§§                return url + str(result['id']) + "/"
§§
§§    def executePlaybook(self, playbookName, limit, arguments = None):
§§        inventoryID = self.getFluffiInventoryID()
§§        fluffiProjectURL = self.getFluffiProjectURL()
§§        fluffiProjectURL+="execute_playbook/"
§§        data = {}
§§        data['inventory'] = str(inventoryID)
§§        data['playbook'] = playbookName
§§        data['limit'] = limit
§§        if arguments is not None:
§§            extraVarsString = ""
§§            for key, value in arguments.items():
§§                extraVarsString += key + "=" + value + " "
§§            extraVarsString.strip()
§§            data['extra_vars'] = extraVarsString
§§        # Sending post request to execute a playbook
§§        response = requests.post(fluffiProjectURL, json = data, auth = self.auth)
§§        print(str(data))
§§        jsonResult = json.loads(response.text)
§§        if 'history_id' in jsonResult:
§§            historyID = jsonResult['history_id']
§§            return "http://" + self.ansibleURL.split("/")[2] + "/?history/" + str(historyID)
§§        else:
§§            return None
§§        print(str(jsonResult))
§§
§§    def getSystemObjectByName(self, hostname):
§§        system = type('', (), {})()
§§        system.Name=hostname
§§        if system.Name not in self.SHOWN_GROUPS:
§§            system.IsGroup=False
§§        else:
§§            system.IsGroup=True
§§
§§        return system
§§
§§    # calls Polemarch REST API to add a new linked system to fluffi network
§§    # must assign to a group (linux or windows)
§§    def addNewSystem(self, hostname, group):
§§        url = self.ansibleURL + "group/"+group+"/host/"
§§        data = {}
§§        data['name'] = "dev-" + hostname
§§        data['type'] = "HOST"
§§
§§        result = ""
§§
§§        try:   
§§            # Sending post request to execute playbook to add new system
§§            response = requests.post(url, json = data, auth = self.auth)
§§            jsonResult = json.loads(response.text)
§§            print(jsonResult)
§§
§§            # get id of newly created host
§§            newHostId = jsonResult['id']
§§
§§            # Add variable to host
§§            url = self.ansibleURL + "group/"+group+"/host/"+str(newHostId)+"/variables/"
§§            data = {}
§§            data['key'] = "ansible_ssh_host"
§§            data['value'] = hostname + ".fluffi"
§§            response = requests.post(url, json = data, auth=self.auth)
§§            jsonResult = json.loads(response.text)
§§
§§        except Exception as e:
§§            print("Exception: Cannot add new host. Check host name.")
§§            return False
§§        
§§        return True
§§
§§    # calls Polemarch REST API to remove a self created dev system to fluffi network
§§    def removeDevSystem(self, hostId):
§§        url = self.ansibleURL + "host/" + hostId + "/"
§§        data = {}
§§
§§        try:                                              
§§            # Sending post request to execute playbook to add new system
§§            response = requests.delete(url, json = data, auth = self.auth)
§§            return True            
§§            
§§        except Exception as e:
§§            return False
§§
§§    def getHostAliveState(self):
§§        #self.executePlaybook("checkHostAlive.yml", "all")
§§        url = self.ansibleURL + "history/?mode=checkHostAlive.yml"
§§        requests.session().close()
§§        response = requests.get(url, auth=self.auth, headers={'Connection':'close'}) # Should already be sorted
§§        jsonResults = json.loads(response.text)
§§        lastHostCheckResultURL = ""
§§        for result in jsonResults['results']:
§§            if result['status'].lower() not in {"run", "running", "delay"}: # --> filter out some states, because no result available
§§                lastHostCheckResultURL = self.ansibleURL + "history/" + str(result['id'])
§§                break
§§
§§        response = requests.get(lastHostCheckResultURL, auth=self.auth)
§§        jsonResults = json.loads(response.text)
§§        getResultURL = jsonResults['raw_stdout']# --> get result
§§        response = requests.get(getResultURL, auth = self.auth, headers = {'User-Agent':'Python', 'Connection':'close'})
§§
§§        hosts = []
§§        resultHostData = response.text.split("PLAY")
§§        aliveProtocol = 0
§§        for p in resultHostData:
§§            if "RECAP" in p:
§§                break
§§            aliveProtocol+=1
§§        for i, line in enumerate(resultHostData[aliveProtocol].split("\n")):
§§            line = line.strip()
§§            parts = line.split()
§§            host = type('', (), {})()
§§            if i > 0 and len(line)>2:
§§                host.Id = i
§§                host.Name = parts[0]
§§                if "1" in parts[2]:
§§                    host.Ok = True
§§                else:
§§                    host.Ok = False
§§                if "1" in parts[4]:
§§                    host.Unreachable = True
§§                else:
§§                    host.Unreachable = False            
§§                if "1" in parts[5]:
§§                    host.Failed = True
§§                else:
§§                    host.Failed = False 
§§                hosts.append(host)
§§            
§§        url = self.ansibleURL + 'group/'
§§        res = requests.get(url, auth = self.auth, headers = {'Connection':'close'})
§§        groups = []
§§        if res.ok:
§§            data = res.json()
§§            for s in data['results']:
§§                group = type('', (), {})()
§§                group.Id = s['id']
§§                group.Name = s['name']
§§                group.URL = self.ansibleURL + 'group/' + str(group.Id) + "/host/"
§§                group.hosts=[]
§§                if group.Name.lower() not in self.SHOWN_GROUPS:
§§                    continue
§§                groups.append(group)
§§                url = group.URL
§§                resHosts = requests.get(url, auth = self.auth, headers = {'Connection':'close'})
§§                if resHosts.ok:
§§                    dataHosts = resHosts.json()
§§                    for h in dataHosts['results']:
§§                        host = type('', (), {})()
§§                        host.Id = h['id']
§§                        host.Name = h['name']
§§                        host.Type = h['type']
§§                        host.URL = self.ansibleURL + 'host/' + str(host.Id)
§§                        host.Location = ""
§§                        for singleHost in hosts:
§§                            if singleHost.Name == host.Name:
§§                                if singleHost.Ok == True:
§§                                    host.Status = "OK"
§§                                elif singleHost.Unreachable == True:
§§                                    host.Status = "Unreachable"
§§                                else:
§§                                    host.Status = "Failed"
§§                        group.hosts.append(host)
§§            response.close()
§§            res.close()
§§            resHosts.close()
§§            requests.session().close()
§§            return groups
