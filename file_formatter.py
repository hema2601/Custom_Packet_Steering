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


Filetype = Enum('Filetype', ['PACKET_CNT', 'SOFTIRQ', 'IRQ', 'IPERF', 'SOFTNET'])


class JsonGenerator:
    # json_dict = list()
    # f = None

    _tr = {}

    def __init__(self, path):
        print("Open the file: " + path)
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
        print("Close all files")
        self.f.close()


class SOFTIRQGen(JsonGenerator):
    def generate_json(self):
        print("Generate softirq.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):
        print("Read original softirq.json")
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
        print("Generate irq.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):
        print("Read original irq.json")

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
        print("Generate packet_cnt.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):
        print("Read original packet_cnt.json")
        for line in self.f:
            parts = [x for x in line.split(':') if x.strip()]

            # Read Type (TX/RX)
            if 'rx' in parts[0]:
                net_type = 'RX'
            if 'tx' in parts[0]:
                net_type = 'TX'

            # Read Queue Number
            q_num = int(re.sub("[^0-9]", "", parts[0]))

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
        print("Generate iperf.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):
        print("Read original iperf.json")
        tmp_dict = json.load(self.f)
        conns = list()

        for conn in tmp_dict["start"]["connected"]:
            elem = dict()
            elem["Socket"] = conn["socket"]
            elem["NAPI_ID"] = 0
            conns.append(elem)

        for interval in tmp_dict["intervals"]:

            for stream in interval["streams"]:
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

                self.json_dict.append(elem)

class SOFTNETGen(JsonGenerator):

    def generate_json(self):
        print("Generate softnet.json")
        self.f.seek(0)
        self.f.truncate()
        json.dump(self.json_dict, self.f, indent=0)

    def read_source(self):
        print("Read original softnet.json")


        for line in self.f:
            parts = [x for x in line.split(' ') if x.strip()]

            # Read Queue Number
            curr_cpu = int(parts[-1],16)
            print(curr_cpu)

            # Check if object exists
            elem = next((item for item in self.json_dict if item["CPU"] == curr_cpu), None)

            # Write to object
            if elem is not None:
                elem["After"] = int(parts[1])
                elem["Processed"] = int(parts[0],16) - elem["Processed"]
                elem["Dropped"] = int(parts[1],16) - elem["Dropped"]
                elem["Squeezed"] = int(parts[2],16) - elem["Squeezed"]
                elem["RPS Interrupts"] = int(parts[3],16) - elem["RPS Interrupts"]
            else:
                elem = dict()
                elem["CPU"] = curr_cpu
                elem["Processed"] = int(parts[0],16)
                elem["Dropped"] = int(parts[1],16)
                elem["Squeezed"] = int(parts[2],16)
                elem["RPS Interrupts"] = int(parts[3],16)
                self.json_dict.append(elem)



argc = len(sys.argv)


if argc < 3:
    print("Usage: python3 file_formatter.py <experiment name> <file type>")


# get folder name from command line

# [TODO] Check if the directory exists
folder = "./data/" + sys.argv[1] + "/"


for target in sys.argv[2:]:

    # [TODO] Guard this with try
    ftype = Filetype[target]

    gen = JsonGenerator.instantiate(ftype, folder)

    if gen is None:
        continue

    gen.read_source()
    gen.generate_json()
    gen.cleanup()
