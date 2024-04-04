import serial.tools.list_ports
import subprocess

def list_esp_devices():
    ports = serial.tools.list_ports.comports()
    mDevices = []
    for port in ports:
        mDevice = port.device
        if mDevice.find('modem') >= 0:
            mDevices.append(port.device)
    print(mDevices)

    return mDevices

esp_devices = list_esp_devices()
