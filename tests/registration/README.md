## Sender.py
Example app that sends a POST request to the Registration APIs to generate a dataflow, adds the dataflow information in the S&D DB and starts publishing AMQP messages.

## Requirements
- **requests**, to send the post request;
- **proton**, to send the messages;
- **sqlalchemy**, to access the S&D DB.

## How to install requirements
pip3 install requests python-qpid-proton sqlalchemy PyMySQL

## Run
`python3 sender.py`
