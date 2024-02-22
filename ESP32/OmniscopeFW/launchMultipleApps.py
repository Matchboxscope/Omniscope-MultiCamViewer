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


# Path to your compiled C++ application
app_path = './ESP32/OmniscopeFW/app'

# Launch an instance of the app for each serial port
processes = []
for port in mDevices:
    command = [app_path, '-s', port]
    process = subprocess.Popen(command)
    processes.append(process)
    print(f'Launched app with {port}')

# Optionally, wait for all processes to complete
for process in processes:
    process.wait()

print('All processes completed.')
