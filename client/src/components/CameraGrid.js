import React, { useContext, useEffect, useState } from 'react';
import { Grid } from '@mui/material';
import Camera from './Camera';
import CameraPlaceholder from './CameraPlaceholder';
import { useWebSocket } from '../context/WebSocketContext';

function CameraGrid() {
  const { devices } = useWebSocket();
  
  // Additional state to track the last update time for each camera
  const [lastFrameTime, setLastFrameTime] = useState({});

  const [cameraSlots, setCameraSlots] = useState(() =>
    Array.from({ length: 24 }, (_, index) => ({
      id: `Cam#${index + 1}`,
      connected: false,
      stream: null,
    }))
  );

  // State to track framerates
  const [framerates, setFramerates] = useState({});

  // Function to update the framerate
  const handleFramerateUpdate = (cameraId, framerate) => {
    setFramerates(prevFramerates => ({
      ...prevFramerates,
      [cameraId]: framerate,
    }));
  };

  useEffect(() => {
    const updatedCameraSlots = cameraSlots.map((slot) => {
      const deviceInfo = devices.find((device) => `Cam#${device.key}` === slot.id);
      return deviceInfo
        ? { ...slot, connected: true, stream: deviceInfo.stream }
        : slot;
    });

    setCameraSlots(updatedCameraSlots);
  }, [devices]);

  useEffect(() => {
    cameraSlots.forEach((camera) => {
      if (camera.connected) {
        // Clear any existing timeout
        clearTimeout(lastFrameTime[camera.id]);

        // Set a new timeout
        lastFrameTime[camera.id] = setTimeout(() => {
          setCameraSlots((prevSlots) =>
            prevSlots.map((slot) =>
              slot.id === camera.id ? { ...slot, connected: false } : slot
            )
          );
        }, 5000); // 5 seconds timeout
      }
    });

    // Cleanup function to clear timeouts when the component unmounts
    return () => {
      Object.values(lastFrameTime).forEach(clearTimeout);
    };
  }, [cameraSlots]);

  // Function to update the last frame time, called from Camera component
  const updateLastFrameTime = (cameraId) => {
    setLastFrameTime((prevTimes) => ({
      ...prevTimes,
      [cameraId]: new Date().getTime(),
    }));
  };

  return (
    <Grid container justifyContent="center" style={{ marginTop: 20 }}>
      {cameraSlots.map((camera, index) => (
        <Grid item xs={2} key={camera.id} style={{ position: 'relative' }}>
          {camera.connected ? (
            <Camera 
              id={camera.id} 
              stream={camera.stream} 
              onFrameUpdate={() => updateLastFrameTime(camera.id)}
              onFramerateUpdate={(framerate) => handleFramerateUpdate(camera.id, framerate)}
            />
          ) : (
            <CameraPlaceholder id={camera.id} />
          )}
          <div style={{ position: 'absolute', bottom: 5, right: 5, color: 'white' }}>
            {framerates[camera.id] ? `${framerates[camera.id]} FPS` : ''}
          </div>
        </Grid>
      ))}
    </Grid>
  );
}

export default CameraGrid;
