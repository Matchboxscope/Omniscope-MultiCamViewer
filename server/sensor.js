const WebSocket = require("ws");
const fs = require("fs");
const path = require("path");
const { SerialPort } = require("serialport");
const { ReadlineParser } = require("@serialport/parser-readline");

let sensor;
let command = null;
let port;
let parser;
let counter = 0;
let frameDelay = 200; // ms
let imageReceived = false; // Track if image has been received


const imagesPath = path.join(__dirname, "images");
if (!fs.existsSync(imagesPath)) {
  fs.mkdirSync(imagesPath);
}

fs.readdir(imagesPath, { withFileTypes: true }, (err, files) => {
  if (err) {
    console.error(err);
    return;
  }
  validEntities = files
    .filter((file) => file.isDirectory())
    .map((folder) => folder.name);
});

const wss = new WebSocket.Server({ port: 8080 }, () => {});

wss.on("connection", (ws) => {
  console.log("Client connected");
  ws.on("message", (message) => {
    console.log("Received message:", message);
  });
});

// Add a new function to handle the timeout scenario
function handleImageTimeout() {
    console.error("Image reception timed out.");
    sensor.image = -1;
    process.send({ update: "sensor", data: sensor });
  }
  

// function to write to serial port
function requestImage() {
  //console.log("Requesting image from", sensor.port, sensor.key);
  port.write("c\n", (err) => {
    if (err) {
      return console.log("Error on write: ", err.message);
    }
  });
      // Start a timeout to wait for the image
      setTimeout(() => {
        if (!imageReceived) {
          // If no image has been received by the time the timeout expires
          handleImageTimeout();
        }
      }, 1000); // Wait for 1 second (1000 milliseconds)
}

process.on("message", (message) => {
  if (message.update === "sensor") {
    sensor = message.data;
    //console.log('Connection prepared for', sensor);

    if (port) {
      // Close existing port if open
      port.close();
    }

    // Initialize serial port
    port = new SerialPort({
      path: sensor.port,
      baudRate: 2000000,
    });
    // Initialize parser to read lines as frames
    parser = port.pipe(new ReadlineParser({ delimiter: "\n" }));

    // open the port
    port.on("open", () => {
      console.log("Port " + port.path + " is open");
      setInterval(() => {
        requestImage();
      }, frameDelay + Math.floor(Math.random() * 100)); // Sends 'c\n' every 100ms
    });
    // Read data from the serial port
    parser.on("data", (data) => {
      // Process the data and send it to the WebSocket clients
      let img = data; //Buffer.from(Uint8Array.from(data)).toString('base64');
      counter++;

      // check if we have a valid jpeg image
      if (img[0] == "/") {
        //saveImage();
        imageReceived = true; // Mark as image received to cancel the timeout
        sensor.image = img;
        process.send({ update: "sensor", data: sensor });
        //console.log("Image received from", sensor.key, counter);
      } else {
        console.log("Invalid image received from", sensor.port, data);
      }
    });
  } else if (message.update === "command") {
    command = message.data;
    console.log("Command to be executed for", sensor);
  } else if (message.update === "snapImage") {
    saveImage();
  }
});

// Function to save the image
function saveImage() {
  if (sensor && sensor.image) {
    // Format the current date and time for the filename
    const timestamp = new Date().toISOString().replace(/[:.-]/g, "_");
    const filename = `Camera_${sensor.key}_${timestamp}.jpg`;
    const folderpath = path.join(__dirname, "saved_images");
    const filepath = path.join(folderpath, filename);
    if (!fs.existsSync(folderpath)) {
      fs.mkdirSync(folderpath);
    }
    // Convert base64 image to binary data
    const buffer = Buffer.from(sensor.image, "base64");

    // Write the file to disk
    fs.writeFile(filepath, buffer, { encoding: null }, (err) => {
      if (err) {
        return console.error("Failed to save image:", err);
      }
      console.log(`Saved image: ${sensor.port}`); // Port number: $(sensor.port)` );
    });
  }
}

async function main() {}

main();
