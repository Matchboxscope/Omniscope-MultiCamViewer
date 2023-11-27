// src/components/Header.js
import React from 'react';
import { AppBar, Toolbar, Typography, Button, TextField, Box } from '@mui/material';
import { useWebSocket } from '../context/WebSocketContext';

const Header = () => {
  const { sendMessage } = useWebSocket();

  const handleSnapAll = () => {
    sendMessage({ operation: 'snapAllCameras' });
  };

  const handleFocusPlus = () => {
    sendMessage({ operation: 'stageMove', value: 100 });
  }
  
  const handleFocusMinus = () => {
    sendMessage({ operation: 'stageMove', value: -100  });
  }

  const handleTimelapseStart = () => {
    sendMessage({ operation: 'timelapseStart' });
  }

  const handleTimelapseStop = () => {
    sendMessage({ operation: 'timelapseStop' });
  }

  const handleReloadCameras = () => {
    sendMessage({ operation: 'reloadCameras' });
  }

  const handleResetAllCameras = () => {
    sendMessage({ operation: 'resetAllCameras' });
  }

  const handleIlluOn = () => {
    sendMessage({ operation: 'turnIlluminationON' });
  }

  const handleIlluOff = () => {
    sendMessage({ operation: 'turnIlluminationOFF' });
  }


  // Add functions for other buttons and inputs similarly

  return (
    <AppBar position="static">
    <Toolbar>
      <img src="https://avatars.githubusercontent.com/u/4345528?v=4" alt="Omniscope" width="50" height="50" />
      <Button color="inherit" onClick={handleSnapAll}>Snap All</Button>
      <Button color="inherit" onClick={handleFocusMinus}>+ Focus</Button>
      <Button color="inherit" onClick={handleFocusPlus}>- Focus</Button>
      <Button color="inherit" onClick={handleIlluOn}>Illu On</Button>
      <Button color="inherit" onClick={handleIlluOff}>Illu Off</Button>
      <TextField
        label="Timelapse Period (s)"
        variant="outlined"
        size="small"
        margin="normal"
        style={{ marginLeft: 20, marginRight: 20 }}
      />
      <Button color="inherit">Start Timelapse</Button>
      <Button color="inherit">Stop Timelapse</Button>
      <Button color="inherit" onClick={handleReloadCameras}>Reload Cameras</Button>
    </Toolbar>
  </AppBar>
  );
};
export default Header;

