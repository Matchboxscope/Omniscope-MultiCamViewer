// src/App.js
import React from 'react';
import { CssBaseline, createTheme, ThemeProvider } from '@mui/material';
import CameraGrid from './components/CameraGrid';
import Header from './components/Header';
import WebSocketProvider from './context/WebSocketContext';

const darkTheme = createTheme({
  palette: {
    mode: 'dark',
  },
});

function App() {
  return (
    <ThemeProvider theme={darkTheme}>
      <CssBaseline />
      <WebSocketProvider>
        <Header />
        <CameraGrid />
      </WebSocketProvider>
    </ThemeProvider>
  );
}

export default App;
