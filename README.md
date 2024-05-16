# Omniscope Mutliviewer for the Browser

##### Install:
in your terminal, write 

```
npm install
```

for the Omniscope APP 

```
cd 

```

## Showcase

![](./IMAGES/REACTAPP.gif)

## For the raspberry pi

```bash
sudo apt update
sudo apt upgrade

sudo apt install -y ca-certificates curl GnuPG
curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | sudo gpg --dearmor -o /usr/share/keyrings/nodesource.gpg
NODE_MAJOR=18
echo "deb [signed-by=/usr/share/keyrings/nodesource.gpg] https://deb.nodesource.com/node_$NODE_MAJOR.x nodistro main" | sudo tee /etc/apt/sources.list.d/nodesource.list
sudo apt update
sudo apt install nodejs
node -v
```

```bash
git clone https://github.com/Matchboxscope/Omniscope-MultiCamViewer
cd Omniscope-MultiCamViewer/server
npm install
npm run
```

```bash
cd Omniscope-MultiCamViewer/client
npm install
npm run
```
## Jetson Nano

```bash
sudo apt-get install curl
curl -sL https://deb.nodesource.com/setup_12.x | sudo -E bash -
sudo apt-get install -y nodejs
```

## Electron

```bash
cd electron
npm run electron-start
````
``
```bash
npm install --save-dev electron-builder
npm run dist -- -w
npm run dist -- -m
```


## Ports

The following ports will be used for specific functions: 

```
locahost:12345 - UDP: streams the server IP so that the ESP32 knows where the socketserver is sitting
localhost:8000/ - serving the APP (e.g. localhost:8000) 
localhost:8000/setIPPort - announce the camera ID from the esp32 side: curl -X 'POST' 'http://192.168.0.116:8000/setIPPort' -H 'accept: application/json' -H 'Content-Type: application/json'  -d '{"ip": "192.168.1.1", "port":8001}'
```


## Sensors 

Once a camera connects to the server it announces it's unique 3-digit ID to the socket server. The socket server will create a new camera slot and assings an accessinding ID. This will be stored in the `sensor.json` for later use (e.g. when the server restarts):

```json
{
  "6": {
    "port": 8006,
    "class": "cam-instance",
    "display": "Cam#4",
    "id": "4"
  },
  "65": {
    "port": 8065,
    "class": "cam-instance",
    "display": "Cam#6",
    "id": "6"
  },
  "84": {
    "port": 8084,
    "class": "cam-instance",
    "display": "Cam#3",
    "id": "3"
  },
  "379": {
    "port": 8379,
    "class": "cam-instance",
    "display": "Cam#7",
    "id": "7"
  },
  "386": {
    "port": 8386,
    "class": "cam-instance",
    "display": "Cam#1",
    "id": "1"
  },
  "530": {
    "port": 8530,
    "class": "cam-instance",
    "display": "Cam#8",
    "id": "8"
  },
  "541": {
    "port": 8541,
    "class": "cam-instance",
    "display": "Cam#9",
    "id": "9"
  },
  "613": {
    "port": 8613,
    "class": "cam-instance",
    "display": "Cam#2",
    "id": "2"
  },
  "626": {
    "port": 8626,
    "class": "cam-instance",
    "display": "Cam#12",
    "id": "12"
  },
  "633": {
    "port": 8633,
    "class": "cam-instance",
    "display": "Cam#11",
    "id": "11"
  },
  "742": {
    "port": 8742,
    "class": "cam-instance",
    "display": "Cam#10",
    "id": "10"
  },
  "950": {
    "port": 8950,
    "class": "cam-instance",
    "display": "Cam#5",
    "id": "5"
  }
}
```

## Timelpase Imaging

All camera frames will be stored periodically in the folder: `./server/saved_images`


## Errorhandling

- If a camera (e.g. ESP32 Xiao) loses connection to the Wifi: It'll restart and announces it's ID/Port to the server. It'll use the same camera ID as previously assigned by the Socket Server, hence keeps it's position in the 24 camera grid

- if the socket server connection will break, the camera will automatically restart and try to reconnect 

## Camera Lenses

https://www.largan.com.tw/html/product/alllist/all-list.htm

## Errors:

```
----- Uncaught exception -----
sensor.js:31
RangeError: Invalid WebSocket frame: RSV2 and RSV3 must be clear
    at Receiver.getInfo (/home/bene/Downloads/Omniscope-MultiCamViewer/server/node_modules/ws/lib/receiver.js:171:14)
    at Receiver.startLoop (/home/bene/Downloads/Omniscope-MultiCamViewer/server/node_modules/ws/lib/receiver.js:131:22)
    at Receiver._write (/home/bene/Downloads/Omniscope-MultiCamViewer/server/node_modules/ws/lib/receiver.js:78:10)
    at writeOrBuffer (/home/bene/Downloads/Omniscope-MultiCamViewer/server/lib/internal/streams/writable.js:391:12)
    at _write (/home/bene/Downloads/Omniscope-MultiCamViewer/server/lib/internal/streams/writable.js:332:10)
    at Receiver.Writable.write (/home/bene/Downloads/Omniscope-MultiCamViewer/server/lib/internal/streams/writable.js:336:10)
    at Socket.socketOnData (/home/bene/Downloads/Omniscope-MultiCamViewer/server/node_modules/ws/lib/websocket.js:1162:35)
    at Socket.emit (/home/bene/Downloads/Omniscope-MultiCamViewer/server/lib/events.js:513:28)
    at addChunk (/home/bene/Downloads/Omniscope-MultiCamViewer/server/lib/internal/streams/readable.js:315:12)
    at readableAddChunk (node:internal/streams/readable:289:9) {code: 'WS_ERR_UNEXPECTED_RSV_2_3', stack: 'RangeError: Invalid WebSocket frame: RSV2 and…dChunk (node:internal/streams/readable:289:9)', message: 'Invalid WebSocket frame: RSV2 and RSV3 must be clear', Symbol(status-code): 1002, Symbol(kEnhanceStackBeforeInspector): ƒ}

sensor.js:32
----- Exception origin -----
sensor.js:33
uncaughtException
sensor.js:34
----- Status -----
```
