import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # internet, UDP
sock.bind(('141.3.150.20', 6060))

while True:
	data, addr = sock.recvfrom(1024*32) # buffer size is 1024 bytes
	print "received message:", data
