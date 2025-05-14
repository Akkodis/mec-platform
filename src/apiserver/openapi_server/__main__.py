#!/usr/bin/env python3
from openapi_server.config.config import connexion_app
import uvicorn

app = connexion_app 

if __name__ == '__main__':
    app.add_api('edgeinstance.yaml', arguments={'title': 'Edge Instance API'}, pythonic_params=True)
    config = uvicorn.Config("__main__:app", host='0.0.0.0', port=5000, log_level="info")
    server = uvicorn.Server(config)
    server.run()
