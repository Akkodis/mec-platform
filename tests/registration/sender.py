#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
#
# Author: D.Amendola
# Useful resources: 
#   https://qpid.apache.org/releases/qpid-proton-0.36.0/proton/python/docs/tutorial.html
#   https://access.redhat.com/documentation/en-us/red_hat_amq/6.3/html/client_connectivity_guide/amqppython

from __future__ import print_function

import optparse
import json
import time
from proton.handlers import MessagingHandler
from proton.reactor import Container

import address
import content

import sqlalchemy as db
import requests

from threading import Thread

send = False
dataflowId = -1
topic = ""

def sendKeepAlive():
    global send
    while(True):
        time.sleep(30)
        r = requests.put("http://"+platformaddress+':12346/dataflows/'+str(dataflowId), json = (dataflowmetadata))
        print(r.text)
        send = r.json()['send']

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
            print("Send to "+ self.url +": \n\t" )#+ str(message))
            event.sender.send(message)
            self._message_index += 1
            self._sent_count += 1

    def on_accepted(self, event):
        self._confirmed_count += 1
        if self._confirmed_count == len(self._messages):
            event.connection.close()

    def on_transport_error(self, event):
        raise Exception(event.transport.condition)


platformaddress = "192.168.85.151"
registrationapi_port = "12346"
amqp_port = "5673"

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
        "timeZone": 10,
        "timeStratumLevel": 3,
        "sourceId": 1,
        "sourceType": "vehicle",
        "sourceLocationInfo": {
            "locationQuadkey": str(12022301011102),
            "locationCountry": "ITA",
        }
    }   
}

body = '{"header":{"protocolVersion":2,"messageID":2,"stationID":3907},"cam":{"generationDeltaTime":2728,"camParameters":{"basicContainer":{"stationType":5,"referencePosition":{"latitude":435549160,"longitude":103036950,"positionConfidenceEllipse":{"semiMajorConfidence":4095,"semiMinorConfidence":4095,"semiMajorOrientation":3601},"altitude":{"altitudeValue":180,"altitudeConfidence":"unavailable"}}},"highFrequencyContainer":{"basicVehicleContainerHighFrequency":{"heading":{"headingValue":1340,"headingConfidence":127},"speed":{"speedValue":618,"speedConfidence":127},"driveDirection":"unavailable","vehicleLength":{"vehicleLengthValue":42,"vehicleLengthConfidenceIndication":"unavailable"},"vehicleWidth":20,"longitudinalAcceleration":{"longitudinalAccelerationValue":161,"longitudinalAccelerationConfidence":102},"curvature":{"curvatureValue":359,"curvatureConfidence":"unavailable"},"curvatureCalculationMode":"yawRateUsed","yawRate":{"yawRateValue":1,"yawRateConfidence":"unavailable"},"accelerationControl":"00","lanePosition":-1}},"lowFrequencyContainer":{"basicVehicleContainerLowFrequency":{"vehicleRole":"default","exteriorLights":"00","pathHistory":[{"pathPosition":{"deltaLatitude":-280,"deltaLongitude":1140,"deltaAltitude":250},"pathDeltaTime":22393}]}}}}}'

if __name__ == "__main__":

    # Url to add a dataflow
    url = "http://"+platformaddress+':'+registrationapi_port+'/dataflows'

    # Send the JSON of the dataflow's metadata, and receive the dataflowId and the topic where to publish the messages
    r = requests.post(url, json = dataflowmetadata)
    if(r.status_code == 200):
        r = r.json()
        dataflowId = r['id']
        topic = r['topic']
        send = r['send']
    else:
        print(r.text)
        exit()

    #Start sending keepalives
    thread = Thread(target = sendKeepAlive)
    thread.start()

    # Start publishing messages in the received topic
    while(True):
        try:
            #If need to send, send message every second
            if(send):
                content.messages_generator(1, body)
                Container(Sender("amqp://<username>:<password>@"+platformaddress+":"+amqp_port+":/topic://"+topic, content.messages)).run()
                print("Message sent.\n")
        except KeyboardInterrupt:
            exit(1)
        time.sleep(int(1))
