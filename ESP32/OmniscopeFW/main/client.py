import sys

import cv2
import numpy
import serial


port = '/dev/ttyUSB0'
port = "/dev/cu.usbmodem111101"
if len(sys.argv) > 1:
    port = sys.argv[1]
baud = 2000000


def grab_image(conn):
    conn.write(b"c\n");
    b = conn.read(4)
    assert b == b'img:'
    buf = conn.read_until(b':')
    print(f"read: {buf}")
    n = int(buf[:-1])
    img_buf = conn.read(n)
    return img_buf


def buf_to_img(buf):
    arr = numpy.frombuffer(buf, dtype='uint8')
    return cv2.imdecode(arr, flags=1)


if __name__ == '__main__':
    cv2.namedWindow('win')
    i = 0
    conn = serial.Serial(port, baud)
    while True:
        buf = grab_image(conn)
        im = buf_to_img(buf)
        cv2.imshow('win', im)
        k = cv2.waitKey(10)
        print(k)
        if k == ord('c'):
            cv2.imwrite(f'image_{i}.jpg', im)
            i += 1
            continue
        if k == ord('q'):
            break