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
# Author(s): Pascal Eckmann, Thomas Riedmaier, Junes Najah

import time

import mysql.connector
import requests

# settings
DEBUG = False  # show log in terminal
MODE = "inactive"  # default mode, get to be overwritten with value of database
# options:  active -> try to keep the desired number of agents
#           inactive -> don't do anything
#           kill -> kill all agents

if DEBUG:
    import json

# dict for agents, which are online
CURRENT = {}
# dict for agents, which are offline
SHOULD = {}

#   structure of CURRENT & SHOULD
#     {
#       [fuzzjob name]: {
#         [unique id]: {
#           [system domain]: {
#             "agentType": [entry of agentTypenD],
#             "location": [room or position, where system is located]
#             "port": [port for communication with agent on system]
#             "status": ["ignore"/"generate"/"kill"]
#           }
#         },
#         ...
#       },
#       ...
#     }


# dict for     system id <-> system name
systems_dic = {}
# dict for    fuzzjob id <-> fuzzjob name
fuzzjobs_dic = {}
# dict for fuzzjob typ id <-> fuzzjob typ name
agent_types_dic = {
    0: "TestcaseGenerator",
    1: "TestcaseRunner",
    2: "TestcaseEvaluator",
    3: "GlobalManager",
    4: "LocalManager"
}
# dict for   location id <-> location name
locations_dic = {}
# dict for     system id <-> location id
systems_location_dic = {}
# dict for unassigned workers
workers_dic = {}


def set_mode(database):
    global MODE
    cur = database.cursor()
    cur.execute("SELECT value FROM gm_options WHERE setting=\'agentstartermode\'")
    row = list(cur)
    if row[0] is not None:
        MODE = "".join(row[0])
    if DEBUG:
        print("MODE: " + MODE)


def add_entries_to_dic(database, command, dictionary):
    cur = database.cursor()
    cur.execute("SELECT * FROM " + command)
    rows = list(cur)
    for row in rows:
        dictionary[row[0]] = row[1]
    cur.close()

    if DEBUG:
        print("=============================\n" + command + "\n\n" + json.dumps(dictionary, indent=1) + "\n")


def add_entries_to_current(database):
    count_c = 0
    CURRENT.update({"0": {}})
    cur = database.cursor()
    cur.execute("SELECT name FROM fuzzjob")
    fuzzjob_names = list(cur)

    # add fuzzjob names to dict
    for name in fuzzjob_names:
        CURRENT.update({name[0]: {}})

    cur.execute("SELECT DBName FROM fuzzjob")
    database_names = list(cur)

    # add all *CURRENT* agents to dict
    for name in database_names:
        fuzzjob_database = str(name[0])
        if DEBUG:
            print("connect to " + fuzzjob_database + " database")
        fuzzjob_database_conn = mysql.connector.connect(
            host="db.fluffi",
            user="fluffi_gm",
            passwd="fluffi_gm",
            database=fuzzjob_database
        )
        cur = fuzzjob_database_conn.cursor()
        cur.execute("SELECT ServiceDescriptorGUID, ServiceDescriptorHostAndPort, AgentType, AgentSubType, Location "
                    "FROM managed_instances")
        instance_rows = list(cur)
        for row in instance_rows:
            system_entry = row[1].split(':')[0]
            agent_typ = agent_types_dic[row[2]]
            CURRENT[fuzzjob_database[7:]][row[0]] = {
                system_entry: {
                    "agentType": agent_typ,
                    "location": row[4],
                    "port": row[1].split(':')[1]
                }
            }
            count_c += 1

    cur = database.cursor()
    cur.execute("SELECT ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, FuzzJob FROM localmanagers")
    localmanagers = list(cur)
    for lm_entry in localmanagers:
        system_entry = lm_entry[1].split(':')[0]
        CURRENT["0"][lm_entry[0]] = {
            system_entry: {
                "agentType": "LocalManager",
                "location": locations_dic[lm_entry[2]],
                "port": lm_entry[1].split(':')[1]
            }
        }
    cur = database.cursor()
    cur.execute("SELECT ServiceDescriptorGUID, ServiceDescriptorHostAndPort, Location, FuzzJob, AgentType FROM workers")
    localmanagers = list(cur)
    for lm_entry in localmanagers:
        if lm_entry[4] == 4:
            system_entry = lm_entry[1].split(':')[0]
            CURRENT["0"][lm_entry[0]] = {
                system_entry: {
                    "agentType": "LocalManager",
                    "location": locations_dic[lm_entry[2]],
                    "port": lm_entry[1].split(':')[1]
                }
            }
    cur.close()
    count_c += len(CURRENT["0"])
    if DEBUG:
        print("\n")
    return count_c


def add_entries_to_should(database):
    cur = database.cursor()
    # connect to each fuzzjob
    cur.execute("SELECT System, Fuzzjob, AgentType, InstanceCount, Architecture FROM system_fuzzjob_instances")
    rows = list(cur)

    for row in rows:
        try:
            if row[1] is not None:
                fuzzjob = fuzzjobs_dic[row[1]]
            else:
                fuzzjob = "0"
            SHOULD.update({fuzzjob: {}})
        except KeyError as e:
            if DEBUG:
                print("#0 ERROR " + str(e) + " " + str(row))
            continue

    row_counter = 0
    # add all *SHOULD* agents to dict
    system = "default"
    agent_typ = 1
    fuzzjob = "default"
    room = "default"

    for row in rows:
        for rounds in range(0, row[3]):
            for x in range(0, 5):
                try:
                    if x == 0:
                        system = systems_dic[row[x]]
                        room = locations_dic[systems_location_dic[row[x]]]
                    if x == 1:
                        if row[x] is not None:
                            fuzzjob = fuzzjobs_dic[row[x]]
                    if x == 2:
                        agent_typ = agent_types_dic[row[x]]
                except KeyError as e:
                    if DEBUG:
                        print("#1 ERROR " + str(e) + " " + str(row))
                    continue

            system_entry = system + ".fluffi"
            if agent_typ == "LocalManager":
                fuzzjob = "0"
            SHOULD[fuzzjob][row_counter] = {
                system_entry: {
                    "agentType": agent_typ,
                    "location": room,
                    "port": "0",
                    "architecture": row[4]
                }
            }
            row_counter += 1

    cur.close()
    return row_counter


def check_current_with_should():
    count_i = 0
    for fuzzjob_s, fuzzjob_agents_s in SHOULD.items():
        for agent_id_s, agent_systems_s in fuzzjob_agents_s.items():
            if fuzzjob_s in CURRENT:
                for agent_id_i, agent_systems_i in CURRENT[fuzzjob_s].items():
                    if list(agent_systems_s.keys())[0] == list(agent_systems_i.keys())[0]:
                        system = list(agent_systems_s.keys())[0]
                        if ((agent_systems_s[system]['agentType'] == agent_systems_i[system]['agentType']) and
                                (agent_systems_s[system]['location'] == agent_systems_i[system]['location']) and
                                "status" not in agent_systems_s[system] and "status" not in agent_systems_i[system]):
                            CURRENT[fuzzjob_s][agent_id_i][system]["status"] = "ignore"
                            SHOULD[fuzzjob_s][agent_id_s][system]["status"] = "ignore"
                            SHOULD[fuzzjob_s][agent_id_s][system]["port"] = \
                                CURRENT[fuzzjob_s][agent_id_i][system]["port"]
                            SHOULD[fuzzjob_s][agent_id_s][system]["guid"] = agent_id_i
                            count_i += 1
                            break
    return count_i


def generate_agents():
    count_g = 0
    count_lm = 0
    for fuzzjob, fuzzjob_agents in SHOULD.items():
        for agent_id, agent_systems in fuzzjob_agents.items():
            for system, agent_info in agent_systems.items():
                if "status" not in agent_systems[system]:
                    SHOULD[fuzzjob][agent_id][system]["status"] = "generate"
                if SHOULD[fuzzjob][agent_id][system]["status"] == "generate" and len(
                        agent_systems[system]["architecture"]) > 0:
                    check = generate_agent_with_architecture(system, agent_systems[system]["architecture"],
                                                             agent_systems[system]["agentType"],
                                                             agent_systems[system]["location"])
                    if check:
                        count_g += 1
                        if agent_systems[system]["agentType"] == "LocalManager":
                            count_lm += 1
                    else:
                        SHOULD[fuzzjob][agent_id][system]["status"] = "generate failed"
                        count_g += 1
                        if agent_systems[system]["agentType"] == "LocalManager":
                            count_lm += 1
    return [count_g, count_lm]


def generate_agent_with_architecture(system, architecture, agent_type, location):
    count_err = 0
    check = False
    if "dev-" in system:
        system = system[4:]
    url = "http://" + system + ":9000/start?cmd=" + architecture + "/" + agent_type + " " + location
    while True:
        try:
            if DEBUG:
                print("GENERATE   # " + url)
            response = requests.get(url)
            if response.status_code == 200:
                check = True
                if DEBUG:
                    print("SUCCESS " + str(response.status_code) + "# " + agent_type + " on " + system + " / "
                          + location + " with architecture: " + str(architecture) + ": " + str(response.json())
                          + "\n")
            else:
                if DEBUG:
                    print("ERROR   " + str(response.status_code) + "# " + agent_type + " on " + system + " / "
                          + location + " with architecture: " + str(architecture) + ": " + str(response.json())
                          + "\n")
            break
        except Exception as e:
            if DEBUG:
                print("#2 ERROR retry request: " + url + "\n" + str(e) + "\n")
            time.sleep(1)
            count_err += 1
            if count_err >= 1:
                check = False
                break
    return check


def kill_agents(database):
    count_k = 0
    for fuzzjob, fuzzjob_agents in CURRENT.items():
        for agent_id, agent_systems in fuzzjob_agents.items():
            for system, agent_info in agent_systems.items():
                if "status" not in agent_systems[system]:
                    CURRENT[fuzzjob][agent_id][system]["status"] = "kill"
                if CURRENT[fuzzjob][agent_id][system]["status"] == "kill":
                    count_k += 1
                    cur = database.cursor()
                    sql = "INSERT INTO command_queue (Command, Argument, Done) VALUES ('kill', '" + system + ":" + \
                          CURRENT[fuzzjob][agent_id][system]["port"] + "', 0);"
                    if DEBUG:
                        print("KILL       $ " + sql)
                    cur.execute(sql)
                    cur.close()
                    database.commit()
    return count_k


def assign_agents_to_fuzzjobs(database):
    count_a = 0
    cur = database.cursor()
    cur.execute("SELECT * FROM workers")
    rows = cur.fetchall()

    for row in rows:
        none_type = type(None)
        if isinstance(row[2], none_type):
            fuzzjob = "None"
        else:
            fuzzjob = row[2]
        workers_dic.update({
            row[0]: {
                "host": row[1].split(':')[0],
                "port": row[1].split(':')[1],
                "fuzzjob": fuzzjob,
                "location": locations_dic[row[3]],
                "agentType": agent_types_dic[row[5]],
            }
        })

    for fuzzjob, fuzzjob_agents in SHOULD.items():
        for agent_id, agent_systems in fuzzjob_agents.items():
            for system, agent_info in agent_systems.items():
                if SHOULD[fuzzjob][agent_id][system]["status"] == "generate":
                    for sd_guid, agent in workers_dic.items():
                        system_w = agent["host"]
                        port_w = agent["port"]
                        fuzzjob_w = agent["fuzzjob"]
                        location_w = agent["location"]
                        agent_type_w = agent["agentType"]

                        if ((fuzzjob_w is "None") and
                                (system.lower() == system_w.lower()) and
                                (agent_info["agentType"] == agent_type_w) and
                                (agent_info["location"] == location_w)):
                            workers_dic[sd_guid]["fuzzjob"] = fuzzjob
                            for fuzzjob_id, fuzzjob_dic_elem in fuzzjobs_dic.items():
                                if fuzzjob_dic_elem == fuzzjob:
                                    SHOULD[fuzzjob][agent_id][system]["port"] = port_w
                                    SHOULD[fuzzjob][agent_id][system]["status"] = "assign to " + sd_guid
                                    sql = "UPDATE workers SET FuzzJob = " + str(
                                        fuzzjob_id) + " WHERE ServiceDescriptorGUID = '" + sd_guid + "';"
                                    if DEBUG:
                                        print("UPDATE     $ " + sql)
                                    cur.execute(sql)
                                    database.commit()
                                    count_a += 1
                                    break
                            break

    cur.close()
    return count_a


def main():
    count_a = 0
    count_c = 0
    count_s = 0
    count_i = 0
    count_g_and_lm = [0, 0]
    count_k = 0

    database = mysql.connector.connect(
        host="db.fluffi",
        user="fluffi_gm",
        passwd="fluffi_gm",
        database="fluffi_gm"
    )
    database.autocommit = True

    if DEBUG:
        print("connect to fluffi_gm database\n")

    set_mode(database)

    if "active" == MODE or "kill" == MODE:
        add_entries_to_dic(database, "systems", systems_dic)
        add_entries_to_dic(database, "fuzzjob", fuzzjobs_dic)
        add_entries_to_dic(database, "locations", locations_dic)
        add_entries_to_dic(database, "systems_location", systems_location_dic)

        count_c = add_entries_to_current(database)

        if "active" == MODE:
            count_s = add_entries_to_should(database)
            count_i = check_current_with_should()
            count_g_and_lm = generate_agents()

        count_k = kill_agents(database)

        if DEBUG:
            print("\nSLEEPING 5s\n")
        time.sleep(5)

        count_a = assign_agents_to_fuzzjobs(database)

    if DEBUG:
        print("\n\nCURRENT\n=============================\n\n" + json.dumps(CURRENT, indent=4)
              + "\n\nSHOULD\n=============================\n\n" + json.dumps(SHOULD, indent=4)
              + "\n\nWORKERS\n=============================\n\n" + json.dumps(workers_dic, indent=4) + "\n\n")

    print("\n#   mode      : " + MODE
          + "\n#   current   : " + str(count_c)
          + "\n#   should    : " + str(count_s)
          + "\n#   ignored   : " + str(count_i)
          + "\n#   killed    : " + str(count_k)
          + "\n#   generated : " + str(count_g_and_lm[0]) + " / LM: " + str(count_g_and_lm[1])
          + "\n#   assigned  : " + str(count_a))

    CURRENT.clear()
    SHOULD.clear()


if __name__ == "__main__":
    main()
