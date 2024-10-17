import socket
import sys

UDP_HOST  = sys.argv[1]
UDP_PORT  = sys.argv[2]
TCPS_HOST = sys.argv[3]
TCPS_PORT = sys.argv[4]

UDP = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
TCP = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

TCP.bind((TCPS_HOST, TCPS_PORT))
TCP.listen()
while:
    conn, addr = s.accept()
    while True:
        = TCP.recv(1024);