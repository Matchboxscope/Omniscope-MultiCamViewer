const WebSocket = require('ws');
const fs = require('fs');
const path = require('path');
const { log } = require('console');

let sensor;
let command = null;
let validEntities = [];
let counter = 0;
let initialDataReceived;
let resolveInitialData;
// Variable to store the time of the last processed frame
let lastFrameTime = 0;
let frameDelay = 200; // ms
initialDataReceived = new Promise((resolve) => {
	resolveInitialData = resolve;
});


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

process.on('uncaughtException', (error, origin) => {
	console.log('----- Uncaught exception -----');
	console.log(sensor.port);
	console.log(error);
	console.log('----- Exception origin -----');
	console.log(origin);
	console.log('----- Status -----');
});

process.on('unhandledRejection', (reason, promise) => {
	console.log('----- Unhandled Rejection -----');
	console.log(sensor.port);
	console.log(promise);
	console.log('----- Reason -----');
	console.log(reason);
	console.log('----- Status -----');
});

process.on('message', (message) => {
	if (message.update === 'sensor') {
		sensor = message.data;
		console.log('Connection prepared for', sensor);
		
		resolveInitialData();
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
	await initialDataReceived;
	if (!sensor || !sensor.port) {
		console.error('Sensor or sensor port is not defined');
		process.exit();
	}
	// Save an image every minute
	// setInterval(saveImage, 60000);


	const server = new WebSocket.Server({ port: sensor.port }, () => console.log(`WS Server is listening at ${sensor.port}`));
	server.on('connection', (ws) => {
		console.log('Client connected: ' + ws._socket.remoteAddress);
		ws.on('message', async (data) => {
			if (ws.readyState !== ws.OPEN) return;
			
			if (command) {
				ws.send(command);
				command = null;
			}
			
			if (typeof data === 'object') {
				// Get the current time
				const now = Date.now();
		  
				// If enough time has passed since the last frame
				if (now - lastFrameTime >= frameDelay) {
				  // Process the frame
				  let img = Buffer.from(Uint8Array.from(data)).toString('base64');
				  counter++;
				  sensor.image = img;
				  console.log('Frame received from ' + ws._socket.remoteAddress + ' ' + counter);		  
				  // Update the time of the last processed frame
				  lastFrameTime = now;
				}
			  }
			// send to the client the image data
			process.send({ update: 'sensor', data: sensor });
		});
	});
}

main();

