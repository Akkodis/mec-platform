
from __future__ import print_function

import optparse
import json
import time
from proton.handlers import MessagingHandler
from proton.reactor import Container

import discovery_registration
import content

from pygeotile.tile import Tile
import random





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




def get_tile(latitude, longitude,zoom=18):
    tileTmp = Tile.for_latitude_longitude(latitude=latitude, longitude=longitude, zoom=zoom)
    tile=str(tileTmp.quad_tree)
    return tile


def gen_metadata(latitude,longitude):
    # Replace with your metadata
    tile=get_tile(latitude,longitude)
    dataflowmetadata = {
    "dataTypeInfo": {
        "dataType": "cits",
        "dataSubType": "cam"
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
    return dataflowmetadata


def get_message_broker_address(tile):
    # Get Message Broker access
    service="message-broker"
    messageBroker_ip, messageBroker_port = discovery_registration.discover_sb_service(tile,service)
    if messageBroker_ip == -1 or messageBroker_port == -1:
        print(service+" service not found")
        return -1
    
    address="amqp://"+username+":"+password+"@"+str(messageBroker_ip)+":"+str(messageBroker_port)+":/topic://"+topic    
    return address

def gen_moving_gps(latitude,longitude):
    
    # Pseudo movement to get different MEC coverage

    lat_rand=random.uniform(0.001,-0.02)
    long_rand=random.uniform(0.01,-0.01)

    latitudeRand=latitude+lat_rand
    longitudeRand=longitude+long_rand
    return latitudeRand, longitudeRand

if __name__ == "__main__":


    # Geoposition - Next steps: from GPS device. Now hardcoded.
    # Geoposition tuned for Vicomtech MEC
    latitude    = 43.2924
    longitude    = -1.9861

    username="user"
    password="password"

    timeinterval=10

    tile= get_tile(latitude, longitude)
    print(tile)

    dataflowmetadata=gen_metadata(latitude,longitude)

    # Get Topic and dataFlowId to push data into the Message Broker
    dataflowId, topic = discovery_registration.register(dataflowmetadata,tile)

    address = get_message_broker_address(tile)

    # generate message
    content.messages_generator(1,tile)

    # send message
    while(True):
        try:
            lat,lon=gen_moving_gps(latitude,longitude)
            randTile=get_tile(lat,lon,20)
            dataflowmetadata=gen_metadata(lat,lon)
            address = get_message_broker_address(randTile)
            if address != -1:
                
                # AMQP address is sent as payload in order to identify which MEC has been used to push the data
                content.messages_generator(1,randTile,"Other Tile "+address)
                try:
                    Container(Sender(address, content.messages)).run()
                    discovery_registration.keepAliveDataflow(dataflowmetadata,dataflowId)
                except:
                    print("Cannot connect to "+address)
            else:
                print("MEC not found, do not send anything")
            print("... \n")
        except KeyboardInterrupt:
            pass
        if (int(timeinterval) == 0):
            break
        time.sleep(int(timeinterval))
