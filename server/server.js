// if you use docker: sudo pkill docker-pr
// check ports: sudo lsof -iTCP -sTCP:LISTEN
const path = require("path");
const express = require("express");
const WebSocket = require("ws");
const cluster = require("cluster");
const os = require("os");
const globalSensorData = require("./global-sensor-data");
const { Mutex } = require('async-mutex');
const mutex = new Mutex();

const http = require("http");
const dgram = require("dgram");
const bodyParser = require("body-parser");
const fs = require("fs");

// Load the sensor configurations if available
const FILE_PATH = "sensors.json"; // Specify the path to your JSON file
let sensors;
try {
  // Try to read the JSON file
  sensors = require(path.resolve(FILE_PATH));
} catch (err) {
  // If the file does not exist or is not valid JSON, initialize with an empty object
  sensors = {};
  fs.writeFileSync(FILE_PATH, JSON.stringify(sensors));
}


const app = express();
app.use("/static", express.static(path.join(__dirname, "public")));
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, '../client/build')));

const connectedClients = new Set();
const HTTP_PORT = 8000;
const updateFrequency = 200;

// Define the maximum number of child processes
const maxChildProcesses = 24; // Replace with your desired limit

// Initialize the child process count
let childProcessCount = 0;

// For moving stage and turning on/off light
let stageSocket = null;


cluster.setupPrimary({ exec: path.join(__dirname, "sensor.js") });
const workers = new Map();

const sensorsArray = Object.entries(sensors).map(([key, value]) => ({
  key,
  ...value,
}));
Object.assign(
  globalSensorData,
  Object.fromEntries(sensorsArray.map((sensor) => [sensor.key, sensor]))
);

// Assuming `workers` is a Map where the keys are worker IDs and the values are worker objects
function deleteWorker(workerId) {
  const worker = workers.get(workerId);
  if (worker) {
    // Remove all event listeners attached to the worker
    worker.removeAllListeners();

    // If the worker is a child process, kill it
    if (typeof worker.kill === 'function') {
      worker.kill();
    }

    // Delete the worker from the Map
    workers.delete(workerId);
  }
}

cluster.on("exit", (worker) => {
  console.log(`Worker ${worker.process.pid} killed!`);
  // Decrement the child process count
  childProcessCount--;
  deleteWorker(worker.id);
});

function updateSensors(updatedSensor) {
  globalSensorData[updatedSensor.key] = updatedSensor;
}

const wss = new WebSocket.Server({ port: 8999 }, () =>
  console.log("WS Server is listening at 8999")
);

setInterval(() => {
  // here we send the data to the frontend including device ID and image data
  for (const client of connectedClients) {
    if (client.readyState === WebSocket.OPEN) {
      // this has to be interpreted by the cameragrid component
      client.send(JSON.stringify({ devices: Object.values(globalSensorData) }));
    }
  }
}, updateFrequency);

wss.on("connection", (ws) => {
  /*
    This is the communciation channel between the frontend and the backend.
    The frontend sends a message to the backend, which is then parsed and
    the corresponding action is taken.
    */
  connectedClients.add(ws);
  console.log("Client connected: " + ws._socket.remoteAddress);

  ws.on("message", async (data) => {
    if (ws.readyState !== ws.OPEN) return;

    try {
      data = JSON.parse(data);

      if (data.operation === "function") {
        const sensorToUpdate = sensorsArray.find(
          (sensor) => sensor.key === data.command.recipient
        );

        if (sensorToUpdate) {
          const targetWorker = [...workers.entries()].find(
            ([, port]) => port === sensorToUpdate.port
          )?.[0];
          if (targetWorker) {
            targetWorker.send({
              update: "command",
              data: `${data.command.message.key}=${data.command.message.value}`,
            });
          }
        }
      }
      if (data.operation == "snapAllCameras") {
        await new Promise((resolve) => setTimeout(resolve, 500));
        sendStageCommand('ILLUMINATION=100');

        for (let [worker, _] of workers) {
          worker.send({ update: "snapImage" });

        }
        await new Promise((resolve) => setTimeout(resolve, 500));
        sendStageCommand('ILLUMINATION=0');

      }
      if (data.operation === "reloadCameras") {
        // reannounce all cameras that are stored in the sensors.json file
        console.log("Reloading cameras");
        for (let [worker, port, cameraId] of workers) {
          worker.send({
            update: "sensor",
            data: {
              key: cameraId,
              port: port,
              class: "cam-instance",
              display: "Cam#" + String(cameraId),
              commands: Array(1),
            },
          });
        }
      }

      if (data.operation == "resetAllCameras") {
        // we need to delete the sensors.json file and restart the server
        console.log("Resetting all cameras");
        fs.writeFile(FILE_PATH, JSON.stringify([]), function (err) {
          if (err) throw err;
          console.log("File cleared!");
        });
        // kill all workers
        for (let [worker, _, __] of workers) {
          worker.kill();
          deleteWorker(worker.id);
          childProcessCount=0;
        }
      }

      if (data.operation == "stageMove") {
        // we need to send a message to the stage worker to move the stage up
        console.log("Moving stage up");
        // To send a command
        command = "MOVE_FOCUS=" + String(data.value);
        sendStageCommand(command);

      }

      if (data.operation == "turnIlluminationON") {
        // we need to send a message to the stage worker to turn on the illumination
        // To send a command
        sendStageCommand('ILLUMINATION=100');
      }

      if (data.operation == "turnIlluminationOFF") {
        // we need to send a message to the stage worker to turn off the illumination
        // To send a command
        sendStageCommand('ILLUMINATION=0');
      }

    } catch (error) { }
  });

  ws.on("close", () => {
    // closing the react app will cause the client to disconnect
    console.log("Client disconnected: " + ws._socket.remoteAddress);
    connectedClients.delete(ws);
    childProcessCount--;
  });
});

// IP Broadcasting
const BROADCAST_PORT = 12345;
const BROADCAST_ADDR = "255.255.255.255";

// Function to get server IP
function getServerIP(desiredInterfaceName) {
  const ifaces = os.networkInterfaces();
  // get all names of the ifaces
  let serverIP = "127.0.0.1";

  for (let ifname in ifaces) {
    if (ifname === desiredInterfaceName) {
      const iface = ifaces[ifname].find((iface) => iface.family === "IPv4" && !iface.internal);
      if (iface) {
        serverIP = iface.address;
        //console.log(`Server IP: ${serverIP}`);
        break; // Found the desired interface, exit the loop
      }
    }
  }

  return serverIP;
}




let desiredInterfaceName;

switch (os.platform()) {
  case 'win32':
    // Windows
    desiredInterfaceName = 'Wi-Fi';
    break;
  case 'darwin':
    // MacOS
    desiredInterfaceName = 'en0'; // This might change based on your system
    break;
  case 'linux':
    // Linux
    desiredInterfaceName = 'eth0'; // wlan0'; // This might change based on your system
    break;
  default:
    console.log('Unsupported platform');
    break;
}
// Create a UDP socket bound to the desired network interface's local address
const server = dgram.createSocket("udp4");
server.bind({
  address: getServerIP(desiredInterfaceName),
  port: 0, // Let the OS choose an available port
});

server.on("listening", function () {
  server.setBroadcast(true);
  console.log(
    `UDP server listening on ${server.address().address}:${server.address().port}`
  );
  setInterval(broadcastIP, 1000); // Broadcast every 1000 ms
});

function broadcastIP() {
  const message = Buffer.from(getServerIP(desiredInterfaceName));
  // console.log(`Broadcasting IP address: ${message}`);
  server.send(
    message,
    0,
    message.length,
    BROADCAST_PORT,
    BROADCAST_ADDR,
    function () {
      // Broadcast callback
    }
  );
}

async function handleCamera(port, uniqueCamId) {
  const currentCameraId = await appendIPToFile(port, uniqueCamId);
  console.log(`Current camera ID: ${currentCameraId}`);
  startWorkerForCamera(port, currentCameraId);
}


function handleStage(port, uniqueCamId) {
  const stagewss = new WebSocket.Server({ port: port });

  stagewss.on('connection', function connection(ws) {
    console.log('Stage connected');
    stageSocket = ws;

    ws.on('message', function incoming(message) {
      console.log('Received message from stage:', message);
      // Handle incoming messages as necessary
    });

    ws.on('close', function close() {
      console.log('Stage disconnected');
      stageSocket = null;
    });
  });

  stagewss.on('listening', () => {
    console.log(`Stage WebSocket Server is listening on port ${port}`);
  });

  stagewss.on('error', function error(err) {
    console.error('Stage WebSocket Server error:', err);
  });
}

function sendStageCommand(command) {
  if (stageSocket && stageSocket.readyState === WebSocket.OPEN) {
    stageSocket.send(command, (err) => {
      if (err) {
        console.error('Error sending command to stage:', err);
      } else {
        console.log(`Command sent to stage: ${command}`);
      }
    });
  } else {
    console.log('No stage connection available to send command.');
  }
}

// Handle POST request on /setIP
////curl -X 'POST' 'http://192.168.0.116:8000/setIPPort' -H 'accept: application/json' -H 'Content-Type: application/json'  -d '{"ip": "192.168.1.1", "port":8001}'
app.post("/setIPPort", (req, res) => {
  const ip = req.body.ip;
  const port = req.body.port;
  if (ip) {
    //console.log(`Received IP/port: ${ip}:${port}`);
    const uniqueCamId = port - 8000;
    if (uniqueCamId < 0) { // stage will have a negative ID
      handleStage(port, uniqueCamId);
      // Initialize the stage WebSocket server
      //handleStage(8005, -1); // Example port and uniqueCamId

      res.send({ message: "IP received and stored" });
    }
    else {
      handleCamera(port, uniqueCamId);
      res.send({ message: "IP received and stored" });
    }
  } else {
    res.status(400).send({ message: "Invalid IP" });
  }
});

// Start a new worker for the camera
function startWorkerForCamera(port, cameraId) {
  // if workers has port/cameraID of the new incoming one, kill the one with same port/cameraID
  const isPortInWorkers = Array.from(workers.values()).some(
    (workerPort) => workerPort === port
  );

  if (isPortInWorkers) {
    for (let [worker, oldport] of workers) {
      if (oldport === port) {
        deleteWorker(worker.id);
        childProcessCount--;
        break;
      }
    }
  }

  if (childProcessCount < maxChildProcesses) {
    // Increment the child process count
    childProcessCount++;

    // Start a new worker
    const worker = cluster.fork();
    worker.send({
      update: "sensor",
      data: {
        key: cameraId,
        port: port,
        class: "cam-instance",
        display: "Cam#" + String(cameraId),
      },
    });
    worker.on("message", (message) => {
      if (message.update === "sensor") {
        updateSensors(message.data);
      }
    });
    workers.set(worker, port, cameraId);
    console.log(`Started worker for camera ${cameraId} on port ${port}`);

  }
  else {
    console.log("Max child processes reached");
  }
}



async function appendIPToFile(newPort, newCamId) {
  // Lock the function
  const release = await mutex.acquire();

  try {
    console.log("Appending to file");
    let currentCameraId = 0;
    let data = await fs.promises.readFile(FILE_PATH, "utf8");
    let sensors = !data ? {} : JSON.parse(data);
    // Add new camera if it doesn't already exist
    if (!sensors[newCamId]) {
      currentCameraId = Object.keys(sensors).length + 1;
      console.log(`New camera ${newCamId} added to ${FILE_PATH}`);
      sensors[newCamId] = {
        port: newPort,
        class: "cam-instance",
        display: `Cam#${currentCameraId}`,
        id: `${currentCameraId}`,
      };
    } else {
      currentCameraId = sensors[newCamId].id;
      console.log(
        `Camera ${newCamId} already exists in ${FILE_PATH} with ID ${currentCameraId}`
      );
      // Update IP if camera already exists
      sensors[newCamId].port = newPort;
    }
    // Write to file
    await fs.promises.writeFile(FILE_PATH, JSON.stringify(sensors, null, 2));
    console.log(
      `New camera ${newCamId} with IP ${newPort} added to ${FILE_PATH}`
    );
    return currentCameraId;
  } finally {
    // Release the lock
    release();
  }
}

app.get('*', (req, res) => {
  res.sendFile(path.join(__dirname, '../client/build', 'index.html'));
});

// send a simple test response if the server is up and running
app.get("/client", (_req, res) => {
  res.send("Server is up and running!");
});

app.listen(HTTP_PORT, () => {
  console.log(
    `HTTP server starting on ${HTTP_PORT} with process ID ${process.pid}`
  );
});
