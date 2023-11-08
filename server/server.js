const FILE_PATH = "./sensors.json"; // File path for storing IP addresses

const path = require("path");
const express = require("express");
const WebSocket = require("ws");
const cluster = require("cluster");
const os = require("os");
const globalSensorData = require("./global-sensor-data");
const sensors = require(FILE_PATH);
const http = require("http");
const dgram = require("dgram");
const bodyParser = require("body-parser");
const fs = require("fs");

const app = express();
app.use("/static", express.static(path.join(__dirname, "public")));
app.use(bodyParser.json());

const connectedClients = new Set();
const HTTP_PORT = 8000;
const updateFrequency = 50;

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

// lenghts of sensorArray
let nCameras = sensorsArray.length;
const sensorsPerWorker = 1; // Math.ceil(sensorsArray.length / cores);

/*
for (let i = 0; i < nCameras; i++) {
	const workerSensors = sensorsArray.slice(i * sensorsPerWorker, (i + 1) * sensorsPerWorker);
	if (workerSensors.length === 0) continue;
	
	const worker = cluster.fork();
	worker.send({ update: 'sensor', data: workerSensors[0] });
	
	worker.on('message', (message) => {
		if (message.update === 'sensor') {
			updateSensors(message.data);
		}
	});
	
	workers.set(worker, workerSensors[0].port, i);
}
*/

cluster.on("exit", (worker) => {
  console.log(`Worker ${worker.process.pid} killed. Starting new one!`);

  workers.delete(worker);
  const newWorker = cluster.fork();
  const sensor = sensorsArray.slice(
    (workers.size - 1) * Math.floor(sensorsArray.length / os.cpus().length),
    workers.size * Math.floor(sensorsArray.length / os.cpus().length)
  );
  newWorker.send({ update: "sensor", data: sensor });

  newWorker.on("message", (message) => {
    if (message.update === "sensor") {
      updateSensors(message.data);
    }
  });

  workers.set(newWorker, sensor.port);
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
      if (data.operation === "snapAllCameras") {
        for (let [worker, _] of workers) {
          worker.send({ update: "snapImage" });
        }
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

      if (data.operation === "resetAllCameras") {
        // we need to delete the sensors.json file and restart the server
        console.log("Resetting all cameras");
        fs.writeFile(FILE_PATH, JSON.stringify([]), function (err) {
            if (err) throw err;
            console.log("File cleared!");
        });
        // kill all workers
        for (let [worker, _, __] of workers) {
          worker.kill();
          workers.delete(worker);
        }
      }
    } catch (error) {}
  });

  ws.on("close", () => {
    // closing the react app will cause the client to disconnect
    console.log("Client disconnected: " + ws._socket.remoteAddress);
    connectedClients.delete(ws);
  });
});

// IP Broadcasting
const BROADCAST_PORT = 12345;
const BROADCAST_ADDR = "255.255.255.255";

// Function to get server IP
function getServerIP() {
  const ifaces = os.networkInterfaces();
  let serverIP = "127.0.0.1";

  for (let ifname in ifaces) {
    ifaces[ifname].forEach(function (iface) {
      if ("IPv4" !== iface.family || iface.internal !== false) {
        return;
      }
      serverIP = iface.address;
    });
  }

  return serverIP;
}

// Setting up the UDP socket for broadcasting
const server = dgram.createSocket("udp4");

server.bind(function () {
  server.setBroadcast(true);
  console.log(
    `UDP server listening on ${server.address().address}:${
      server.address().port
    }`
  );
  setInterval(broadcastIP, 1000); // Broadcast every 5000 ms
});

function broadcastIP() {
  let message = Buffer.from(getServerIP());
  server.send(
    message,
    0,
    message.length,
    BROADCAST_PORT,
    BROADCAST_ADDR,
    function () {
      //console.log(`Broadcasting IP address: ${message}`);
    }
  );
}

async function handleCamera(port, uniqueCamId) {
  const currentCameraId = await appendIPToFile(port, uniqueCamId);
  console.log(`Current camera ID: ${currentCameraId}`);
  startWorkerForCamera(port, currentCameraId);
}

// Handle POST request on /setIP
////curl -X 'POST' 'http://192.168.43.235:8000/setIPPort' -H 'accept: application/json' -H 'Content-Type: application/json'  -d '{"ip": "192.168.1.1", "port":8001}'
app.post("/setIPPort", (req, res) => {
  console.log("Received POST request on /setIP");
  console.log(req.body);
  const ip = req.body.ip;
  const port = req.body.port;
  if (ip) {
    console.log(`Received IP/port: ${ip}:${port}`);
    const uniqueCamId = port - 8000;
    handleCamera(port, uniqueCamId);
    res.send({ message: "IP received and stored" });
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
        worker.kill();
        workers.delete(worker);
        break;
      }
    }
  }
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

// Append IP to the file
async function appendIPToFile(newPort, newCamId) {
  // create a random ID for the new camera
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
}

app.get("/client", (_req, res) => {
  res.sendFile(path.resolve(__dirname, "./public/pages/client1/client.html"));
});
app.get("/client2", (_req, res) => {
  res.sendFile(path.resolve(__dirname, "./public/pages/client2/client.html"));
});
app.get("/client3", (_req, res) => {
  res.sendFile(path.resolve(__dirname, "./public/client.html"));
});
app.listen(HTTP_PORT, () => {
  console.log(
    `HTTP server starting on ${HTTP_PORT} with process ID ${process.pid}`
  );
});
