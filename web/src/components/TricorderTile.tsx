import React, { useState, useEffect } from 'react';
import {
  Box,
  Card,
  CardContent,
  CardActions,
  Typography,
  Button,
  Chip,
  IconButton,
  Grid,
  Paper,
  Avatar,
  LinearProgress,
  Menu,
  MenuItem,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Stack,
  Tooltip,
} from '@mui/material';
import {
  PlayArrow,
  Pause,
  Stop,
  Settings,
  Visibility,
  MoreVert,
  Wifi,
  WifiOff,
  Battery3Bar,
  BatteryFull,
  BatteryAlert,
  Videocam,
  VideocamOff,
  Palette,
  CheckCircle,
  Error,
  Warning,
  Circle,
} from '@mui/icons-material';

interface TricorderDevice {
  device_id: string;
  ip_address: string;
  firmware_version: string;
  status: 'online' | 'offline' | 'busy' | 'error';
  last_seen: string;
  battery_voltage?: number;
  temperature?: number;
  current_video?: string;
  video_playing: boolean;
  progress?: number; // For ongoing operations (0-100)
  location?: string;
  notes?: string;
}

interface TricorderTileProps {
  device: TricorderDevice;
  isSelected: boolean;
  onSelect: (device: TricorderDevice) => void;
  onAction: (device: TricorderDevice, action: string, data?: any) => void;
}

const TricorderTile: React.FC<TricorderTileProps> = ({
  device,
  isSelected,
  onSelect,
  onAction,
}) => {
  const [anchorEl, setAnchorEl] = useState<null | HTMLElement>(null);
  const [previewOpen, setPreviewOpen] = useState(false);

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'online': return '#4caf50';
      case 'busy': return '#ff9800';
      case 'error': return '#f44336';
      case 'offline': return '#9e9e9e';
      default: return '#9e9e9e';
    }
  };

  const getStatusIcon = (status: string) => {
    switch (status) {
      case 'online': return <CheckCircle sx={{ color: '#4caf50' }} />;
      case 'busy': return <Warning sx={{ color: '#ff9800' }} />;
      case 'error': return <Error sx={{ color: '#f44336' }} />;
      case 'offline': return <Circle sx={{ color: '#9e9e9e' }} />;
      default: return <Circle sx={{ color: '#9e9e9e' }} />;
    }
  };

  const getBatteryIcon = (voltage?: number) => {
    if (!voltage) return <Battery3Bar />;
    if (voltage > 4.0) return <BatteryFull sx={{ color: '#4caf50' }} />;
    if (voltage > 3.7) return <Battery3Bar sx={{ color: '#ff9800' }} />;
    return <BatteryAlert sx={{ color: '#f44336' }} />;
  };

  const formatLastSeen = (lastSeen: string) => {
    const now = new Date();
    const lastSeenDate = new Date(lastSeen);
    const diffMs = now.getTime() - lastSeenDate.getTime();
    const diffMins = Math.floor(diffMs / (1000 * 60));
    
    if (diffMins < 1) return 'Just now';
    if (diffMins < 60) return `${diffMins}m ago`;
    if (diffMins < 1440) return `${Math.floor(diffMins / 60)}h ago`;
    return `${Math.floor(diffMins / 1440)}d ago`;
  };

  return (
    <>
      <Card
        sx={{
          height: '100%',
          cursor: 'pointer',
          border: isSelected ? '2px solid #2196f3' : '2px solid transparent',
          transition: 'all 0.3s ease',
          transform: isSelected ? 'scale(1.02)' : 'scale(1)',
          boxShadow: isSelected ? 4 : 1,
          '&:hover': {
            boxShadow: 3,
            transform: 'scale(1.01)',
          },
          background: device.status === 'offline' ? '#fafafa' : 'white',
        }}
        onClick={() => onSelect(device)}
      >
        {/* Header with Status and Menu */}
        <Box
          sx={{
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'space-between',
            p: 2,
            pb: 1,
            borderBottom: `3px solid ${getStatusColor(device.status)}`,
          }}
        >
          <Box display="flex" alignItems="center" gap={1}>
            <Avatar
              sx={{
                bgcolor: getStatusColor(device.status),
                width: 32,
                height: 32,
                fontSize: '0.875rem',
              }}
            >
              {device.device_id.split('_')[1] || device.device_id.slice(-2)}
            </Avatar>
            {getStatusIcon(device.status)}
          </Box>
          
          <IconButton
            size="small"
            onClick={(e) => {
              e.stopPropagation();
              setAnchorEl(e.currentTarget);
            }}
          >
            <MoreVert />
          </IconButton>
        </Box>

        <CardContent sx={{ pt: 1, pb: 1 }}>
          {/* Device Name and Location */}
          <Typography variant="h6" noWrap>
            {device.device_id}
          </Typography>
          {device.location && (
            <Typography variant="body2" color="text.secondary" noWrap>
              📍 {device.location}
            </Typography>
          )}

          {/* Current Activity */}
          <Box mt={1} mb={2}>
            {device.video_playing && device.current_video ? (
              <Box>
                <Box display="flex" alignItems="center" gap={1} mb={1}>
                  <Videocam color="primary" fontSize="small" />
                  <Typography variant="body2" noWrap>
                    {device.current_video}
                  </Typography>
                </Box>
                {device.progress !== undefined && (
                  <LinearProgress 
                    variant="determinate" 
                    value={device.progress} 
                    sx={{ mb: 1 }}
                  />
                )}
              </Box>
            ) : (
              <Box display="flex" alignItems="center" gap={1}>
                <VideocamOff color="disabled" fontSize="small" />
                <Typography variant="body2" color="text.secondary">
                  Idle
                </Typography>
              </Box>
            )}
          </Box>

          {/* Status Info Grid */}
          <Grid container spacing={1} alignItems="center">
            <Grid item xs={6}>
              <Box display="flex" alignItems="center" gap={0.5}>
                {device.status === 'online' ? <Wifi fontSize="small" color="success" /> : <WifiOff fontSize="small" color="disabled" />}
                <Typography variant="caption">
                  {device.ip_address.split('.').slice(-1)[0]}
                </Typography>
              </Box>
            </Grid>
            
            <Grid item xs={6}>
              <Box display="flex" alignItems="center" gap={0.5}>
                {getBatteryIcon(device.battery_voltage)}
                <Typography variant="caption">
                  {device.battery_voltage ? `${device.battery_voltage.toFixed(1)}V` : 'N/A'}
                </Typography>
              </Box>
            </Grid>
            
            <Grid item xs={6}>
              <Typography variant="caption" color="text.secondary">
                🌡️ {device.temperature ? `${device.temperature.toFixed(1)}°C` : 'N/A'}
              </Typography>
            </Grid>
            
            <Grid item xs={6}>
              <Typography variant="caption" color="text.secondary">
                {formatLastSeen(device.last_seen)}
              </Typography>
            </Grid>
          </Grid>
        </CardContent>

        {/* Action Buttons */}
        <CardActions sx={{ justifyContent: 'space-between', px: 2, py: 1 }}>
          <Box>
            <Tooltip title="Play Video">
              <IconButton
                size="small"
                color="primary"
                disabled={device.status !== 'online'}
                onClick={(e) => {
                  e.stopPropagation();
                  onAction(device, 'play');
                }}
              >
                <PlayArrow />
              </IconButton>
            </Tooltip>
            
            <Tooltip title="Pause">
              <IconButton
                size="small"
                color="warning"
                disabled={device.status !== 'online' || !device.video_playing}
                onClick={(e) => {
                  e.stopPropagation();
                  onAction(device, 'pause');
                }}
              >
                <Pause />
              </IconButton>
            </Tooltip>
            
            <Tooltip title="Stop">
              <IconButton
                size="small"
                color="error"
                disabled={device.status !== 'online'}
                onClick={(e) => {
                  e.stopPropagation();
                  onAction(device, 'stop');
                }}
              >
                <Stop />
              </IconButton>
            </Tooltip>
          </Box>

          <Box>
            <Tooltip title="LED Control">
              <IconButton
                size="small"
                disabled={device.status !== 'online'}
                onClick={(e) => {
                  e.stopPropagation();
                  onAction(device, 'leds');
                }}
              >
                <Palette />
              </IconButton>
            </Tooltip>
            
            <Tooltip title="Preview Screen">
              <IconButton
                size="small"
                disabled={device.status !== 'online'}
                onClick={(e) => {
                  e.stopPropagation();
                  setPreviewOpen(true);
                }}
              >
                <Visibility />
              </IconButton>
            </Tooltip>
          </Box>
        </CardActions>
      </Card>

      {/* Context Menu */}
      <Menu
        anchorEl={anchorEl}
        open={Boolean(anchorEl)}
        onClose={() => setAnchorEl(null)}
        onClick={(e) => e.stopPropagation()}
      >
        <MenuItem onClick={() => { onAction(device, 'restart'); setAnchorEl(null); }}>
          🔄 Restart Device
        </MenuItem>
        <MenuItem onClick={() => { onAction(device, 'update'); setAnchorEl(null); }}>
          ⬆️ Update Firmware
        </MenuItem>
        <MenuItem onClick={() => { onAction(device, 'settings'); setAnchorEl(null); }}>
          ⚙️ Settings
        </MenuItem>
        <MenuItem onClick={() => { onAction(device, 'remove'); setAnchorEl(null); }}>
          🗑️ Remove
        </MenuItem>
      </Menu>

      {/* Screen Preview Dialog */}
      <Dialog
        open={previewOpen}
        onClose={() => setPreviewOpen(false)}
        maxWidth="sm"
        fullWidth
      >
        <DialogTitle>
          Screen Preview - {device.device_id}
        </DialogTitle>
        <DialogContent>
          <Box
            sx={{
              display: 'flex',
              justifyContent: 'center',
              p: 2,
              bgcolor: '#000',
              borderRadius: 1,
            }}
          >
            <Box
              sx={{
                width: 320,
                height: 240,
                border: '2px solid #333',
                bgcolor: '#111',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                color: '#0f0',
                fontFamily: 'monospace',
              }}
            >
              {device.current_video || 'No active display'}
            </Box>
          </Box>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setPreviewOpen(false)}>Close</Button>
        </DialogActions>
      </Dialog>
    </>
  );
};

export default TricorderTile;
