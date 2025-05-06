import connexion
import six

from openapi_server.models.data_flow import DataFlow  # noqa: E501
from openapi_server import util

import sqlalchemy as db
import json

import time
import threading

import os

db_ip = os.environ["DB_IP"]
db_port = os.environ["DB_PORT"]
db_user = os.environ["DB_USER"]
db_password = os.environ["DB_PASSWORD"]
db_name = os.environ['DB_NAME']
dataflow_vehicle_minutes_limit = int(os.environ["DATAFLOW_VEHICLE_MINUTES_LIMIT"])
dataflow_infrastructure_minutes_limit = int(os.environ['DATAFLOW_INFRASTRUCTURE_MINUTES_LIMIT'])

engine = db.create_engine('mysql+pymysql://'+db_user+':'+db_password+'@'+db_ip+':'+db_port+'/'+db_name, isolation_level="READ UNCOMMITTED")
connection = engine.connect()
metadata = db.MetaData()
dataflows = db.Table('dataflows', metadata, autoload=True, autoload_with=engine)
pipelines = db.Table('pipelines', metadata, autoload=True, autoload_with=engine)
topics = db.Table('topics', metadata, autoload=True, autoload_with=engine)

def applyRules(dataflow):

    connection_local = engine.connect()

    query = db.select([pipelines]).where(pipelines.columns.filterDataType == dataflow['dataType'])
    result = connection_local.execute(query).fetchone()

    connection_local.close()

    if(result is None):
        return dataflow

    df = dataflow
    rules = result['pipelineRules']
    for k in rules:
        df[k] = rules[k]
    return df
    

def delete_old_dataflows():
    while(True):
        query = db.sql.delete(dataflows).where(dataflows.columns.sourceType == "vehicle").where(int(time.time()*1000) - dataflows.columns.timeLastUpdate > dataflow_vehicle_minutes_limit*60*1000)
        connection.execute(query)
        query = db.sql.delete(dataflows).where(dataflows.columns.sourceType == "infrastructure").where(int(time.time()*1000) - dataflows.columns.timeLastUpdate > dataflow_infrastructure_minutes_limit*60*1000)
        connection.execute(query)
        time.sleep(30)

def add_dataflow(body):  # noqa: E501
    """Register a Dataflow

     # noqa: E501

    :param body: Dataflow metadata
    :type body: dict | bytes

    :rtype: str
    """
    if connexion.request.is_json:
        body = DataFlow.from_dict(connexion.request.get_json())  # noqa: E501

    connection_local = engine.connect()

    # Read all the topics to set the counter
    query = db.select([topics])
    ts = connection_local.execute(query).fetchall()
    counter = 0
    for t in ts:
        if( t["dataType"] == body.data_type_info.data_type and
            (t["dataSubType"] == body.data_type_info.data_sub_type or t["dataSubType"] is None) and
            (t["dataFormat"] == body.data_info.data_format or t["dataFormat"] is None) and
            (t["sourceId"] == body.data_source_info.source_id or t["sourceId"] is None) and
            (t["sourceType"] == body.data_source_info.source_type or t["sourceType"] is None) and
            (t["locationCountry"] == body.data_source_info.source_location_info.location_country or t["locationCountry"] is None) and
            (t["locationQuadkey"] is None or body.data_source_info.source_location_info.location_quadkey.startswith(t["locationQuadkey"])) 
           ):
            counter = counter + 1

    dataflow_json = {
        "dataType": body.data_type_info.data_type,
        "dataSubType":  body.data_type_info.data_sub_type,
        "dataFormat": body.data_info.data_format,
        "dataSampleRate": body.data_info.data_sample_rate,
        "licenseGeolimit": body.license_info.license_geo_limit,
        "licenseType": body.license_info.license_type,
        "locationQuadkey": body.data_source_info.source_location_info.location_quadkey,
        "locationLatitude": body.data_source_info.source_location_info.location_latitude,
        "locationLongitude": body.data_source_info.source_location_info.location_longitude,
        "locationCountry": body.data_source_info.source_location_info.location_country,
        "timeRegistration": int(time.time()*1000),
        "timeLastUpdate": int(time.time()*1000),
        "timeZone": body.data_source_info.source_timezone,
        "timeStratumLevel": body.data_source_info.source_stratum_level,
        "extraAttributes": json.loads(body.data_info.extra_attributes.replace("\'", "\"")) if body.data_info.extra_attributes else None,
        "sourceId": body.data_source_info.source_id,
        "sourceType": body.data_source_info.source_type,
        "dataflowDirection": body.data_info.data_flow_direction,
        "counter": counter
    }
    dataflow_json = applyRules(dataflow_json)

    query = db.sql.insert(dataflows).values(dataflow_json)

    try:
        connection_local.execute(query)
        local_id = connection_local.execute('SELECT LAST_INSERT_ID() AS id').fetchone()['id']
        connection_local.close()
        return {"id": local_id, "topic": body.data_type_info.data_type.lower(), "send": counter>0}
    except Exception as e:
        connection_local.close()
        return str(e.__str__), 405

    return df_id


def delete_dataflow(dataflowid):  # noqa: E501
    """Delete a registered a Dataflow

     # noqa: E501

    :param dataflowid: Id of the dataflow
    :type dataflowid: str

    :rtype: str
    """
    connection_local = engine.connect()

    query = db.sql.delete(dataflows).where(dataflows.columns.dataflowId == dataflowid)
    connection_local.execute(query)

    connection_local.close()

    return {"Message": "Done"}


def update_dataflow(body, dataflowid):  # noqa: E501
    """Update a registered a Dataflow

     # noqa: E501

    :param body: Dataflow metadata
    :type body: dict | bytes
    :param dataflowid: Id of the dataflow
    :type dataflowid: str

    :rtype: str
    """
    
    connection_local = engine.connect()

    query = db.select([dataflows]).where(dataflows.columns.dataflowId == dataflowid)
    result = connection_local.execute(query).fetchone()
    if (result is None):
        return {
            "error": "Item not found"
        }, 404

    if connexion.request.is_json:
        body = DataFlow.from_dict(connexion.request.get_json())  # noqa: E501
    
    dataflow_json = {
        "dataType": body.data_type_info.data_type,
        "dataSubType":  body.data_type_info.data_sub_type,
        "dataFormat": body.data_info.data_format,
        "dataSampleRate": body.data_info.data_sample_rate,
        "licenseGeolimit": body.license_info.license_geo_limit,
        "licenseType": body.license_info.license_type,
        "locationQuadkey": body.data_source_info.source_location_info.location_quadkey,
        "locationLatitude": body.data_source_info.source_location_info.location_latitude,
        "locationLongitude": body.data_source_info.source_location_info.location_longitude,
        "locationCountry": body.data_source_info.source_location_info.location_country,
        "timeLastUpdate": int(time.time()*1000),
        "timeZone": body.data_source_info.source_timezone,
        "timeStratumLevel": body.data_source_info.source_stratum_level,
        "extraAttributes": json.loads(body.data_info.extra_attributes.replace("\'", "\"")) if body.data_info.extra_attributes else None,
        "sourceId": body.data_source_info.source_id,
        "sourceType": body.data_source_info.source_type,
        "dataflowDirection": body.data_info.data_flow_direction,
        "counter": result["counter"]
    }
    dataflow_json = applyRules(dataflow_json)

    query = db.sql.update(dataflows).where(dataflows.columns.dataflowId == dataflowid).values(dataflow_json)

    try:
        connection_local.execute(query)
        connection_local.close()
    except Exception as e:
        connection_local.close()
        return str(e.__str__), 405

    return {"id": dataflowid, "topic": body.data_type_info.data_type.lower(), "send": result["counter"] > 0}
