import requests
import json

from CANEncode import CANQuery, CANResponse, Command, MONITOR_COMMAND, CONTROL_COMMAND
from FEMCDefinitions import *

# URL for HTTP/CAN request
URL = "http://10.42.0.246"

# Create a CAN Query using id=0x500000 and delay time of 20ms
canQuery = CANQuery(id=0x500000, delay=20, url=URL)

# Append some CAN Commands
canQuery.append(Command(MONITOR_COMMAND, 0x20001, []))
canQuery.append(Command(MONITOR_COMMAND, 0x2100E, [1]))
canQuery.append(Command(MONITOR_COMMAND, 0x2000E, []))
canQuery.append(Command(CONTROL_COMMAND, POWER_DIST_MODULE | BAND3 | PD_ENABLED, [DISABLE]))
canQuery.append(Command(MONITOR_COMMAND, POWER_DIST_MODULE | POWERED_MODULES, []))
canQuery.append(Command(MONITOR_COMMAND, POWER_DIST_MODULE | BAND3 | PD_P6V | PD_CURRENT, []))
canQuery.append(Command(MONITOR_COMMAND, POWER_DIST_MODULE | BAND3 | PD_P6V | PD_VOLTAGE, []))
canQuery.append(Command(MONITOR_COMMAND, POWER_DIST_MODULE | BAND3 | PD_P8V | PD_CURRENT, []))
canQuery.append(Command(MONITOR_COMMAND, POWER_DIST_MODULE | BAND3 | PD_P8V | PD_VOLTAGE, []))
canQuery.append(Command(MONITOR_COMMAND, POWER_DIST_MODULE | BAND3 | PD_P15V | PD_CURRENT, []))
canQuery.append(Command(MONITOR_COMMAND, POWER_DIST_MODULE | BAND3 | PD_P15V | PD_VOLTAGE, []))
canQuery.append(Command(MONITOR_COMMAND, POWER_DIST_MODULE | BAND3 | PD_P24V | PD_CURRENT, []))
canQuery.append(Command(MONITOR_COMMAND, POWER_DIST_MODULE | BAND3 | PD_P24V | PD_VOLTAGE, []))

# Perform request
print(canQuery.getUrl())
jsonResponse = canQuery.request()
canRespose = CANResponse(jsonResponse)
print(canRespose)

# Clear previous CAN Query for new commands or create a new one
canQuery.clear()

# Append some CAN Commands
canQuery.append(Command(MONITOR_COMMAND, BAND3_MODULE | CARTRIDGE_LO_TEMP | TEMP_4K, []))
canQuery.append(Command(MONITOR_COMMAND, BAND3_MODULE | CARTRIDGE_LO_TEMP | TEMP_15K, []))
canQuery.append(Command(MONITOR_COMMAND, BAND3_MODULE | CARTRIDGE_LO_TEMP | TEMP_110K, []))
canQuery.append(Command(MONITOR_COMMAND, BAND3_MODULE | CARTRIDGE_LO_TEMP | TEMP_SPARE, []))
canQuery.append(Command(MONITOR_COMMAND, BAND3_MODULE | CARTRIDGE_LO_TEMP | TEMP_MIXER0, []))
canQuery.append(Command(MONITOR_COMMAND, BAND3_MODULE | CARTRIDGE_LO_TEMP | TEMP_MIXER1, []))

# Perform request
print(canQuery.getUrl())
jsonResponse = canQuery.request()
canRespose = CANResponse(jsonResponse)
print(canRespose)
