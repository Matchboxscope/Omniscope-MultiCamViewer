import React, { useState } from 'react';
import { CircularProgress } from '@mui/material';
import { useWebSocket } from '../context/WebSocketContext';

const Camera = ({ id }) => {
  const { devices } = useWebSocket();
  const [zoomLevel, setZoomLevel] = useState(1);
  const [position, setPosition] = useState({ x: 0, y: 0 });
  const [isDragging, setIsDragging] = useState(false);
  const [startPos, setStartPos] = useState({ x: 0, y: 0 });

  const device = devices?.find(device => device["display"] === id);
  const isConnected = device && device.image;

  const handleZoom = (event) => {
    setZoomLevel(prevZoom => Math.max(1, prevZoom + event.deltaY * -0.01));
  };

  const handleMouseDown = (event) => {
    setIsDragging(true);
    setStartPos({ x: event.clientX - position.x, y: event.clientY - position.y });
  };

  const handleMouseMove = (event) => {
    if (isDragging) {
      setPosition({
        x: event.clientX - startPos.x,
        y: event.clientY - startPos.y,
      });
    }
  };

  const handleMouseUp = () => {
    setIsDragging(false);
  };

  const cameraStyle = {
    transform: `scale(${zoomLevel}) translate(${position.x}px, ${position.y}px)`,
    transition: 'transform 0.1s',
    cursor: isDragging ? 'grabbing' : 'grab',
    height: '100%',
    width: '100%',
    objectFit: 'cover',
  };

  return (
    <div 
      style={{ 
        border: '1px solid #fff', 
        height: 150, 
        margin: 5, 
        position: 'relative',
        overflow: 'hidden'  // To handle zoomed content
      }}
      onWheel={handleZoom}
      onMouseDown={handleMouseDown}
      onMouseMove={handleMouseMove}
      onMouseUp={handleMouseUp}
      onMouseLeave={handleMouseUp}  // To handle cursor leaving the element while dragging
    >
      {isConnected ? (
        <img 
          src={`data:image/jpeg;base64,${device.image}`} 
          alt={device.display}
          style={cameraStyle}
        />
      ) : (
        <div style={{ display: 'flex', justifyContent: 'center', alignItems: 'center', height: '100%' }}>
          <CircularProgress />
        </div>
      )}
    </div>
  );
};

export default Camera;
