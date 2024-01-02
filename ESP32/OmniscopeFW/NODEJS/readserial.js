const { SerialPort } = require('serialport');
const { DelimiterParser } = require('@serialport/parser-delimiter');

const port = new SerialPort({
  path: '/dev/cu.usbmodem11101',
  baudRate: 2000000
});

const parser = port.pipe(new DelimiterParser({ delimiter: ':' }));

let imageBuffer = Buffer.alloc(0);

// function to write to serial port
function requestImage() {
  port.write('c\n', (err) => {
    if (err) {
      return console.log('Error on write: ', err.message);
    }
    console.log('message written');
  });
}

port.on('open', () => {
  console.log('Port is open');
  setInterval(() => {
    requestImage();
  }, 10000); // Sends 'c\n' every 100ms
});

parser.on('data', (data) => {
  console.log(data.toString());
  /*
  if (data.toString().startsWith('img')) {
    let size = parseInt(data.toString().slice(4), 10);
    if (!size > 0 ) {
      console.log('Image size is 0');
      return;
    }
    imageBuffer = Buffer.alloc(size);
  } else {
    imageBuffer = Buffer.concat([imageBuffer, data]);
    if (imageBuffer.length === imageBuffer.capacity) {
      // Full image is received here
      console.log('Image received');
      // Handle the full image buffer here
    }
  }
  */
});

port.on('error', (err) => {
  console.error('Error: ', err.message);
});
