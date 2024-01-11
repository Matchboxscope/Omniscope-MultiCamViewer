import serial.tools.list_ports

ports = serial.tools.list_ports.comports()
mDevices = []
for port in ports:
    mDevice = port.device
    if mDevice.find('modem') >= 0:
        mDevices.append(port.device)
print(mDevices)
print(len(mDevices))