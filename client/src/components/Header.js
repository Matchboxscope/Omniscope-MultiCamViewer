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
    sendMessage({ operation: 'focusPlus' });
  }
  
  const handleFocusMinus = () => {
    sendMessage({ operation: 'focusMinus' });
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


  // Add functions for other buttons and inputs similarly

  return (
    <AppBar position="static">
    <Toolbar>
      <img src="https://avatars.githubusercontent.com/u/4345528?v=4" alt="Omniscope" width="50" height="50" />
      <Button color="inherit" onClick={handleSnapAll}>Snap All</Button>
      <Button color="inherit">+ Focus</Button>
      <Button color="inherit">- Focus</Button>
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

