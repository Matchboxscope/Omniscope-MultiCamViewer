import React, { useContext, useEffect, useState } from 'react';
import { Grid } from '@mui/material';
import Camera from './Camera'; // Ensure this path is correct
import CameraPlaceholder from './CameraPlaceholder'; // Component for the placeholder/spinner
import { useWebSocket } from '../context/WebSocketContext'; // Adjust path as necessary

function CameraGrid() {
  // Use the context to get the devices
  const { devices } = useWebSocket();
  
  // Initialize a state with 24 placeholders
  const [cameraSlots, setCameraSlots] = useState(() =>
    Array.from({ length: 24 }, (_, index) => ({
      id: `Cam#${index + 1}`,
      connected: false,
      stream: null, // Will be updated with the stream
    }))
  );

  // will be updated when devices gets updated
  useEffect(() => {
    // Map over the existing camera slots and update them if a camera is connected
    const updatedCameraSlots = cameraSlots.map((slot) => {
      const deviceInfo = devices.find((device) => `Cam#${device.key}` === slot.id);
      return deviceInfo
        ? { ...slot, connected: true, stream: deviceInfo.stream } // Update with camera info
        : slot; // Or leave the slot as is
    });

    setCameraSlots(updatedCameraSlots);
  }, [devices]);

  return (
    <Grid container justifyContent="center" style={{ marginTop: 20 }}>
      {cameraSlots.map((camera, index) => (
        <Grid item xs={2} key={camera.id}>
          {camera.connected ? (
            <Camera id={camera.id} stream={camera.stream} />
          ) : (
            <CameraPlaceholder id={camera.id} /> // Placeholder component with a spinner
          )}
        </Grid>
      ))}
    </Grid>
  );
}

export default CameraGrid;
