import subprocess

import serial.tools.list_ports

ports = serial.tools.list_ports.comports()
mDevices = []
for port in ports:
    mDevice = port.device
    if mDevice.find('modem') >= 0:
        mDevices.append(port.device)
print(mDevices)
print(len(mDevices))

asdf
concatenatedDevices = " ".join(mDevices)

#mDevices = mDevices[0:2]  # Limit to the first two devices

# Path to your compiled C++ application
app_path = './app'

# Launch an instance of the app for each serial port
command = [app_path,  concatenatedDevices]
process = subprocess.Popen(command)
process.wait()

print('All processes completed.')
