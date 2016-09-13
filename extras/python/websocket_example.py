import socket

# install websockets:
#  sudo pip install websocket-client

from websocket import create_connection
ws = create_connection("ws://localhost:5500")

print 'send ping'
ws.send('ping')

result = ws.recv()
print 'received ', result

ws.close()

