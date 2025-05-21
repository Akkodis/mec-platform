# CITS Message Quality

This module needs to be deployed inside a pipeline in the MEC platform.
This modules reads cits/cam data from each dataflow and assigns to each dataflow publishing a quality value from 1 to 7.    
This value is set in the "quality" property in the dataflowdb.    
A quality of 0 means that the dataflow has not published enough messages to be evalued.

## Example execution

```
docker run -e AMQP_ADDRESS=<ip>:><port> -e AMQP_USERNAME=<username> -e AMQP_PASSWORD=<password> -e AMQP_TOPIC=cits-large \
-e DB_ADDRESS=<5gmeta-cloud-db-ip>:<port> -e DB_USERNAME=<db-username> -e DB_PASSWORD=<db-password> -it --rm 5gmeta/data-quality
```

## CREDITS

* Federico Princiotto ([federico.princiotto@linksfoundation.com](mailto:federico.princiotto@linksfoundation.com))
* Francesco Aglieco ([francesco.aglieco@linksfoundation.com](mailto:francesco.aglieco@linksfoundation.com))

