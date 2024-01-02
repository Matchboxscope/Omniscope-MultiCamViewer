import sys

import cv2
import numpy
import serial


port = '/dev/cu.usbmodem11101'
if len(sys.argv) > 1:
    port = sys.argv[1]
baud = 2000000


def grab_image(conn):
    try:
        conn.write(b"c\n");
        b = conn.read(4)
        assert b == b'img:'
        buf = conn.read_until(b':')
        print(f"read: {buf}")
        n = int(buf[:-1])
        img_buf = conn.read(n)
        return img_buf
    except Exception as e:
        print(conn.port + str(e))
        return None


def buf_to_img(buf):
    arr = numpy.frombuffer(buf, dtype='uint8')
    return cv2.imdecode(arr, flags=1)

mConns = {}
ports = ['/dev/cu.usbmodem11101', '/dev/cu.usbmodem11201', '/dev/cu.usbmodem11301', '/dev/cu.usbmodem11401']
if __name__ == '__main__':
    cv2.namedWindow('win')
    i = 0
    for port in ports:
        mConns[port] = serial.Serial(port, baud)
    
    while True:
        for port in ports:
            conn = mConns[port]
            
            buf = grab_image(conn)
            if buf is None:
                continue
            im = buf_to_img(buf)
            cv2.imshow(port, im)
            k = cv2.waitKey(10)
            print(k)
            if k == ord('c'):
                #cv2.imwrite(f'image_{i}.jpg', im)
                i += 1
                continue
            if k == ord('q'):
                break