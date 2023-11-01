// CameraGrid.js
import React from 'react';
import { Grid } from '@mui/material';
import Camera from './Camera'; // Ensure this path is correct

function CameraGrid() {
  const cameraIDs = [...Array(24).keys()].map(k => `esp32cam${k + 1}`); // Example IDs

  return (
    <Grid container justifyContent="center" style={{ marginTop: 20 }}>
      {cameraIDs.map(id => (
        <Grid item xs={2} key={id}>
          <Camera id={id} />
        </Grid>
      ))}
    </Grid>
  );
}

export default CameraGrid;
