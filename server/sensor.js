const WebSocket = require('ws');
const fs = require('fs');
const path = require('path');
const { SerialPort } = require("serialport");
const { ReadlineParser } = require("@serialport/parser-readline");

let sensor;
let command = null;
let port;
let parser;
let validEntities = [];
let counter = 0;
let lastFrameTime = 0;
let frameDelay = 200; // ms

const imagesPath = path.join(__dirname, 'images');
if (!fs.existsSync(imagesPath)) {
    fs.mkdirSync(imagesPath);
}

fs.readdir(imagesPath, { withFileTypes: true }, (err, files) => {
    if (err) {
        console.error(err);
        return;
    }
    validEntities = files.filter(file => file.isDirectory()).map(folder => folder.name);
});

const wss = new WebSocket.Server({ port: 8080 }, () => console.log(`WS Server is listening at port 8080`));

wss.on('connection', (ws) => {
    console.log('Client connected');
    ws.on('message', (message) => {
        console.log('Received message:', message);
        // Process incoming messages from clients
    });
});

process.on('message', (message) => {
    if (message.update === 'sensor') {
        sensor = message.data;
        console.log('Connection prepared for', sensor);

        if (port) {
            // Close existing port if open
            port.close();
        }

        // Initialize serial port
        port = new SerialPort({
            path: sensor.port,
            baudRate: 2000000,
        });

        parser = port.pipe(new ReadlineParser({ delimiter: "\n" }));

        parser.on("data", (data) => {
            // Process the data and send it to the WebSocket clients
			let img = data;//Buffer.from(Uint8Array.from(data)).toString('base64');
			counter++;
			sensor.image = img;
			process.send({ update: 'sensor', data: sensor });
			
        });
    } else if (message.update === 'command') {
        command = message.data;
        console.log('Command to be executed for', sensor);
    }
    else if (message.update === 'snapImage') {
        saveImage();
    }
});

// Function to save the image
function saveImage() {
    if (sensor && sensor.image) {
        // Format the current date and time for the filename
        const timestamp = new Date().toISOString().replace(/[:.-]/g, '_');
        const filename = `Camera_${sensor.key}_${timestamp}.jpg`;
		const folderpath = path.join(__dirname, 'saved_images');
        const filepath = path.join(folderpath, filename);
		if (!fs.existsSync(folderpath)) {
			fs.mkdirSync(folderpath);
		}
        // Convert base64 image to binary data
        const buffer = Buffer.from(sensor.image, 'base64');

        // Write the file to disk
        fs.writeFile(filepath, buffer, { encoding: null }, (err) => {
            if (err) {
                return console.error('Failed to save image:', err);
            }
            console.log(`Saved image: ${filename}`);
        });
    }
}

async function main() {
    console.log('Application started');
}

main();

