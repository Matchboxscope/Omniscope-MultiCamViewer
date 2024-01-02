import serial
import base64
import cv2
import numpy as np
import threading
import time
# Open the serial port
port_names = ["/dev/cu.usbmodem11101"]#, "/dev/cu.usbmodem11201", "/dev/cu.usbmodem11301"]
baud_rate = 2000000

print("Reading data...")
frame_count = 0
ser = {}
isRunning = True


for port_name in port_names:
    ser[port_name] = serial.Serial(port_name, baud_rate)
 
mImages = {}
for port_name in port_names:
    mImages[port_name] = np.zeros((480, 640, 3), np.uint8)
    
def readFrames(serial_port, name):
    while isRunning:
        # Read data from serial port
        if serial_port.in_waiting:
            buffer = serial_port.readline()

            # Check for line break indicating end of frame
            if b'\n' in buffer:
                line_break_pos = buffer.find(b'\n')
                encoded_frame = buffer[:line_break_pos]
                buffer = buffer[line_break_pos+1:]

                try:
                    # Decode the Base64-encoded frame
                    decoded_frame = base64.b64decode(encoded_frame)

                    # Convert to a NumPy array and decode image
                    nparr = np.frombuffer(decoded_frame, np.uint8)
                    image = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

                    if image is not None:                    
                        mImages[port_name] = image

                        # Optionally, save the frame to a file
                        # filename = f"frame_{frame_count}.jpg"
                        # cv2.imwrite(filename, image)

                except Exception as e:
                    print("Error decoding frame"+str(e))

allThreads = []
for port_name in port_names:
    allThreads.append(threading.Thread(target=readFrames, args=(ser[port_name], port_name,)))
    allThreads[-1].start()


while 1:
    
    for port_name in port_names:
        cv2.imshow(port_name, mImages[port_name])
        cv2.waitKey(1)



time.sleep(100)
isRunning = False

for thread in allThreads:
    thread.join()
         
# Close the port
ser.close()
