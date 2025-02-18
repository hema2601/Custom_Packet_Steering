import sys
from enum import Enum
import json
import re

# Add new filetypes here
class Filetype(Enum):
    PACKET_CNT  = 1
    SOFTIRQ     = 2
    IRQ         = 3
    IPERF       = 4
    SOFTNET     = 5
    PKT_STEER   = 6
    PROC_STAT   = 7
    PERF        = 8
    PERF_STAT   = 9
    BUSY_HISTO  = 10

Filetype = Enum('Filetype', ['PACKET_CNT', 'SOFTIRQ', 'IRQ', 'IPERF', 'SOFTNET', 'PKT_STEER', 'PROC_STAT', 'PERF', 'PERF_STAT', 'BUSY_HISTO'])


class JsonGenerator:
    # json_dict = list()
    # f = None

    _tr = {}

    def __init__(self, path):
        #print("Open the file: " + path)
        self.f = open(path, 'r+')
        self.json_dict = list()

    @staticmethod
    def instantiate(target, path):
        gen = None
        full_path = None
        for ftype in Filetype:
            if(ftype == target):
                gen = __class__._tr.get(ftype.name + "Gen")
                full_path = path + ftype.name.lower() + ".json"

        return gen(full_path) if gen is not None else None

    def __init_subclass__(cls):
        __class__._tr[cls.__name__] = cls

    def generate_json(self):
        print("Implement this in Child class. This is an empty function")

    def read_source(self):
        print("Implement this in Child class. This is an empty function")

    def cleanup(self):
        # close files
        #print("Close all files")
        self.f.close()


class SOFTIRQGen(JsonGenerator):
    def generate_json(self):
        #print("Generate softirq.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):
        #print("Read original softirq.json")
        for line in self.f:
            parts = [x for x in line.split(' ') if x.strip()]

            # Read Type (TX/RX)
            if 'RX' in parts[0]:
                net_type = 'RX'
            if 'TX' in parts[0]:
                net_type = 'TX'

            for idx, num in enumerate(parts[1:]):
                # Check if object exists
                elem = next((item for item in self.json_dict if item["Core"] == idx and item["Type"] == net_type), None)
                if elem is not None:
                    elem["After"] = int(num)
                else:
                    elem = dict()
                    elem["Core"] = idx
                    elem["Type"] = net_type
                    elem["Before"] = int(num)
                    self.json_dict.append(elem)


class IRQGen(JsonGenerator):
    def generate_json(self):
        #print("Generate irq.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):
        #print("Read original irq.json")


        first = True
        for line in self.f:
            parts = [x for x in line.split(' ') if x.strip()]

            if first is True:
                first = False
                offset = int(parts[0][:-1])

            irq_num = int(parts[0][:-1])
            q_num = int(parts[0][:-1]) - offset

            for idx, num in enumerate(parts[1:]):
                if num.isnumeric() is False:
                    continue

                # Check if object exists
                elem = next((item for item in self.json_dict if item["Core"] == idx and item["Queue"] == q_num ), None)
                if elem is not None:
                    elem["After"] = int(num)
                else:
                    elem = dict()
                    elem["Irq"] = irq_num
                    elem["Queue"] = q_num
                    elem["Core"] = idx
                    elem["Before"] = int(num)
                    self.json_dict.append(elem)


class PACKET_CNTGen(JsonGenerator):

    def generate_json(self):
        #print("Generate packet_cnt.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):
        #print("Read original packet_cnt.json")
        for line in self.f:

            dropped = False

            parts = [x for x in line.split(':') if x.strip()]

            # Read Type (TX/RX)
            if 'dropped' in parts[0]:
                net_type = 'dropped'
                dropped = True
            elif 'rx' in parts[0]:
                net_type = 'RX'
            elif 'tx' in parts[0]:
                net_type = 'TX'

            # Read Queue Number
            #print(parts[0])
            if not dropped:
                q_num = int(re.sub("[^0-9]", "", parts[0]))
            else:
                q_num = -1

            # Check if object exists
            elem = next((item for item in self.json_dict if item["Queue"] == q_num and item["Type"] == net_type), None)

            # Write to object
            if elem is not None:
                elem["After"] = int(parts[1])
            else:
                elem = dict()
                elem["Queue"] = q_num
                elem["Type"] = net_type
                elem["Before"] = int(parts[1])
                self.json_dict.append(elem)


class IPERFGen(JsonGenerator):

    def generate_json(self):
        #print("Generate iperf.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):
        #print("Read original iperf.json")
        tmp_dict = json.load(self.f)
        
        #histos = list()
        conns = list()
        #histo_stats = dict()
    

        for conn in tmp_dict["start"]["connected"]:
            elem = dict()
            elem["Socket"] = conn["socket"]
            elem["NAPI_ID"] = 0
            conns.append(elem)

        for interval in tmp_dict["intervals"]:

            for stream in interval["streams"]:
                print(self.json_dict)
                elem = dict()
                elem["bps"] = stream["bits_per_second"]
                elem["t"] = stream["end"]
                elem["Socket"] = stream["socket"]

                # Check NAPI Id and save it, then add to the element
                for conn in conns:
                    if conn["Socket"] == stream["socket"]:
                        if conn["NAPI_ID"] == 0 and stream["napi_id"] > 1:
                            conn["NAPI_ID"] = stream["napi_id"]

                        if conn["NAPI_ID"] != 0:
                            elem["NAPI_ID"] = conn["NAPI_ID"]

                        break

                histo = next((end_stream for end_stream in tmp_dict["end"]["streams"] if end_stream['receiver']['socket'] == stream['socket']), None)
                
                if(histo == None):
                    self.json_dict.append(elem)
                    continue

                histo = histo["receiver"]["rx_ts_histogram"]

                elem["min"] = histo["min"]
                elem["max"] = histo["max"]
                elem["avg"] = histo["avg"]
                elem["total"] = histo["total"]

                for single_bin in histo["bins"]:
                    new_elem = dict(elem)
                    new_elem["start"] = single_bin["start"]
                    new_elem["end"] = single_bin["end"]
                    new_elem["count"] = single_bin["count"]
                    self.json_dict.append(new_elem)

      #  for stream in tmp_dict["end"]["streams"]:

      #      histo_stream = stream["receiver"]["rx_ts_histogram"]

      #      if 'max' in histo_stats:
      #          
      #          histo_stats['max'] = max(histo_stream['max'], histo_stats['max'])
      #      else:
      #          histo_stats['max'] = histo_stream['max']
      #      if 'min' in histo_stats:
      #          histo_stats['min'] = min(histo_stream['min'], histo_stats['min'])
      #      else:
      #          histo_stats['min'] = histo_stream['min']
      #      if 'avg' in histo_stats:
      #          histo_stats['avg'] = (histo_stats['total'] * histo_stats['avg'] + histo_stream['avg']) / (histo_stats['total'] + histo_stream['total'])
      #      else:
      #          histo_stats['avg'] = histo_stream['avg']
      #      if 'total' in histo_stats:
      #          histo_stats['total'] += histo_stream['total']

      #      else:
      #          histo_stats['total'] = histo_stream['total']


      #      for single_bin in stream["receiver"]["rx_ts_histogram"]["bins"]: 
      #          
      #          elem = next((item for item in histos if item["start"] == single_bin["start"]), None)
      #          if elem is None:
      #              histos.append(single_bin)
      #          else:
      #              elem["count"] += single_bin["count"]
   
      #  for item in histos:
      #      self.json_dict.append(item)


      #  self.json_dict.append(histo_stats)



class SOFTNETGen(JsonGenerator):

    def generate_json(self):
        #print("Generate softnet.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):
        #print("Read original softnet.json")


        for line in self.f:
            parts = [x for x in line.split(' ') if x.strip()]

            # Read Queue Number
            curr_cpu = int(parts[-3],16)

            # Check if object exists
            elem = next((item for item in self.json_dict if item["CPU"] == curr_cpu), None)

            # Write to object
            if elem is not None:
                elem["Processed"] = int(parts[0],16) - elem["Processed"]
                elem["Dropped"] = int(parts[1],16) - elem["Dropped"]
                elem["Squeezed"] = int(parts[2],16) - elem["Squeezed"]
                elem["RPS Interrupts"] = int(parts[9],16) - elem["RPS Interrupts"]
                elem["IPI Enqueued"] = int(parts[4],16)- elem["IPI Enqueued"]
                elem["InputQ Dequeued"] = int(parts[5],16)- elem["InputQ Dequeued"]
                elem["Input Pkts"] = int(parts[6],16) - elem["Input Pkts"]

            else:
                elem = dict()
                elem["CPU"] = curr_cpu
                elem["Processed"] = int(parts[0],16)
                elem["Dropped"] = int(parts[1],16)
                elem["Squeezed"] = int(parts[2],16)
                elem["RPS Interrupts"] = int(parts[9],16)
                elem["IPI Enqueued"] = int(parts[4],16)
                elem["InputQ Dequeued"] = int(parts[5],16)
                elem["Input Pkts"] = int(parts[6],16)
                self.json_dict.append(elem)

class PKT_STEERGen(JsonGenerator):

    def generate_json(self):
        #print("Generate pkt_steer.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):
        #print("Read original pkt_steer.json")


        for line in self.f:
            parts = [x for x in line.split(' ') if x.strip()]

            # Read Queue Number
            curr_cpu = int(parts[0],16)

            # Check if object exists
            elem = next((item for item in self.json_dict if item["CPU"] == curr_cpu), None)

            # Write to object
            if elem is not None:
                elem["Total"] = int(parts[1],16) - elem["Total"]
                elem["PrevInvalid"] = int(parts[2],16)- elem["PrevInvalid"]
                elem["PrevIdle"] = int(parts[3],16)- elem["PrevIdle"]
                elem["AssignedToBusy"] = int(parts[4],16) - elem["AssignedToBusy"]
                elem["NoBusyAvailable"] = int(parts[5],16) - elem["NoBusyAvailable"]
                elem["TargetIsSelf"] = int(parts[6],16) - elem["TargetIsSelf"]
                elem["ChoseInvalid"] = int(parts[7],16) - elem["ChoseInvalid"]
                elem["Fallback"] = int(parts[8],16) - elem["Fallback"]
                elem["IsOverloaded"] = int(parts[9],16) - elem["IsOverloaded"]
                elem["FromOverloaded"] = int(parts[10],16) - elem["FromOverloaded"]
            else:
                elem = dict()
                elem["CPU"] = curr_cpu
                elem["Total"] = int(parts[1],16)
                elem["PrevInvalid"] = int(parts[2],16)
                elem["PrevIdle"] = int(parts[3],16)
                elem["AssignedToBusy"] = int(parts[4],16)
                elem["NoBusyAvailable"] = int(parts[5],16)
                elem["TargetIsSelf"] = int(parts[6],16)
                elem["ChoseInvalid"] = int(parts[7],16)
                elem["Fallback"] = int(parts[8],16)
                elem["IsOverloaded"] = int(parts[9],16)
                elem["FromOverloaded"] = int(parts[10],16)
                self.json_dict.append(elem)

class PROC_STATGen(JsonGenerator):

    def generate_json(self):
        #print("Generate proc_stat.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):
        #print("Read original proc_stat.json")


        for line in self.f:
            parts = [x for x in line.split(' ') if x.strip()]

            # Read Queue Number
            if len(parts[0]) == 3:
                continue

            if parts[0][:3] != "cpu":
                continue

            curr_cpu = int(parts[0][3:])

            # Check if object exists
            elem = next((item for item in self.json_dict if item["CPU"] == curr_cpu), None)

            # Write to object
            if elem is not None:
                elem["User"] = int(parts[1]) - elem["User"]
                elem["Nice"] = int(parts[2]) - elem["Nice"]
                elem["System"] = int(parts[3]) - elem["System"]
                elem["Idle"] = int(parts[4]) - elem["Idle"]
                elem["IOWait"] = int(parts[5]) - elem["IOWait"]
                elem["Irq"] = int(parts[6]) - elem["Irq"]
                elem["Softirq"] = int(parts[7]) - elem["Softirq"]
                elem["Steal"] = int(parts[8]) - elem["Steal"]
                elem["Guest"] = int(parts[9]) - elem["Guest"]
                elem["Guest_Nice"] = int(parts[10]) - elem["Guest_Nice"]
            else:
                elem = dict()
                elem["CPU"] = curr_cpu
                elem["User"] = int(parts[1])
                elem["Nice"] = int(parts[2])
                elem["System"] = int(parts[3])
                elem["Idle"] = int(parts[4])
                elem["IOWait"] = int(parts[5])
                elem["Irq"] = int(parts[6])
                elem["Softirq"] = int(parts[7])
                elem["Steal"] = int(parts[8])
                elem["Guest"] = int(parts[9])
                elem["Guest_Nice"] = int(parts[10])
                self.json_dict.append(elem)
class PERFGen(JsonGenerator):

    def generate_json(self):
        # pass
        #print("Generate perf.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):



        contributions = {}
        not_found = []
        symbol_map = {}
        total_contrib = 0.
        unaccounted_contrib = 0.
        this_type = None
        first = True
        # Read the symbols map file
        with open("symbol_mapping.tsv", "r") as f:
            for line in f.readlines():
                comps = line.split()
                if len(comps) == 2:
                    symbol, typ = line.split()
                    symbol_map[symbol] = typ
                    if typ not in contributions:
                        contributions[typ] = 0.
        
        # Process the perf output to calculate breakdown
        for line in self.f:

            comps = line.split()
            if len(comps) == 2 and comps[0] == "TYPE":
                if first is not True:
                    # Save previous perf info
                    elem = dict()
                    elem['Type'] = this_type
                    elem['Total'] = total_contrib
                    elem['Unaccounted'] = unaccounted_contrib
                    for typ in contributions.keys():
                        elem[typ] = contributions[typ]
                    #print(elem)
                    self.json_dict.append(elem)

                # reinitialize
                first = False
                this_type = comps[1]
                for x in contributions.keys():
                    contributions[x] = 0.
                not_found = []
                total_contrib = 0.
                unaccounted_contrib = 0.

                # skip to next line
                continue

            if total_contrib < 95:
                if len(comps) == 5 and comps[3] == "[k]":
                    func = comps[4].split(".")[0]
                    contrib = float(comps[0][:-1])
                    total_contrib += contrib
                    if func in symbol_map:
                        typ = symbol_map[func]
                        contributions[typ] += contrib
                    elif comps[2] == "[pkt_steer_module]":
                        contributions['netdev'] += contrib
                    else:
                        if contrib > 0.01:
                            not_found.append(func)
                        unaccounted_contrib += contrib
            else:
                continue
        # print(total_contrib)
        # print(unaccounted_contrib)
        # print(contributions)
        # for x in not_found:
        #     print(x)

        elem = dict()
        elem['Type'] = this_type
        elem['Total'] = total_contrib
        elem['Unaccounted'] = unaccounted_contrib
        for typ in contributions.keys():
            elem[typ] = contributions[typ]

        self.json_dict.append(elem)


        #return total_contrib, unaccounted_contrib, contributions, not_found

class PERF_STATGen(JsonGenerator):

    def generate_json(self):
        # pass
        #print("Generate perf_stat.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):

        first = True

        keys = ["cycles", "instructions", "LLC-loads", "LLC-load-misses"]
        elem = dict()

        for line in self.f:
            comps = line.split()
            
            if len(comps) == 2 and comps[0] == "TYPE":
                if first is not True:
                    self.json_dict.append(elem)
                    elem = dict()
                elem["Type"] = comps[1]

                first = False

                # skip to next line
                continue

            if len(comps) >= 2 and comps[1] in keys:
                elem[comps[1]] = int(comps[0].replace(',',''))

        self.json_dict.append(elem)

        #print(self.json_dict)
class BUSY_HISTOGen(JsonGenerator):

    def generate_json(self):
        #print("Generate proc_stat.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):
        #print("Read original proc_stat.json")
        line_count = 0;

        for line in self.f:
            parts = [x for x in line.split(' ') if x.strip()]
            idx = 0
            print("New line " + str(idx))
            for part in parts:
                print(idx)
                c = int(part,16)
                if line_count == 0:
                    elem = dict()
                    elem["Busy_CPUs"] = idx
                    elem["Count"] = c
                    self.json_dict.append(elem)
                else:
                    elem = next((item for item in self.json_dict if item["Busy_CPUs"] == idx), None)
                    elem["Count"] = c - elem["Count"]
                idx += 1

            line_count += 1
       # for line in self.f:
       #     parts = [x for x in line.split(' ') if x.strip()]

       #     # Read Queue Number
       #     if len(parts[0]) == 3:
       #         continue

       #     if parts[0][:3] != "cpu":
       #         continue

       #     curr_cpu = int(parts[0][3:])

       #     # Check if object exists
       #     elem = next((item for item in self.json_dict if item["CPU"] == curr_cpu), None)

       #     # Write to object
       #     if elem is not None:
       #         elem["User"] = int(parts[1]) - elem["User"]
       #         elem["Nice"] = int(parts[2]) - elem["Nice"]
       #         elem["System"] = int(parts[3]) - elem["System"]
       #         elem["Idle"] = int(parts[4]) - elem["Idle"]
       #         elem["IOWait"] = int(parts[5]) - elem["IOWait"]
       #         elem["Irq"] = int(parts[6]) - elem["Irq"]
       #         elem["Softirq"] = int(parts[7]) - elem["Softirq"]
       #         elem["Steal"] = int(parts[8]) - elem["Steal"]
       #         elem["Guest"] = int(parts[9]) - elem["Guest"]
       #         elem["Guest_Nice"] = int(parts[10]) - elem["Guest_Nice"]
       #     else:
       #         elem = dict()
       #         elem["CPU"] = curr_cpu
       #         elem["User"] = int(parts[1])
       #         elem["Nice"] = int(parts[2])
       #         elem["System"] = int(parts[3])
       #         elem["Idle"] = int(parts[4])
       #         elem["IOWait"] = int(parts[5])
       #         elem["Irq"] = int(parts[6])
       #         elem["Softirq"] = int(parts[7])
       #         elem["Steal"] = int(parts[8])
       #         elem["Guest"] = int(parts[9])
       #         elem["Guest_Nice"] = int(parts[10])
       #         self.json_dict.append(elem)


argc = len(sys.argv)


if argc < 3:
    print("Usage: python3 file_formatter.py <experiment name> <file type>")


# get folder name from command line

# [TODO] Check if the directory exists
folder = "/home/hema/Custom_Packet_Steering/data/" + sys.argv[1] + "/"


for target in sys.argv[2:]:

    # [TODO] Guard this with try
    ftype = Filetype[target]

    gen = JsonGenerator.instantiate(ftype, folder)

    if gen is None:
        continue

    gen.read_source()
    gen.generate_json()
    gen.cleanup()
