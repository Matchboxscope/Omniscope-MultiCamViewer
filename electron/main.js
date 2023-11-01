const { app, BrowserWindow } = require('electron');
const { fork, spawn } = require('child_process');
const path = require('path');

let mainWindow;
let serverProcess = null;
let reactProcess = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: true
    }
  });

  // The URL should be where your React app will be served from.
  // This is the default create-react-app port.
  
  setTimeout(() => {
    mainWindow.loadURL('http://localhost:3000');
  }, 2000); // 10 seconds delay

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

app.whenReady().then(() => {
  // Start the Node.js server
  serverProcess = fork(path.join(__dirname, '../server/server.js'));

  // Start the React client app
  reactProcess = spawn('npm', ['start'], { cwd: path.join(__dirname, '../client'), stdio: 'inherit' });

  createWindow();
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});

app.on('quit', () => {
  // Terminate the server and React app when closing the Electron app
  if (serverProcess) serverProcess.kill();
  if (reactProcess) reactProcess.kill();
});

app.on('activate', () => {
  if (mainWindow === null) createWindow();
});
