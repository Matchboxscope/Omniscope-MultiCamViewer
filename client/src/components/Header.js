// src/components/Header.js
import React from 'react';
import { AppBar, Toolbar, Typography, Button, TextField, Box } from '@mui/material';
import { useWebSocket } from '../context/WebSocketContext';

const Header = () => {
  const { sendMessage } = useWebSocket();

  const handleSnapAll = () => {
    sendMessage({ operation: 'snapAllCameras' });
  };

  // Add functions for other buttons and inputs similarly

  return (
    <AppBar position="static">
    <Toolbar>
      <img src="https://avatars.githubusercontent.com/u/4345528?v=4" alt="Omniscope" width="50" height="50" />
      <Button color="inherit">Snap All</Button>
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
    </Toolbar>
  </AppBar>
  );
};
/*
</AppBar>
    <AppBar position="static">
      <Toolbar>
        <Typography variant="h6" component="div" sx={{ flexGrow: 1 }}>
          Camera Control Panel
        </Typography>
        <Button color="inherit" onClick={handleSnapAll}>Snap All</Button>

        </Toolbar>
        </AppBar>
        */

export default Header;

