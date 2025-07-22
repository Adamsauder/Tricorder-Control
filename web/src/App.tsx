import React from 'react';
import {
  AppBar,
  Toolbar,
  Typography,
  Container,
  Grid,
  Card,
  CardContent,
  CardActions,
  Button,
  Chip,
  Box,
  Paper,
  List,
  ListItem,
  ListItemText,
  ListItemSecondaryAction,
  IconButton,
} from '@mui/material';
import {
  PlayArrow,
  Pause,
  Stop,
  Palette,
  VideoLibrary,
  Wifi,
  WifiOff,
  Battery3Bar,
  BatteryFull,
  BatteryAlert,
} from '@mui/icons-material';

interface Device {
  device_id: string;
  ip_address: string;
  firmware_version: string;
  status: 'online' | 'offline';
  last_seen: string;
  battery_voltage?: number;
  temperature?: number;
  current_video?: string;
  video_playing: boolean;
}

const mockDevices: Device[] = [
  {
    device_id: 'TRICORDER_001',
    ip_address: '192.168.1.100',
    firmware_version: '1.0.0',
    status: 'online',
    last_seen: '2024-01-15T10:30:00Z',
    battery_voltage: 4.2,
    temperature: 25.0,
    current_video: 'scene1.mp4',
    video_playing: true,
  },
  {
    device_id: 'TRICORDER_002',
    ip_address: '192.168.1.101',
    firmware_version: '1.0.0',
    status: 'online',
    last_seen: '2024-01-15T10:29:45Z',
    battery_voltage: 3.8,
    temperature: 23.5,
    current_video: '',
    video_playing: false,
  },
  {
    device_id: 'TRICORDER_003',
    ip_address: '192.168.1.102',
    firmware_version: '1.0.0',
    status: 'offline',
    last_seen: '2024-01-15T10:25:00Z',
    battery_voltage: 3.2,
    temperature: 22.0,
    current_video: '',
    video_playing: false,
  },
];

const getBatteryIcon = (voltage?: number) => {
  if (!voltage) return <Battery3Bar />;
  if (voltage > 4.0) return <BatteryFull color="success" />;
  if (voltage > 3.7) return <Battery3Bar color="warning" />;
  return <BatteryAlert color="error" />;
};

const DeviceCard: React.FC<{ device: Device }> = ({ device }) => {
  return (
    <Card sx={{ height: '100%', display: 'flex', flexDirection: 'column' }}>
      <CardContent sx={{ flexGrow: 1 }}>
        <Box display="flex" justifyContent="space-between" alignItems="center" mb={1}>
          <Typography variant="h6" component="div">
            {device.device_id}
          </Typography>
          <Box display="flex" alignItems="center" gap={1}>
            {device.status === 'online' ? (
              <Wifi color="success" />
            ) : (
              <WifiOff color="error" />
            )}
            {getBatteryIcon(device.battery_voltage)}
          </Box>
        </Box>
        
        <Chip
          label={device.status}
          color={device.status === 'online' ? 'success' : 'error'}
          size="small"
          sx={{ mb: 1 }}
        />
        
        <Typography variant="body2" color="text.secondary">
          IP: {device.ip_address}
        </Typography>
        <Typography variant="body2" color="text.secondary">
          Firmware: {device.firmware_version}
        </Typography>
        
        {device.battery_voltage && (
          <Typography variant="body2" color="text.secondary">
            Battery: {device.battery_voltage.toFixed(1)}V
          </Typography>
        )}
        
        {device.temperature && (
          <Typography variant="body2" color="text.secondary">
            Temp: {device.temperature.toFixed(1)}°C
          </Typography>
        )}
        
        {device.current_video && (
          <Typography variant="body2" color="text.secondary">
            Video: {device.current_video}
          </Typography>
        )}
      </CardContent>
      
      <CardActions>
        <Button size="small" startIcon={<PlayArrow />} disabled={device.status === 'offline'}>
          Play
        </Button>
        <Button size="small" startIcon={<Pause />} disabled={device.status === 'offline'}>
          Pause
        </Button>
        <Button size="small" startIcon={<Stop />} disabled={device.status === 'offline'}>
          Stop
        </Button>
        <Button size="small" startIcon={<Palette />} disabled={device.status === 'offline'}>
          LEDs
        </Button>
      </CardActions>
    </Card>
  );
};

const App: React.FC = () => {
  return (
    <Box sx={{ flexGrow: 1 }}>
      <AppBar position="static">
        <Toolbar>
          <Typography variant="h6" component="div" sx={{ flexGrow: 1 }}>
            Tricorder Control System
          </Typography>
          <Chip label={`${mockDevices.filter(d => d.status === 'online').length}/${mockDevices.length} Online`} color="primary" />
        </Toolbar>
      </AppBar>
      
      <Container maxWidth="lg" sx={{ mt: 4, mb: 4 }}>
        <Grid container spacing={3}>
          {/* Device Grid */}
          <Grid item xs={12}>
            <Typography variant="h4" gutterBottom>
              Device Dashboard
            </Typography>
          </Grid>
          
          {mockDevices.map((device) => (
            <Grid item xs={12} sm={6} md={4} key={device.device_id}>
              <DeviceCard device={device} />
            </Grid>
          ))}
          
          {/* Control Panel */}
          <Grid item xs={12} md={6}>
            <Paper sx={{ p: 2 }}>
              <Typography variant="h6" gutterBottom>
                Global Controls
              </Typography>
              <Box display="flex" gap={1} flexWrap="wrap">
                <Button variant="contained" startIcon={<PlayArrow />}>
                  Play All
                </Button>
                <Button variant="contained" startIcon={<Pause />}>
                  Pause All
                </Button>
                <Button variant="contained" startIcon={<Stop />}>
                  Stop All
                </Button>
                <Button variant="contained" startIcon={<VideoLibrary />}>
                  File Manager
                </Button>
              </Box>
            </Paper>
          </Grid>
          
          {/* System Status */}
          <Grid item xs={12} md={6}>
            <Paper sx={{ p: 2 }}>
              <Typography variant="h6" gutterBottom>
                System Status
              </Typography>
              <List dense>
                <ListItem>
                  <ListItemText 
                    primary="Server Status" 
                    secondary="Running on localhost:8080" 
                  />
                  <ListItemSecondaryAction>
                    <Chip label="Online" color="success" size="small" />
                  </ListItemSecondaryAction>
                </ListItem>
                <ListItem>
                  <ListItemText 
                    primary="Connected Devices" 
                    secondary={`${mockDevices.filter(d => d.status === 'online').length} of ${mockDevices.length}`}
                  />
                </ListItem>
                <ListItem>
                  <ListItemText 
                    primary="Network" 
                    secondary="WiFi: TRICORDER_CONTROL"
                  />
                </ListItem>
              </List>
            </Paper>
          </Grid>
        </Grid>
      </Container>
    </Box>
  );
};

export default App;
