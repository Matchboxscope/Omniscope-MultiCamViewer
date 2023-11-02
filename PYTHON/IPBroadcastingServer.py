import socket
import time
import threading 

from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import uvicorn

app = FastAPI()
ip_list = []  # List to store the IP addresses

class IPModel(BaseModel):
    ip: str

@app.get("/")
def reply():
    print("TEST")
    return {"message": "Welcome to the IP Broadcasting Server"}

@app.post("/setIP")
def set_ip(ip_model: IPModel):
    ip_list.append(ip_model.ip)
    print(f"IP added: {ip_model.ip}")
    return {"message": "IP added", "current_ips": ip_list}


def get_ip():
    """ Get the primary IP address of the machine. """
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't even have to be reachable
        s.connect(('10.255.255.255', 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP

def broadcast_ip(ip, port):
    """ Broadcasts the server IP address on the given port. """
    server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    # Set a timeout so the socket does not block indefinitely when trying to receive data.
    server.settimeout(0.2)
    message = ip.encode('utf-8')
    while True:
        server.sendto(message, ('<broadcast>', port))
        print(f"Message broadcasted: {ip}")
        time.sleep(5)

if __name__ == "__main__":
    port = 12345
    ip = get_ip()
    print(f"Server IP: {ip}")
    mThread = threading.Thread(target=broadcast_ip, args=(ip, port))
    mThread.start()
    uvicorn.run(app, host="0.0.0.0", port=4444)
    mThread.join()
    