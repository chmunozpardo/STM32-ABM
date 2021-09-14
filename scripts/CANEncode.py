import struct


class Command:
    def __init__(self, RCA: int, data=[]):
        self.RCA = RCA
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
            message = hex(self.RCA).upper() + ",0x0"
        elif self.length == 1:
            if type(self.data[0]) is float:
                message = (
                    hex(self.RCA).upper() + ",0x1," + self.floatToString(self.data[0])
                )
            elif type(self.data[0]) is int:
                message = (
                    hex(self.RCA).upper() + ",0x1," + self.intToString(self.data[0])
                )
        elif self.length > 1:
            message = hex(self.RCA).upper() + "," + self.intToString(self.length)
            for x in self.data:
                message = message + "," + self.intToString(x)
        return message


class CANQuery:
    def __init__(self, id, delay: int):
        self.id = id
        self.commands = []
        self.delay = delay

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
