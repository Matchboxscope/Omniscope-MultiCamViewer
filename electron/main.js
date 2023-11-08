const { app, BrowserWindow } = require("electron");
const { fork, spawn } = require("child_process");
const path = require("path");
const isDev = require("electron-is-dev");



let mainWindow;
let serverProcess = null;
let reactProcess = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: true,
      webSecurity: false // Disable only for development!
    },
  });
  mainWindow.webContents.openDevTools();

  const startUrl = isDev
    ? "http://localhost:3000" // Dev server URL
    : `file://${path.join(__dirname, "../client/build/index.html")}`; // Static build for production
    // Implement a more robust check to wait for the Omniscope APP to be available
  
  
    const waitUntilReactIsReady = (url, retries = 5, interval = 2000) => {
    fetch(url)
      .then(res => {
        if (res.ok) mainWindow.loadURL(url); // If the server is up, load the URL
        else if (retries > 0) {
          // If the server is not up, try again after an interval
          setTimeout(() => waitUntilReactIsReady(url, retries - 1, interval), interval);
        }
      })
      .catch(() => {
        if (retries > 0) {
          setTimeout(() => waitUntilReactIsReady(url, retries - 1, interval), interval);
        }
      });
  };
  

  waitUntilReactIsReady(startUrl);
  //mainWindow.loadURL('http://localhost:3000')
  console.log("Loading URL", startUrl);
  mainWindow.on("reload", () => {
    console.log("Reloading URL", startUrl);
    mainWindow.loadURL('http://localhost:3000')
  })
  mainWindow.on("closed", () => {
    mainWindow = null;
  });
}


const npmCommand = process.platform === "win32" ? "npm.cmd" : "npm";

app.whenReady().then(() => {
  // Start the Node.js server
  serverProcess = fork(path.join(__dirname, "../server/server.js"));

  // Start the React client app
  reactProcess = spawn(npmCommand, ["start"], {
    shell: true,
    env: process.env,
    cwd: path.join(__dirname, "../client"),
    stdio: "inherit",
  });

  reactProcess.on('error', (err) => {
    console.error('Failed to start React process:', err);
  });

  createWindow();
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") app.quit();
});

app.on("quit", () => {
  // Terminate the server and Omniscope APP when closing the Electron app
  if (serverProcess) serverProcess.kill();
  if (reactProcess) reactProcess.kill('SIGTERM');
});

app.on("activate", () => {
  if (mainWindow === null) createWindow();
});
