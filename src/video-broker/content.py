# https://qpid.apache.org/releases/qpid-proton-0.31.0/proton/python/docs/proton.html#proton.PropertyDict

from proton import Message
from proton import symbol, ulong, PropertyDict
import base64
import sys

message = Message(subject='s1', body=u'b1')

#
# This method creates a list of messages that will be sent to the Message Broker.
#
def message_generator(vid, fps, tile, msgbody=None ):
    global message
    # MAX SIZE 15MB
    
    if(msgbody==None):
        return
    
    print("Size of the image: " + str(sys.getsizeof(msgbody)))

    # Attributes of the messages to be sent (marshaling UDP into AMQP messages) and received (new and terminated video streams)
    props = {
                "dataType": "video", 
                "dataSubType": "h264", 
                "dataSampleRate": fps,
                "sourceId": vid, 
                "locationQuadkey": tile,
                "body_size": str(sys.getsizeof(msgbody))
            }

    message = Message(body=msgbody, properties=props) 
    #print("Message ready! \n")
