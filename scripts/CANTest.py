import requests
from CANEncode import CANQuery, Command

# Create a CAN Query using id=0x500000 and delay time of 20ms
canQuery = CANQuery(0x500000, 20)

# Append some CAN Commands
canQuery.append(Command(0x2100E, [0x01]))
canQuery.append(Command(0x2000E, []))

# Create the query dictionary
query = canQuery.generateQuery()

# Send HTTP request
r = requests.get("http://10.42.0.246/abm.cgi", params=query)

# Print result
print(r.json())
