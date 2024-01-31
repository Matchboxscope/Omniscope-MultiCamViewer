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

def flash_device(port):
    command = f"platformio run --target upload --upload-port {port} --verbose"
    subprocess.run(command, shell=True)

def main():
    esp_devices = list_esp_devices()
    print("ESP Devices found:", esp_devices)

    for device in esp_devices:
        print(f"Flashing device on {device}...")
        flash_device(device)
        print(f"Finished flashing {device}")

if __name__ == "__main__":
    main()
