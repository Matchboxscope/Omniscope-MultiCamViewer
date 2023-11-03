import React from 'react';
import { CircularProgress } from '@mui/material';

const CameraPlaceholder = () => {
  return (
    <div style={{ border: '1px solid #fff', height: 150, margin: 5,  display: 'flex', justifyContent: 'center', alignItems: 'center', position: 'relative', }}>
      <CircularProgress />
    </div>
  );
};

export default CameraPlaceholder;
