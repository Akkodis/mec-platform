import requests

def get_auth_token():
    headers = {
    'Content-Type': 'application/x-www-form-urlencoded',
    }
    data = {
    'grant_type': 'password',
    'username': '5gmeta',
    'password': 'user',
    'client_id': 'password',
    }
    response = requests.post('https://5gmeta-platform.eu/identity/realms/5gmeta/protocol/openid-connect/token', headers=headers, data=data)
    r = response.json()
    return r['access_token']

def get_header_with_token():
    token = "Bearer " + get_auth_token()

    headers = {
    'Authorization': token,
    }

    return headers
