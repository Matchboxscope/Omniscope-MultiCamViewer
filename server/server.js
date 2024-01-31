// if you use docker: sudo pkill docker-pr
// check ports: sudo lsof -iTCP -sTCP:LISTEN
const path = require("path");
const express = require("express");
const WebSocket = require("ws");
const cluster = require("cluster");
const os = require("os");
const globalSensorData = require("./global-sensor-data");
const { Mutex } = require("async-mutex");
const mutex = new Mutex();
const { SerialPort } = require("serialport");

const http = require("http");
const dgram = require("dgram");
const bodyParser = require("body-parser");
const fs = require("fs");
const HTTP_PORT = 8000;
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

// launch the backend server for the react app
const app = express();
app.use("/static", express.static(path.join(__dirname, "public")));
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, "../client/build")));

const connectedClients = new Set();
const updateFrequency = 200;

// Define the maximum number of child processes
const maxChildProcesses = 24; // Replace with your desired limit

// Initialize the child process count
let childProcessCount = 0;

// For moving stage and turning on/off light
let stageSocket = null;

// attach camera sensors
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

// for all ports, launch the worker
//handleStage(port, uniqueCamId);

if (0) {
  const ports = [
    "/dev/cu.usbmodem1111101",
    "/dev/cu.usbmodem1111301",
    "/dev/cu.usbmodem1112201",
    "/dev/cu.usbmodem1112401",
    "/dev/cu.usbmodem1112301",
    "/dev/cu.usbmodem1113101",
    "/dev/cu.usbmodem1113401",
    "/dev/cu.usbmodem1111401",
    "/dev/cu.usbmodem1113201",
    "/dev/cu.usbmodem1112101",
    "/dev/cu.usbmodem1113301",
    "/dev/cu.usbmodem1111201",
  ];
  for (let i = 0; i < ports.length; i++) {
    const port = ports[i];
    const uniqueCamId = i + 1;
    handleCamera(port, uniqueCamId);
  }
}
var portList = [];

SerialPort.list().then((ports) => {
  portList = ports.map((port) => {
    return {
      path: port.path,
      manufacturer: port.manufacturer,
    };
  });
  console.log("Available Serial Ports:", portList);

  let filteredPortList = portList.filter((port) =>
    port.path.startsWith("/dev/tty.usbmodem")
  );

  // Now, portList contains all the serial port information
  // start the workers for all the cameras
  for (let i = 0; i < filteredPortList.length; i++) {
    const port = filteredPortList[i].path;
    const uniqueCamId = i + 1;
    handleCamera(port, uniqueCamId);
  }
});

// Assuming `workers` is a Map where the keys are worker IDs and the values are worker objects
function deleteWorker(workerId) {
  const worker = workers.get(workerId);
  if (worker) {
    // Remove all event listeners attached to the worker
    worker.removeAllListeners();
    console.log(`Killing worker ${workerId}`);
    // If the worker is a child process, kill it
    if (typeof worker.kill === "function") {
      worker.kill();
    }

    // Delete the worker from the Map
    const mPort = worker.port; // store for later 
    const mCameraId = worker.cameraId; // store for later
    workers.delete(workerId);
    // spin up a new worker for the same port and cameraId
    handleCamera(port, uniqueCamId);

    // now we would need to try to reassign the camera to another worker
    // the port would be the same, let's try to create a new worker
    
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
        sendStageCommand("ILLUMINATION=100");

        for (let [worker, _] of workers) {
          worker.send({ update: "snapImage" });
        }
        await new Promise((resolve) => setTimeout(resolve, 500));
        sendStageCommand("ILLUMINATION=0");
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
          childProcessCount = 0;
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
        sendStageCommand("ILLUMINATION=100");
      }

      if (data.operation == "turnIlluminationOFF") {
        // we need to send a message to the stage worker to turn off the illumination
        // To send a command
        sendStageCommand("ILLUMINATION=0");
      }
    } catch (error) {}
  });

  ws.on("close", () => {
    // closing the react app will cause the client to disconnect
    console.log("Client disconnected: " + ws._socket.remoteAddress);
    connectedClients.delete(ws);
    childProcessCount--;
  });
});

async function handleCamera(port, uniqueCamId) {
  console.log(`Current camera ID: ${uniqueCamId}`);
  startWorkerForCamera(port, uniqueCamId);
}

function handleStage(port, uniqueCamId) {
  const stagewss = new WebSocket.Server({ port: port });

  stagewss.on("connection", function connection(ws) {
    console.log("Stage connected");
    stageSocket = ws;

    ws.on("message", function incoming(message) {
      console.log("Received message from stage:", message);
      // Handle incoming messages as necessary
    });

    ws.on("close", function close() {
      console.log("Stage disconnected");
      stageSocket = null;
    });
  });

  stagewss.on("listening", () => {
    console.log(`Stage WebSocket Server is listening on port ${port}`);
  });

  stagewss.on("error", function error(err) {
    console.error("Stage WebSocket Server error:", err);
  });
}

function sendStageCommand(command) {
  if (stageSocket && stageSocket.readyState === WebSocket.OPEN) {
    stageSocket.send(command, (err) => {
      if (err) {
        console.error("Error sending command to stage:", err);
      } else {
        console.log(`Command sent to stage: ${command}`);
      }
    });
  } else {
    console.log("No stage connection available to send command.");
  }
}

// Start a new worker for the camera
async function startWorkerForCamera(port, cameraId) {
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
    // add metadata
    worker.port=port;
    worker.cameraId=cameraId;
    worker.key=cameraId;

    worker.on("message", (message) => {
      if (message.update === "sensor") {
        if (message.data.image==-1) {
          console.log("Image reception timed out, Killing worker.");
          deleteWorker(worker.id);
        }
        else{
          updateSensors(message.data);
        }
      }
    });
    workers.set(worker, port, cameraId);
    console.log(`Started worker for camera ${cameraId} on port ${port}`);
  } else {
    console.log("Max child processes reached");
  }
}

app.get("*", (req, res) => {
  res.sendFile(path.join(__dirname, "../client/build", "index.html"));
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
