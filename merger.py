import json
import os
import sys


reps = int(sys.argv[1])
conns_pow_of_2 = int(sys.argv[2])
if len(sys.argv) > 3:
    base_dir = sys.argv[3]
else:
    base_dir = "."
#bases = ["RSS", "RPS", "RFS", "Custom"]
bases = ["RSS", "RPS", "RFS", "RSS+RPS", "RSS+RFS", "Custom", "Custom1", "Custom2", "Custom3", "RSS+Custom", "RSS+Custom1", "RSS+Custom2", "RSS+Custom3"]
directory = []

exp = 1
tmp = []
for i in range(conns_pow_of_2):
    for base in bases:
        tmp.append(base+"_"+str(exp))
    exp *= 2

for i in range(reps):
    for curr_dir in tmp:
        directory.append(curr_dir+"_"+str(i+1))




files = ["iperf.json", "irq.json", "packet_cnt.json", "softirq.json", "softnet.json", "pkt_steer.json", "latency.json", "proc_stat.json", "perf.json"]

os.mkdir("./summaries")


for f in files:
    
    new_dict = list()
    
    # Create new file
    file_name = "summary_"+f


    with open("./summaries/"+file_name, "w") as file:

        for exp in directory:
            if os.path.isfile("./data/" + base_dir + "/" + exp+"/"+f) is False:
                continue
            with open("./data/" + base_dir + "/"+exp+"/"+f) as json_file:
                print(exp + " " + f)
                d = json.load(json_file)
                for elem in d:
                    #print(elem)
                    elem["Exp"]=exp.split("_")[0]
                    elem["Conns"]=exp.split("_")[1]
                    elem["Rep"]=exp.split("_")[2]
                    new_dict.append(elem)

        json.dump(new_dict, file, indent=0)




new_json = dict()

for curr_dir in directory:
    acc = curr_dir.rpartition("_")[0]
    new_json[acc] = dict()
print(new_json)

with open("./summaries/summary_packet_cnt.json") as f:
    d = json.load(f)

    for elem in d:
        if elem["Type"] == "TX":
            continue

        accessor = elem["Exp"] + "_" + elem["Conns"]

        if "pkts" in new_json[accessor]:
            new_json[accessor]["pkts"] += (elem["After"] - elem["Before"])
        else:
            new_json[accessor]["pkts"] = (elem["After"] - elem["Before"])



with open("./summaries/summary_softirq.json") as f:
    d = json.load(f)

    for elem in d:
        if elem["Type"] == "TX":
            continue

        accessor = elem["Exp"] + "_" + elem["Conns"]

        if "softirqs" in new_json[accessor]:
            new_json[accessor]["softirqs"] += (elem["After"] - elem["Before"])
        else:
            new_json[accessor]["softirqs"] = (elem["After"] - elem["Before"])



final_json=list()

for key in new_json.keys():
    tmp = dict()
    tmp["Type"] = key.split("_")[0]
    tmp["Conns"] = key.split("_")[1]
    tmp["pkts"] = new_json[key]["pkts"]
    tmp["softirqs"] = new_json[key]["softirqs"]
    final_json.append(tmp)

with open("./summaries/summary_sirq_per_pkt.json", "w") as f:
    json.dump(final_json, f, indent=0)


