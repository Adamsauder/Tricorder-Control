import React, { useState } from 'react';

const SimpleApp: React.FC = () => {
  const [selectedDevices, setSelectedDevices] = useState<Set<string>>(new Set());

  const mockTricorders = [
    { device_id: 'TRICORDER_001', status: 'online', battery: 85, video_playing: true, location: 'Set A - Bridge' },
    { device_id: 'TRICORDER_002', status: 'offline', battery: 42, video_playing: false, location: 'Set B - Engineering' },
    { device_id: 'TRICORDER_003', status: 'online', battery: 95, video_playing: true, location: 'Set C - Sick Bay' },
    { device_id: 'TRICORDER_004', status: 'error', battery: 12, video_playing: false, location: 'Set D - Ready Room' },
    { device_id: 'TRICORDER_005', status: 'online', battery: 78, video_playing: true, location: 'Set A - Transporter' },
    { device_id: 'TRICORDER_006', status: 'online', battery: 62, video_playing: false, location: 'Set E - Cargo Bay' },
  ];

  const toggleDeviceSelection = (deviceId: string) => {
    const newSelected = new Set(selectedDevices);
    if (newSelected.has(deviceId)) {
      newSelected.delete(deviceId);
    } else {
      newSelected.add(deviceId);
    }
    setSelectedDevices(newSelected);
  };

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'online': return '#4caf50';
      case 'offline': return '#ff9800';
      case 'error': return '#f44336';
      default: return '#757575';
    }
  };

  const getBatteryColor = (battery: number) => {
    if (battery > 60) return '#4caf50';
    if (battery > 30) return '#ff9800';
    return '#f44336';
  };

  return (
    <div style={{ padding: '20px', fontFamily: 'Arial, sans-serif', backgroundColor: '#f5f5f5', minHeight: '100vh' }}>
      <h1 style={{ textAlign: 'center', color: '#333', marginBottom: '30px' }}>
        🚀 Prop Control Farm
      </h1>
      
      <div style={{
        display: 'grid',
        gridTemplateColumns: 'repeat(auto-fill, minmax(320px, 1fr))',
        gap: '20px',
        maxWidth: '1200px',
        margin: '0 auto'
      }}>
        {mockTricorders.map((device) => (
          <div
            key={device.device_id}
            onClick={() => toggleDeviceSelection(device.device_id)}
            style={{
              backgroundColor: selectedDevices.has(device.device_id) ? '#e3f2fd' : '#fff',
              border: selectedDevices.has(device.device_id) ? '2px solid #2196f3' : '1px solid #ddd',
              borderRadius: '8px',
              padding: '16px',
              cursor: 'pointer',
              boxShadow: '0 2px 4px rgba(0,0,0,0.1)',
              transition: 'all 0.3s ease',
              position: 'relative'
            }}
          >
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '12px' }}>
              <h3 style={{ margin: 0, color: '#333' }}>{device.device_id}</h3>
              <span style={{
                backgroundColor: getStatusColor(device.status),
                color: 'white',
                padding: '4px 8px',
                borderRadius: '4px',
                fontSize: '12px',
                fontWeight: 'bold'
              }}>
                {device.status.toUpperCase()}
              </span>
            </div>
            
            <p style={{ margin: '8px 0', color: '#666', fontSize: '14px' }}>
              📍 {device.location}
            </p>
            
            <div style={{ marginBottom: '12px' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '4px' }}>
                <span style={{ fontSize: '14px', color: '#666' }}>Battery</span>
                <span style={{ fontSize: '14px', fontWeight: 'bold', color: getBatteryColor(device.battery) }}>
                  {device.battery}%
                </span>
              </div>
              <div style={{
                backgroundColor: '#f0f0f0',
                height: '4px',
                borderRadius: '2px',
                overflow: 'hidden'
              }}>
                <div style={{
                  backgroundColor: getBatteryColor(device.battery),
                  height: '100%',
                  width: `${device.battery}%`,
                  transition: 'width 0.3s ease'
                }} />
              </div>
            </div>
            
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '12px' }}>
              <span style={{ fontSize: '14px', color: '#666' }}>Video</span>
              <span style={{
                backgroundColor: device.video_playing ? '#4caf50' : '#ccc',
                color: 'white',
                padding: '2px 6px',
                borderRadius: '4px',
                fontSize: '12px'
              }}>
                {device.video_playing ? 'Playing' : 'Stopped'}
              </span>
            </div>
            
            <div style={{ display: 'flex', gap: '8px' }}>
              <button style={{
                backgroundColor: '#4caf50',
                color: 'white',
                border: 'none',
                borderRadius: '4px',
                padding: '6px 12px',
                fontSize: '12px',
                cursor: 'pointer',
                flex: 1
              }}>
                Play
              </button>
              <button style={{
                backgroundColor: '#ff9800',
                color: 'white',
                border: 'none',
                borderRadius: '4px',
                padding: '6px 12px',
                fontSize: '12px',
                cursor: 'pointer',
                flex: 1
              }}>
                Stop
              </button>
              <button style={{
                backgroundColor: '#2196f3',
                color: 'white',
                border: 'none',
                borderRadius: '4px',
                padding: '6px 12px',
                fontSize: '12px',
                cursor: 'pointer',
                flex: 1
              }}>
                Settings
              </button>
            </div>
            
            {selectedDevices.has(device.device_id) && (
              <div style={{
                position: 'absolute',
                top: '8px',
                right: '8px',
                backgroundColor: '#2196f3',
                color: 'white',
                borderRadius: '50%',
                width: '20px',
                height: '20px',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                fontSize: '12px'
              }}>
                ✓
              </div>
            )}
          </div>
        ))}
      </div>
      
      {selectedDevices.size > 0 && (
        <div style={{
          position: 'fixed',
          bottom: '20px',
          right: '20px',
          backgroundColor: '#2196f3',
          color: 'white',
          padding: '10px 16px',
          borderRadius: '8px',
          boxShadow: '0 4px 8px rgba(0,0,0,0.2)'
        }}>
          {selectedDevices.size} devices selected
        </div>
      )}

      {/* Add Device Button */}
      <div style={{ 
        position: 'fixed', 
        bottom: '20px', 
        left: '20px',
        width: '56px',
        height: '56px',
        borderRadius: '50%',
        backgroundColor: '#2196F3',
        color: 'white',
        border: 'none',
        fontSize: '24px',
        cursor: 'pointer',
        boxShadow: '0 4px 8px rgba(0,0,0,0.2)',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center'
      }}>
        +
      </div>
    </div>
  );
};

export default SimpleApp;
