import struct
import requests


MONITOR_COMMAND = 0
CONTROL_COMMAND = 1


class Response:
    def __init__(self, dict):
        self.RCA = int(dict["rca"], 16)
        self.length = int(dict["length"], 16)
        if "data" in dict.keys():
            self.data = [int(value, 16) for value in dict["data"]]
        else:
            self.data = []

    def __str__(self):
        if self.length == 2:
            return "{{0x{:X},{},{}}}\n".format(self.RCA, self.length, self.data[0])
        if self.length == 5:
            tmpList = ["{:02x}".format(value) for value in self.data[:4]]
            tmpString = "".join(tmpList)
            floatOut = struct.unpack("!f", bytes.fromhex(tmpString))[0]
            return "{{0x{:X},{},{}}}\n".format(self.RCA, self.length, floatOut)
        else:
            return "{{0x{:X},{},{}}}\n".format(self.RCA, self.length, ["0x%02X" % i for i in self.data])


class Command:
    def __init__(self, commandType: int, RCA: int, data=[]):
        self.RCA = RCA
        self.command = commandType
        self.length = len(data)
        self.data = data

    def floatToBytes(self, floatNumber: float):
        ba = bytearray(struct.pack("f", floatNumber))
        return ba

    def intToBytes(self, intNumber: int):
        ba = bytearray(struct.pack("B", intNumber))
        return ba

    def bytesToString(self, bytesArray):
        outputString = ""
        for i, b in enumerate(bytesArray):
            tmp = "0x%02X" % b
            outputString = outputString + tmp
            if i < len(bytesArray) - 1:
                outputString = outputString + ","
        return outputString

    def intToString(self, intNumber: int):
        ba = self.intToBytes(intNumber)
        return self.bytesToString(ba)

    def floatToString(self, floatNumber: int):
        ba = self.floatToBytes(floatNumber)
        return self.bytesToString(ba)

    def commandToString(self):
        message = ""
        if self.length == 0:
            message = hex(self.RCA | (self.command << 16)).upper() + ",0x0"
        elif self.length == 1:
            if type(self.data[0]) is float:
                message = hex(self.RCA | (self.command << 16)).upper() + ",0x1," + self.floatToString(self.data[0])
            elif type(self.data[0]) is int:
                message = hex(self.RCA | (self.command << 16)).upper() + ",0x1," + self.intToString(self.data[0])
        elif self.length > 1:
            message = hex(self.RCA | (self.command << 16)).upper() + "," + self.intToString(self.length)
            for x in self.data:
                message = message + "," + self.intToString(x)
        return message


class CANQuery:
    def __init__(self, id, delay: int, url: str):
        self.id = id
        self.commands = []
        self.delay = delay
        self.url = url

    def append(self, command: Command):
        self.commands.append(command)

    def generateQuery(self):
        message = ""
        id = hex(self.id).upper()
        length = len(self.commands)
        for index, command in enumerate(self.commands):
            message = message + command.commandToString()
            if index < length - 1:
                message = message + "\r\n"
        dict = {"id": id, "message": message, "json": "true", "delay": str(self.delay)}
        return dict

    def request(self):
        query = self.generateQuery()
        r = requests.get(self.url + "/abm.cgi", params=query)
        return r.json()

    def getUrl(self):
        query = self.generateQuery()
        s = requests.Session()
        p = requests.Request('GET', self.url + "/abm.cgi", params=query).prepare()
        return p.url

    def clear(self):
        self.commands = []


class CANResponse:
    def __init__(self, dict):
        self.id = dict["id"]
        self.delay = dict["delay"]
        self.commandsSent = [Response(sent) for sent in dict["sent"]]
        self.commandsReceived = [Response(recv) for recv in dict["received"]]

    def __str__(self):
        return "".join([str(recv) for recv in self.commandsReceived])
