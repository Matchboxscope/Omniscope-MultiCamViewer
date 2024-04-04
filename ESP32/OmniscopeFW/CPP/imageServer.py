import socket
import base64
import cv2
import re
# Define host and port
HOST = '127.0.0.1'  # Standard loopback interface address (localhost)
PORT = 8080         # Port to listen on
import json

# Create a socket object (SOCK_STREAM indicates a TCP socket)
cameraFrames = {}
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    # Bind the socket to the address and port
    s.bind((HOST, PORT))
    # Enable the server to accept connections
    s.listen()
    print(f"Server listening on {HOST}:{PORT}")

    # Accept a connection
    conn, addr = s.accept()
    with conn:
        print(f"Connected by {addr}")
        # Loop to keep receiving data from the client
        
        mMessage = ""
        while True:
            while True:
                try:
                    data = conn.recv(1024*64)  # Receive data from the client
                    if not data:
                        break  # If no data is received, exit the loop

                    # Decode the data to a string
                    received_str = data.decode('utf-8')
                    # decode data to json
                    # Suche nach dem ersten gültigen JSON-Objekt in der Zeichenkette
                    # first cut out the last json that starts with { and ends with }
                    jsonString = json.loads(("{"+received_str.split("{")[1]).split("\r")[0]+'"}')
                    portname = jsonString["portName"]
                    mData = jsonString["data"]
                    # decode base64 data
                    
                    cameraFrames[portname] = base64.b64decode(mData)
                    # convert base64 to image
                    
                    
                    cv2.imshow(portname, cv2.imdecode(cameraFrames[portname], cv2.IMREAD_COLOR))
                    cv2.waitKey(1)
                    
                    # display using cv2 
                    
                    print(f"Received: {received_str.strip()}")
                    print(" ")
                    print(" ")
                except Exception as e:
                    print(f"Error: {e}")
                     
                 
            # Optional: Send a response back to the client
            # response = "Message received\n"
            # conn.sendall(response.encode('utf-8'))
