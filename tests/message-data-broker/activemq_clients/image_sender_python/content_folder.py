

from proton import Message
from proton import symbol, ulong, PropertyDict
import base64
import sys

messages = [Message(subject='s%d' % i, body=u'b%d' % i) for i in range(10)]

#
# author: DAmendola
# This method creates a list of messages that will be sent to the Message Broker.
#
def messages_generator(num, tile, image, msgbody=None ):
    messages.clear()

    
    if(msgbody==None):
        with open(image, "rb") as f:
            msgbody = base64.b64encode(f.read())
    
    print("Size of the image: " + str(sys.getsizeof(msgbody)))

    print("Sender prepare the messages... ")
    for i in range(num):        
        props = {
                    "dataType": "image", 
                    "dataSubType": "jpg", 
                    "sourceId": "v"+str(i),
                    "locationQuadkey": tile+str(i%4),
                    "body_size": str(sys.getsizeof(msgbody))
                }

        messages.append( Message(body=msgbody, properties=props) ) 
        #print(messages[i])
    print("Message array done! \n")
