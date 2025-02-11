# Based on this consumer: https://github.com/5gmeta/stream-data-gateway/blob/main/examples/consumer/cits-kafka-consumer.py

# Some sample code for the cosumere can be find 
# here https://github.com/confluentinc/confluent-kafka-python/blob/master/examples/json_consumer.py

import keycloak
from confluent_kafka import Consumer, KafkaException
import sys
import getopt
import json
import logging
from pprint import pformat
import time
from confluent_kafka import KafkaError
from confluent_kafka.avro import AvroConsumer
from confluent_kafka.avro.serializer import SerializerError
from confluent_kafka.cimpl import TopicPartition
import sys
import base64

import requests

import proton

import os
import statistics
from prometheus_client import start_http_server, Gauge

platformaddress = "5gmeta-platform.eu"
bootstrap_port = "31090"
registry_port =  "31081"
dataflowapi_port = "443"

# # Url to request a topic containing cits messages with subtype cam and with in the quadkey 123012301230123
# # url = "https://"+platformaddress+':'+dataflowapi_port+'/dataflow-api/topics/cits/query?instance_type=large&quadkey=12022301011102'
# url = "https://"+platformaddress+':'+dataflowapi_port+'/dataflow-api/topics/cits/query?instance_type=large&quadkey=0313331232'

# print(requests.delete("https://"+platformaddress+':'+dataflowapi_port+'/dataflow-api/topics/5GMETA_1009_CITS_LARGE_7', headers=keycloak.get_header_with_token("5gmeta", "5Gm3t4!")))
# # time.sleep(100000)

# # The request returns the generated topic
# r = requests.post(url, headers=keycloak.get_header_with_token("5gmeta", "5Gm3t4!"))
# if r.status_code != 200:
#     print(r.text)
#     exit()
# topic = r.text

topic = input("Insert 5GMETA topic: ")

c = AvroConsumer({
    'bootstrap.servers': "<5gmeta-ip>"+':' + bootstrap_port,
    'schema.registry.url':'http://'+"<5gmeta-ip>"+':' + registry_port, # 'http://192.168.15.44:8081',
    'group.id': 'group1',
    'api.version.request': True,
    'auto.offset.reset': 'earliest'
})

c.subscribe([topic.upper()])
print("Subscribed to topic: " + str(topic))
print("Running...")

i = 0

monitoring_port = int(os.getenv("MONITORING_PORT", 8080))
start_http_server(monitoring_port)
gauge = Gauge(
    "application_latency",
    "Application Latency."
)

latencies = [0,0,0,0,0,0,0,0,0,0]
def add_latency(value):
    if len(latencies) >= 10:
        latencies.pop(0)  # Remove the first element
    latencies.append(value)

while True:
    msg = c.poll(0.1)

    if msg is None:
        #print("Empty msg: " + str(msg) );
        print(".",  end="", flush=True)
        continue
    if msg.error():
        print("Consumer error: {}".format(msg.error()))
        continue
    
    # The AVRO Message here in mydata
    mydata = msg.value() # .decode('latin-1') #.replace("'", '"')

    # The QPID proton message: this is the message sent from the S&D to the MEC
    raw_sd = mydata['BYTES_PAYLOAD']
    msg_sd = proton.Message()
    proton.Message.decode(msg_sd, raw_sd)

    # The msg_sd.body contains the data of the sendor
    latency = time.time()*1000 - msg_sd.properties['timestamp']
    add_latency(latency)
    gauge.set(statistics.mean(latencies))
    print("Latency: "+ str(statistics.mean(latencies)))

c.close()
