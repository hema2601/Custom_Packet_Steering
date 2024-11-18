import json
import sys

if len(sys.argv) >= 2:
    file_name = sys.argv[1] + ".json"
else:
    file_name = "tmp.json"

if len(sys.argv) >= 3:
    type_name = sys.argv[2]
else:
    type_name = "Undefined"


json_list = list() 
count = 0
limit = 2000

core_dict = dict()

while count <= limit:
    with open("/proc/ipi_lat_module", "r") as f:
        for line in f:
            parts = [x for x in line.split(' ') if x.strip()]
            if parts[0] not in core_dict:
                core_dict[parts[0]] = int(parts[1])
                continue
            elif core_dict[parts[0]] == int(parts[1]):
                continue
                
            tmp = dict()
            tmp["CPU"] = int(parts[0])
            tmp["Cycles"] = int(parts[1])
            tmp["Type"] = type_name
            #print(tmp)
            json_list.append(tmp)
    count += 1


with open(file_name, "w") as new_file:
    json.dump(json_list, new_file, indent=0)
