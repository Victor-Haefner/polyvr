# python -m pip install websocket


print(' === Start scenegraph interface stress test ===')


import asyncio
import websockets


class Geo:
	def __init__(self):
		self.positions = ''
		self.normals = ''
		self.colors = ''
		self.indices = ''

		N = 100000
		K = 1.0/N
		
		for k in range(N):
			y1 = k*K
			y2 = y1+K
			self.positions += '0 '+str(y2)+' 1 0 '+str(y1)+' 1 0 '+str(y1)+' -1 0 '+str(y2)+' -1 '
			self.normals += '1 0 0 1 0 0 1 0 0 1 0 0 '
			self.colors += '0 '+str(y1)+' 1 1 0 '+str(y1)+' 1 1 0 '+str(y1)+' 1 1 0 '+str(y1)+' 1 1 '
			self.indices += str(k*4)+' '+str(k*4+1)+' '+str(k*4+2)+' '+str(k*4)+' '+str(k*4+2)+' '+str(k*4+3)+' '  #'0 1 2 0 2 3 '*N

async def sendObject(ws, geo, name, x, y, z):
	ID = name+'ID'
	await ws.send('new|Geometry|'+name+'|'+ID)
	await ws.send('set|transform|'+ID+'|1 0 0 0 1 0 0 0 1 '+str(x)+' '+str(y)+' '+str(z))
	await ws.send('set|positions|'+name+'|'+geo.positions)
	await ws.send('set|normals|'+name+'|'+geo.normals)
	await ws.send('set|colors|'+name+'|'+geo.colors)
	await ws.send('set|indices|'+name+'|'+geo.indices)
	await ws.send('set|material|'+name+'|mat')
	
async def startTest(ws):
	await ws.send('clear')
	await ws.send('new|Material|mat')
	geo = Geo()
	for i in range(10):
		await sendObject(ws, geo, 'testGeo'+str(i), i, i*0.1, 0)

async def init():
	uri = 'ws://localhost:5555'
	async with websockets.connect(uri) as websocket:
		await startTest(websocket)
		#res = await websocket.recv()

asyncio.get_event_loop().run_until_complete(init())