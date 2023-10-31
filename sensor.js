const WebSocket = require('ws');
const fluidb = require('fluidb');
const fs = require('fs');
const path = require('path');

let sensor;
let command = null;
let validEntities = [];
let counter = 0;
let initialDataReceived;
let resolveInitialData;

initialDataReceived = new Promise((resolve) => {
	resolveInitialData = resolve;
});

fs.readdir('./images', { withFileTypes: true }, (err, files) => {
	if (err) {
		console.error(err);
		return;
	}
	validEntities = files.filter(file => file.isDirectory()).map(folder => folder.name);
});

process.on('uncaughtException', (error, origin) => {
	console.log('----- Uncaught exception -----');
	console.log(error);
	console.log('----- Exception origin -----');
	console.log(origin);
	console.log('----- Status -----');
});

process.on('unhandledRejection', (reason, promise) => {
	console.log('----- Unhandled Rejection -----');
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
        const filepath = path.join(__dirname, 'saved_images', filename);

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
	if (!sensor) {
		process.exit();
	}
	// Save an image every minute
	setInterval(saveImage, 60000);

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
				let img = Buffer.from(Uint8Array.from(data)).toString('base64');
				counter++;
				sensor.image = img;
			} else {
				const commandRegex = /\(c:(.*?)\)/g;
				const sensorRegex = /\(s:(.*?)\)/g;
				let match;
				
				while ((match = commandRegex.exec(data))) {
					const keyValuePairs = match[1];
					const pairs = keyValuePairs.trim().split(/\s*,\s*/);
					
					for (const pair of pairs) {
						const [key, value] = pair.split("=");
						const commandFind = sensor.commands.find(c => c.id === key);
						if (commandFind) {
							commandFind.state = value;
						}
					}
				}
				
				const sensorsObj = {};
				while ((match = sensorRegex.exec(data))) {
					const keyValuePairs = match[1];
					const pairs = keyValuePairs.trim().split(/\s*,\s*/);
					
					for (const pair of pairs) {
						const [key, value] = pair.split("=");
						sensorsObj[key] = value;
					}
				}
				
				sensor.sensors = sensorsObj;
			}
			
			process.send({ update: 'sensor', data: sensor });
		});
	});
}

main();

