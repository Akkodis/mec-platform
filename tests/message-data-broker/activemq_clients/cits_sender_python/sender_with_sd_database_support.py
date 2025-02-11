from __future__ import print_function

import optparse
import json
import time
from proton.handlers import MessagingHandler
from proton.reactor import Container

import discovery_registration
import sd_database
import content

from pygeotile.tile import Tile
import os

# Geoposition - Next steps: from GPS device. Now hardcoded.
latitude    = 43.3128
longitude    = -1.9750

tileTmp = Tile.for_latitude_longitude(latitude=latitude, longitude=longitude, zoom=18)
tile=str(tileTmp.quad_tree)
username="user"
password="password"

## Replace with your S&D mysql database credentials
db_user="dbuser"
db_password="dbpassword"
db_ip="192.168.14.192"
db_port="3307"


# Replace with your metadata
dataflowmetadata = {
    "dataTypeInfo": {
        "dataType": "cits",
        "dataSubType": "json"
    },
    "dataInfo": {
        "dataFormat": "asn1_jer",
        "dataSampleRate": 0.0,
        "dataflowDirection": "upload",
        "extraAttributes": None,
    },
    "licenseInfo": {
        "licenseGeolimit": "europe",
        "licenseType": "profit"
    },
    "dataSourceInfo": {
        "sourceTimezone": 2,
        "sourceStratumLevel": 3,
        "sourceId": 1,
        "sourceType": "vehicle",
        "sourceLocationInfo": {
            "locationQuadkey": tile,
            "locationCountry": "ESP",
            "locationLatitude": latitude,
            "locationLongitude": longitude
        }
    }   
}

class Sender(MessagingHandler):
    def __init__(self, url, messages):
        super(Sender, self).__init__()
        self.url = url
        self._messages = messages
        self._message_index = 0
        self._sent_count = 0
        self._confirmed_count = 0

    def on_start(self, event):
        event.container.create_sender(self.url)

    def on_sendable(self, event):
        while event.sender.credit and self._sent_count < len(self._messages):
            message = self._messages[self._message_index]
            #print("Send to "+ self.url +": \n\t" )#+ str(message))
            #print(str(message))
            event.sender.send(message)
            self._message_index += 1
            self._sent_count += 1
            event.sender.close()

    def on_accepted(self, event):
        self._confirmed_count += 1
        if self._confirmed_count == len(self._messages):
            event.connection.close()

    def on_transport_error(self, event):
        raise Exception(event.transport.condition)



if __name__ == "__main__":

    parser = optparse.OptionParser(usage="usage: %prog [options]",
                               description="Send messages to the supplied address.")
    
    parser.add_option("-a", "--address",
                    help="address to which messages are sent (default %default)")

    parser.add_option("-m", "--messages", type="int", default=100,
                    help="number of messages to send (default %default)")

    parser.add_option("-t", "--timeinterval", default=10,
                    help="messages are sent continuosly every time interval seconds (0: send once) (default %default)")


    opts, args = parser.parse_args()

    # Get Message Broker access
    service="message-broker"
    messageBroker_ip, messageBroker_port = discovery_registration.discover_sb_service(tile,service)
    if messageBroker_ip == -1 or messageBroker_port == -1:
        print(service+" service not found")
        exit(-1)
    
    # Get Topic and dataFlowId to push data into the Message Broker
    dataflowId, topic = discovery_registration.register(dataflowmetadata,tile)


    # Insert data into Sensor and Device database
    connection, dataflows,produced,owner,sensor,internalSensor = sd_database.prepare_database(db_user,db_password,db_ip,db_port)
    sd_database.insert_dataflow_localdb(dataflowmetadata, connection, dataflows, owner, dataflowId)
    sd_database.insert_sensor_local_db(dataflowmetadata, connection, sensor)
    sd_database.insert_internal_sensor_local_db(connection, internalSensor)
    sd_database.insert_dataflow_produced_dataflows_local_db(connection, produced, dataflowId)


    opts.address="amqp://"+username+":"+password+"@"+messageBroker_ip+":"+str(messageBroker_port)+":/topic://"+topic

    jargs = json.dumps(args)

    print("Sending #" + str(opts.messages) + " messages every " + str(opts.timeinterval) + " seconds to: " + str(opts.address) + "\n" )

    # Generate message
    content.messages_generator(opts.messages,tile)

    # send message
    while(True):
        try:
            Container(Sender(opts.address, content.messages)).run()
            sd_database.keepAliveDataflow(db_ip,db_user,db_password,db_port,tile)
            print("... \n")
        except KeyboardInterrupt:
            pass
        if (int(opts.timeinterval) == 0):
            break
        time.sleep(int(opts.timeinterval))
