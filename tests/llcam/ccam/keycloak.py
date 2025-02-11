import requests
import sys

url = "https://5gmeta-platform.eu/identity/realms/5gmeta/protocol/openid-connect/token"

def get_auth_token(username, password):
    headers = {
    'Content-Type': 'application/x-www-form-urlencoded',
    }
    
    data = {
    'grant_type': 'password',
    'username': username,
    'password': password,
    'client_id': '5gmeta_login',
    }
    
    try:
        r = requests.post(url, headers=headers, data=data)
        r.raise_for_status()
        json_response = r.json()
        
        return json_response['access_token']
        
    except Exception as err:
#        print(f"{err}")
        sys.exit("Invalid username or password")

def get_header_with_token(username, password):
    token = "Bearer " + get_auth_token(username, password)

    headers = {
    'Authorization': token,
    }

    return headers
    