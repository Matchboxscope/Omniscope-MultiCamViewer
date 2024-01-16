import sys
import numpy as np
import cv2
import numpy
import serial
import base64


baud = 500000


def grab_image(conn):
    try:
        if 0:
            # close the device - similar to hard reset
            conn.setDTR(False)
            conn.setRTS(True)
            time.sleep(.1)
            conn.setDTR(False)
            conn.setRTS(False)
            time.sleep(.1)
            while 1:
                rLine = conn.readline()
                print(rLine)
                if rLine == b'':
                    break
        conn.write(b"c\n")
        img_buf = conn.readline()
        #print(img_buf)
        image = buf_to_img(img_buf)
        if image is None:
            conn.write(b"s\n")
        return image
    except Exception as e:
        print(conn.port + str(e))
        return None

def buf_to_img(buf):
    # Decode the Base64-encoded frame
    decoded_frame = base64.b64decode(buf)

    # Convert to a NumPy array and decode image
    nparr = np.frombuffer(decoded_frame, np.uint8)
    if np.squeeze(nparr.shape)>0:
        image = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        return image
    else:
        image = None

mConns = {}
import serial.tools.list_ports

ports = serial.tools.list_ports.comports()
mDevices = []
for port in ports:
    mDevice = port.device
    if mDevice.find('modem') >= 0:
        mDevices.append(port.device)
print(mDevices)
print(len(mDevices))
ports = mDevices# ['/dev/cu.usbmodem11143101', '/dev/cu.usbmodem11143301', '/dev/cu.usbmodem11144101', '/dev/cu.usbmodem11143201', '/dev/cu.usbmodem11143401', '/dev/cu.usbmodem11144201', '/dev/cu.usbmodem11144401', '/dev/cu.usbmodem11144301', '/dev/cu.usbmodem11142101', '/dev/cu.usbmodem11142301', '/dev/cu.usbmodem11142401', '/dev/cu.usbmodem11142201']

#ports = ports[0:4]
# start the stream
'''
readyPorts = []
for port in ports:
    try:
        mConns[port] = serial.Serial(port, baud, write_timeout=0.5, timeout=0.5)
        readyPorts.append(port)
        print("Connected to " + port)
    except Exception as e:print(e); continue
'''

while True:
    for port in ports:
        try:
            import time
            conn = serial.Serial(port, baud, write_timeout=0.5, timeout=1)

            
        
            image = grab_image(conn)
            if image is None:
                print("not reading from " + port)
                continue
            
            print("reading from " + port)
            cv2.imshow(port, image)
            k = cv2.waitKey(10)
            conn.close()
        except Exception as e:print(e); continue
            