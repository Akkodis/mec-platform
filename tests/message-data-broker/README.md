
# Examples

## Example of client ActiveMQ (AMQP) 

The activemq_clients is used to sent messages froma Sensor&Device to the MEC.

This code provide an example of messages with a payload and with the properties that can be attached to the message. 

A list of the properties should be:
- source_id: 
- tile: Tile of source in QuadKey code. e.g. 1230123012301230 (must be 18 chars in [0-3])
- datatype: should be one of the allowed datatype [cits, video, image]
- sub_datatype: depend on the datatype e.g. cam, denm, mappem
 


How to use the activemq client to generate messages:


```
Usage: sender.py [options]

Send messages to the supplied address.

Options:
  -h, --help            show this help message and exit
  -a ADDRESS, --address=ADDRESS
                        address to which messages are sent (default
                        amqp://user:password@192.168.15.34:5673/topic://cits)
  -m MESSAGES, --messages=MESSAGES
                        number of messages to send (default 100)
  -t TIMEINTERVAL, --timeinterval=TIMEINTERVAL
                        messages are sent continuosly every time interval
                        seconds (0: send once) (default 10) 
```
