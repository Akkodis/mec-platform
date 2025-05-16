from logging import raiseExceptions
import signal
import subprocess
import sys
from datetime import datetime
import time, os, json

import string

import threading
from proton.reactor import ApplicationEvent, Container, EventInjector
from proton.handlers import MessagingHandler, TransactionHandler

from udpvideo2amqp import UDP2AMQP

ids = []
ports = []
framerates = []
prxs = []
parsers = []

# Received messages will trigger the generation of a peer to receive the WebRTC video or to retire the peer and stop the session
class Receiver(MessagingHandler):
    def __init__(self, url, topic):
        super(Receiver, self).__init__()
        self.url = url
        self.topic = topic
        self._messages_actually_received = 0
        self._stopping = False

    def on_start(self, event):
        print("Receiver Created")
        event.container.create_receiver(self.url + self.topic)

    def on_message(self, event):
        global ids
        global prxs
        global ports
        global parsers

        if self._stopping:
            return

        # New video stream to be received and ingressed in the MEC as UDP and then in the AMQP as messages
        if "new" in self.topic:
            print("Received message in NEW of size: {size}".format(size=event.message.properties['body_size']))
            print("Message Content-Type:" + event.message.properties['dataType'] + " Sub-Content-Type:" + event.message.properties['dataSubType'] + " from source ID:"+str(int(float(event.message.properties['sourceId']))))

            # Check that the video format is H.264 (other formats could be extended)
            if ((event.message.properties['dataType'] == "video") and (event.message.properties['dataSubType'] == "h264")):
                # Forward received UDP + H.264 stream into a UDP stream in the 5000-5999 port range
                port = ( int(float(event.message.properties['sourceId'])) % 1000 ) + 5000
                # Launch the receiver
                command = './webrtcRX --self-id=%s --report-period=0 --disable-ssl --server="ws://localhost:8443" --udp=%i' %("peer"+str(event.message.properties['sourceId']), port)

                # Get the FPS attribute
                framerate = event.message.properties['dataSampleRate']
                # Push the UDP into AMQP messages (1 message per frame)
                parser = UDP2AMQP(event.message.properties['sourceId'], port, framerate, event.message.properties['locationQuadkey'])
                parsers.append(parser)

                # Store the objects to manage them later
                print('command: '+command)      
                prx = subprocess.Popen(command,shell=True)
                ids.append(event.message.properties['sourceId'])
                ports.append(port)
                prxs.append(prx)
                framerates.append(framerate)
                
                print("Completed!")
                time.sleep(1)
                parser.start()
                time.sleep(1)

            self._messages_actually_received += 1
        #event.connection.close()
        #self._stopping = True

        # Existing video stream to stop sending to the MEC
        if "terminate" in self.topic:
            print("Received message in TERMINATE of size: {size}".format(size=event.message.properties['body_size']))

            # Check that the video format is H.264 (other formats could be extended)
            if ((event.message.properties['dataType'] == "video") and (event.message.properties['dataSubType'] == "h264")):
                port = ( int(float(event.message.properties['sourceId'])) % 1000 ) + 5000
                index = ids.index(event.message.properties['sourceId'])

                parsers[index].kill()
                time.sleep(1)

                # Remove the objects for the specific video stream to retire
                try:
                    del parsers[index]
                    #os.killpg(os.getpgid(prxs[index].pid), signal.SIGINT)
                    del ids[index]
                    del prxs[index]
                    del ports[index]
                    del framerates[index]
                except KeyboardInterrupt:
                    sys.exit()

            self._messages_actually_received += 1
        #event.connection.close()
        #self._stopping = True

    def on_transport_error(self, event):
        raise Exception(event.transport.condition)


class AMQP:
    
    def __init__(self, subscription:list=None, param:dict=None) -> None:
        self.subscription = subscription
        self.param = param

        # Launch the WebRTC Proxy
        if param['deviceType'] == 'webrtc_proxy':
            subprocess.Popen(['python3', 'simple_server.py', '--disable-ssl'])
            subprocess.Popen(["gst-inspect-1.0", "--version"])

        # Wait for new video streams in the corresponding topic
        time.sleep(1)
        print("Container RECEIVER NEWDATAFLOW")
        reactor0 = Container(Receiver(self.param['serverURL'],self.subscription[0]))
        # Create a thread
        thread0 = threading.Thread(target=reactor0.run)
        thread0.daemon=True
        thread0.start()

        # Wait for stopped video streams in the corresponding topic
        time.sleep(1)
        print("Container RECEIVER TERMINATEDATAFLOW")
        #Container(Receiver(self.param['serverURL']+self.subscription[1])).run()
        reactor1 = Container(Receiver(self.param['serverURL'],self.subscription[1]))
        # Create a thread
        thread1 = threading.Thread(target=reactor1.run)
        thread1.daemon=True
        thread1.start()

        while True:
            time.sleep(1.0)

