// src/context/WebSocketContext.js
import React, { createContext, useContext, useEffect, useState } from 'react';

const WebSocketContext = createContext(null);

export const useWebSocket = () => {
  return useContext(WebSocketContext);
};

export const WebSocketProvider = ({ children }) => {
  const [socket, setSocket] = useState(null);
  const [devices, setDevices] = useState([]);

  
  useEffect(() => {
    const location = window.location;
    let wsUrl;

    if (location.protocol === 'https:') {
        wsUrl = 'wss:';
    } else {
        wsUrl = 'ws:';
    }

    wsUrl += `//${location.hostname}:8999`;

    console.log(wsUrl);
    const ws = new WebSocket(wsUrl);

    setSocket(ws);

    ws.onopen = () => {
      ws.send(JSON.stringify({
        'client': '8999',
        'operation': 'connecting',
        'data': {}
      }));
    };

    ws.onmessage = (message) => {
      let md = JSON.parse(message.data);
      setDevices(md.devices);
    };

    return () => ws.close();
  }, []);

  const sendMessage = (message) => {
    if (socket?.readyState === WebSocket.OPEN) {
      socket.send(JSON.stringify(message));
    }
  };

  return (
    <WebSocketContext.Provider value={{ socket, sendMessage, devices }}>
      {children}
    </WebSocketContext.Provider>
  );
};

export default WebSocketProvider;
