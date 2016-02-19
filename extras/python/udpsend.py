import socket

msg = "Hello, World!"
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(msg, ('141.3.150.20', 6060))
