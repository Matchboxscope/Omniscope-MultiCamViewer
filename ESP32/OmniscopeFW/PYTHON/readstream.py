import sys
import numpy as np
import cv2
import numpy
import serial
import base64


baud = 2000000


def grab_image(conn):
    try:
        conn.write(b"c\n");
        img_buf = conn.readline()
        print(img_buf)
        return img_buf
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
readyPorts = []
for port in ports:
    try:
        mConns[port] = serial.Serial(port, baud, write_timeout=0.5, timeout=0.5)
        readyPorts.append(port)
        print("Connected to " + port)
    except Exception as e:print(e); continue


while True:
    for port in readyPorts:
        conn = mConns[port]
        print("reading from " + port)
       
        buf = grab_image(conn)
        if buf is None:
            continue
        im = buf_to_img(buf)
        if im is None:
            continue
        cv2.imshow(port, im)
        k = cv2.waitKey(10)
