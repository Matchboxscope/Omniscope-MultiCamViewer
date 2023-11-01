//npm run electron-start
const { app, BrowserWindow } = require('electron');
const path = require('path');
const { fork } = require('child_process');

let mainWindow;
let serverProcess = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: true
    }
  });

  // Point to where your React app is hosted
  mainWindow.loadURL('http://localhost:3000');

  mainWindow.on('closed', function () {
    mainWindow = null;
  });
}

app.on('ready', () => {
  // Start the Node.js server
  serverProcess = fork(path.join(__dirname, '../server/server.js'));

  createWindow();
});

app.on('window-all-closed', function () {
  if (process.platform !== 'darwin') app.quit();
});

app.on('quit', () => {
  // Terminate the server when closing the app
  if (serverProcess) serverProcess.kill();
});

app.on('activate', function () {
  if (mainWindow === null) createWindow();
});
