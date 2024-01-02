//npm install express ws serialport
const express = require("express");
const WebSocket = require("ws");
const http = require("http");
const { SerialPort } = require("serialport");
const { ReadlineParser } = require("@serialport/parser-readline");

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });


const port = new SerialPort({
  path: "/dev/cu.usbmodem11101",
  baudRate: 2000000,
});
const parser = port.pipe(new ReadlineParser({ delimiter: "\n" }));

// function to write to serial port
function requestImage() {
    port.write('c\n', (err) => {
      if (err) {
        return console.log('Error on write: ', err.message);
      }
      console.log('message written');
    });
  }
  

app.use(express.static("public"));

// Read the port data
port.on('open', () => {
    console.log('Port is open');
    setInterval(() => {
      requestImage();
    }, 50); // Sends 'c\n' every 100ms
  });

parser.on("data", (data) => {
  //console.log(data);
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
});

wss.on("connection", (ws) => {
  console.log("Client connected");
});

server.listen(3000, () => {
  console.log("Server started on http://localhost:3000");
});

// we also want to serve the index.html
app.get("/", (req, res) => {
  res.sendFile(__dirname + "/index.html");
});
