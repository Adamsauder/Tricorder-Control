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
  Tab,
  Tabs,
  ThemeProvider,
  createTheme,
  CssBaseline,
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
  Devices,
  Computer,
  Dashboard,
} from '@mui/icons-material';
import ESP32Simulator from './components/ESP32Simulator';
import TricorderFarmDashboard from './components/TricorderFarmDashboard';

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
  const [tabValue, setTabValue] = React.useState(0);

  const handleTabChange = (event: React.SyntheticEvent, newValue: number) => {
    setTabValue(newValue);
  };

  const handleSimulatorCommand = (command: any) => {
    console.log('Simulator command:', command);
    // Here you can integrate with your real device communication
  };

  // Create a theme inspired by SimplyPrint
  const theme = createTheme({
    palette: {
      primary: {
        main: '#2196f3',
      },
      secondary: {
        main: '#ff4081',
      },
      background: {
        default: '#f8f9fa',
      },
    },
    typography: {
      fontFamily: '-apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif',
    },
    components: {
      MuiCard: {
        styleOverrides: {
          root: {
            borderRadius: 12,
            boxShadow: '0 2px 8px rgba(0,0,0,0.1)',
          },
        },
      },
      MuiButton: {
        styleOverrides: {
          root: {
            borderRadius: 8,
            textTransform: 'none',
          },
        },
      },
    },
  });

  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      {tabValue === 0 ? (
        <TricorderFarmDashboard />
      ) : (
        <Box sx={{ flexGrow: 1 }}>
          <AppBar position="static">
            <Toolbar>
              <Typography variant="h6" component="div" sx={{ flexGrow: 1 }}>
                Prop Control System - Legacy View
              </Typography>
              <Chip label={`${mockDevices.filter(d => d.status === 'online').length}/${mockDevices.length} Online`} color="primary" />
            </Toolbar>
          </AppBar>
          
          <Container maxWidth="lg" sx={{ mt: 4, mb: 4 }}>
            <Tabs value={tabValue} onChange={handleTabChange} sx={{ mb: 3 }}>
              <Tab icon={<Dashboard />} label="Farm Dashboard" />
              <Tab icon={<Computer />} label="ESP32 Simulator" />
            </Tabs>

            {tabValue === 1 && (
              <Grid container spacing={3} justifyContent="center">
                <Grid item xs={12} md={8} lg={6}>
                  <ESP32Simulator 
                    deviceId="SIMULATOR_001"
                    width={320}
                    height={240}
                    onCommand={handleSimulatorCommand}
                  />
                </Grid>
                <Grid item xs={12} md={4} lg={6}>
                  <Paper sx={{ p: 2 }}>
                    <Typography variant="h6" gutterBottom>
                      Simulator Info
                    </Typography>
                    <Typography variant="body2" paragraph>
                      This simulator replicates the ESP32-2432S032C-I display behavior, 
                      showing exactly what would appear on the physical tricorder screen.
                    </Typography>
                    <Typography variant="body2" paragraph>
                      <strong>Hardware Specs:</strong><br />
                      • Display: ST7789 TFT<br />
                      • Resolution: 320x240 pixels<br />
                      • Color: 16-bit RGB565<br />
                      • Frame Rate: Up to 30 FPS
                    </Typography>
                    <Typography variant="body2" paragraph>
                      <strong>Features:</strong><br />
                      • Real-time video playback simulation<br />
                      • Brightness control<br />
                      • Animation sequences<br />
                      • Color test patterns<br />
                      • Command logging
                    </Typography>
                  </Paper>
                </Grid>
              </Grid>
            )}
          </Container>
        </Box>
      )}
    </ThemeProvider>
  );
};

export default App;
