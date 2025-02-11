import requests
from requests import ReadTimeout, ConnectTimeout, HTTPError, Timeout, ConnectionError

# For reference:
# https://stackoverflow.com/questions/9054820/python-requests-exception-handling

def get_auth_token():
    headers = {
    'Content-Type': 'application/x-www-form-urlencoded',
    }
    data = {
    'grant_type': 'password',
    'username': 'user',
    'password': 'password',
    'client_id': '5gmeta_login',
    }

    r = None
    try:
        response = requests.post('https://<ip>/identity/realms/5gmeta/protocol/openid-connect/token', headers=headers, data=data, timeout=2.5)
    except (ConnectTimeout, HTTPError, ReadTimeout, Timeout, ConnectionError):
        # print("Access_Token status:", response.ok)
        print("Unable to get the Access_Token ..... aborting connection !!!")
    else:
        r = response.json()['access_token']
    return r

def get_header_with_token():
    res = get_auth_token()
    if res is None:
        headers = None
    else:    
        token = "Bearer " + get_auth_token()

        headers = {
        'Authorization': token,
        }

    return headers
