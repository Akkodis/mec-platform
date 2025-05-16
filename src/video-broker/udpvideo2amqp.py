# Useful resources: 
#   https://qpid.apache.org/releases/qpid-proton-0.36.0/proton/python/docs/tutorial.html
#   https://access.redhat.com/documentation/en-us/red_hat_amq/6.3/html/client_connectivity_guide/amqppython

from __future__ import print_function

import os
import optparse
import json
import time
from proton.handlers import MessagingHandler
from proton.reactor import Container

import threading

import sys
import numpy

import gi

gi.require_version('GLib', '2.0')
gi.require_version('GObject', '2.0')
gi.require_version('Gst', '1.0')
gi.require_version('GstApp', '1.0')
gi.require_version('GstVideo', '1.0')

from gi.repository import Gst, GObject, GLib, GstApp, GstVideo

import content

# Environment parameters
broker_ip=os.getenv("AMQP_IP") 
broker_port=os.getenv('AMQP_PORT')
topic="video"
user=os.getenv('AMQP_USER')
passwd=os.getenv('AMQP_PASS')

# Class to send video frames as messages into AMQP
class Sender(MessagingHandler):
    def __init__(self, url, message):
        super(Sender, self).__init__()
        self.url = url
        self._message = message
        self._sent_count = 0
        self._confirmed_count = 0

    def on_start(self, event):
        print("Sender Created")
        event.container.create_sender(self.url)

    def on_sendable(self, event):
        message = self._message
        #print("Send to "+ self.url +": \n\t" )#+ str(message))
        event.sender.send(message)
        self._sent_count += 1
        event.sender.close()

    def on_accepted(self, event):
        self._confirmed_count += 1
        event.connection.close()

    def on_transport_error(self, event):
        raise Exception(event.transport.condition)


# Get video frame from a GST Buffer
def extract_buffer(sample):
    """Extracts Gst.Buffer from Gst.Sample and converts to np.ndarray"""

    buffer = sample.get_buffer()  # Gst.Buffer

    caps = sample.get_caps()  # Gst.Caps

    buffer_size = buffer.get_size()
    array = numpy.ndarray((buffer_size, 1, 1), buffer=buffer.extract_dup(0, buffer_size), dtype=numpy.uint8)

    return array  # remove single dimension if exists

# Callback gets GST Buffer from a UDP stream
def on_buffer(sink, data):
    """Callback on 'new-sample' signal"""
    # Emit 'pull-sample' signal
    # https://lazka.github.io/pgi-docs/GstApp-1.0/classes/AppSink.html#GstApp.AppSink.signals.pull_sample

    sample = sink.emit("pull-sample")  # Gst.Sample

    # Prepare AMQP config
    server_url="amqp://"+user+":"+passwd+"@"+broker_ip+":"+str(broker_port)+"/topic://"+topic

    if isinstance(sample, Gst.Sample):
        array = extract_buffer(sample)
        print(
            "Received {type} with shape {shape} of type {dtype}".format(type=type(array),
                                                                        shape=array.shape,
                                                                        dtype=array.dtype))
        # Prepare message with the video frame
        content.message_generator(data.id, data.fps, data.tile, array.tobytes())
        # Send message (video frame) to AMQP
        Container(Sender(server_url, content.message)).run()
        return Gst.FlowReturn.OK

    return Gst.FlowReturn.ERROR


class UDP2AMQP(threading.Thread):
    
    def __init__(self, id, port, fps, tile) :
        super().__init__()
        self._kill = threading.Event()

        self.id = id
        self.port = port
        self.fps = fps
        self.tile = tile

        self.msg = None
        self.pipeline = None
        self.bus = None
        self.appsink = None

    def run(self):
        # initialize GStreamer
        Gst.init(sys.argv[1:])

        print ("\n\n\t\tRUN!\n\n")

        # build the pipeline to receive UDP video stream
        self.pipeline = Gst.parse_launch(
            'udpsrc port=' + str(self.port) + ' ! application/x-rtp, payload=96, media=video, clock-rate=90000, encoding-name=H264 ! rtpjitterbuffer latency=100 ! rtph264depay name=depay ! queue max-size-buffers=1 ! video/x-h264 ! h264parse config-interval=-1 ! video/x-h264, stream-format=byte-stream, alignment=au ! appsink emit-signals=true name=appsink'
        )

        self.appsink = self.pipeline.get_by_name('appsink')  # get AppSink
        # subscribe to <new-sample> signal
        self.appsink.connect("new-sample", on_buffer, self)

        # start playing
        ret = self.pipeline.set_state(Gst.State.PLAYING)
        if ret == Gst.StateChangeReturn.FAILURE:
            print("Unable to set the pipeline to the playing state.")
            exit(-1)

        # wait until EOS or error
        self.bus = self.pipeline.get_bus()

        # Main loop
        while True:
            self.msg = self.bus.timed_pop_filtered(10000, Gst.MessageType.ANY)
            if self.msg:
                if self.msg.type == Gst.MessageType.ERROR:
                    err, debug = self.msg.parse_error()
                    print(("Error received from element %s: %s" % (
                        self.msg.src.get_name(), err)))
                    print(("Debugging information: %s" % debug))
                    break
                elif self.msg.type == Gst.MessageType.EOS:
                    print("End-Of-Stream reached.")
                    break
                elif self.msg.type == Gst.MessageType.STATE_CHANGED:
                    if isinstance(self.msg.src, Gst.Pipeline):
                        old_state, new_state, pending_state = msg.parse_state_changed()
                        print(("Pipeline state changed from %s to %s." %
                            (old_state.value_nick, new_state.value_nick)))
                else:
                    print("Unexpected message received.")
            time.sleep(1.0/float(self.fps))

        # free resources
        self.pipeline.set_state(Gst.State.NULL)

    def kill(self):
        self._kill.set()
