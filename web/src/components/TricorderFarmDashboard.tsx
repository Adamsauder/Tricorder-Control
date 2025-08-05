import React, { useState } from 'react';
import {
  AppBar,
  Toolbar,
  Box,
  Container,
  Typography,
  Grid,
  Card,
  CardContent,
  CardActions,
  Button,
  Chip,
  LinearProgress,
  Fab,
  Paper,
  TextField,
  InputAdornment,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
  Badge,
  IconButton,
  Menu,
  ListItemIcon,
  ListItemText,
  Checkbox,
  Alert,
  Snackbar,
} from '@mui/material';
import {
  Add as AddIcon,
  PlayArrow as PlayIcon,
  Stop as StopIcon,
  Settings as SettingsIcon,
  LocationOn as LocationIcon,
  Battery4Bar as BatteryIcon,
  Videocam as VideoIcon,
  VideocamOff as VideocamOffIcon,
  Search as SearchIcon,
  FilterList as FilterIcon,
  Refresh as RefreshIcon,
  MoreVert as MoreVertIcon,
  Dashboard as DashboardIcon,
  Wifi as WifiIcon,
  WifiOff as WifiOffIcon,
  Error as ErrorIcon,
  Update as UpdateIcon,
  Palette as PaletteIcon,
  Brightness6 as BrightnessIcon,
  SelectAll as SelectAllIcon,
  DeselectAll as DeselectIcon,
} from '@mui/icons-material';
import TricorderSettingsModal from './TricorderSettingsModal';
import FirmwareUpdateModal from './FirmwareUpdateModal';
import { useTricorderFarm } from '../hooks/useTricorderFarm';
import { TricorderDevice } from '../services/tricorderAPI';

const TricorderFarmDashboard: React.FC = () => {
  // Use our custom hook for farm management
  const {
    devices,
    serverInfo,
    loading,
    error,
    connected,
    selectedDevices,
    refreshDevices,
    startDiscovery,
    selectDevice,
    selectAllDevices,
    clearSelection,
    toggleDeviceSelection,
    setLEDColor,
    setLEDBrightness,
    setBuiltinLED,
    playVideo,
    stopVideo,
    displayImage,
    displayBootScreen,
    pingDevices,
    getDeviceStatus,
  } = useTricorderFarm();

  // Local state for UI
  const [searchQuery, setSearchQuery] = useState('');
  const [statusFilter, setStatusFilter] = useState<string>('all');
  const [locationFilter, setLocationFilter] = useState<string>('all');
  const [anchorEl, setAnchorEl] = useState<null | HTMLElement>(null);
  const [settingsOpen, setSettingsOpen] = useState(false);
  const [selectedDevice, setSelectedDevice] = useState<TricorderDevice | null>(null);
  const [firmwareUpdateOpen, setFirmwareUpdateOpen] = useState(false);
  const [updateDevice, setUpdateDevice] = useState<TricorderDevice | null>(null);
  const [snackbarOpen, setSnackbarOpen] = useState(false);
  const [snackbarMessage, setSnackbarMessage] = useState('');

  // Color picker state
  const [colorPickerOpen, setColorPickerOpen] = useState(false);
  const [selectedColor, setSelectedColor] = useState({ r: 255, g: 255, b: 255 });
  const [brightness, setBrightness] = useState(128);

  // Helper functions
  const showSnackbar = (message: string) => {
    setSnackbarMessage(message);
    setSnackbarOpen(true);
  };

  const handleDeviceClick = (deviceId: string, event: React.MouseEvent) => {
    if (event.ctrlKey || event.metaKey) {
      // Multi-select with Ctrl/Cmd
      toggleDeviceSelection(deviceId);
    } else {
      // Single select
      selectDevice(deviceId);
    }
  };

  const handleBulkAction = async (action: string) => {
    const selectedIds = Array.from(selectedDevices);
    if (selectedIds.length === 0) {
      showSnackbar('No devices selected');
      return;
    }

    try {
      switch (action) {
        case 'play_demo':
          await playVideo(selectedIds, 'startup', false);
          showSnackbar(`Playing demo video on ${selectedIds.length} device(s)`);
          break;
        case 'stop_video':
          await stopVideo(selectedIds);
          showSnackbar(`Stopped video on ${selectedIds.length} device(s)`);
          break;
        case 'set_red_led':
          await setLEDColor(selectedIds, 255, 0, 0);
          showSnackbar(`Set LED to red on ${selectedIds.length} device(s)`);
          break;
        case 'set_green_led':
          await setLEDColor(selectedIds, 0, 255, 0);
          showSnackbar(`Set LED to green on ${selectedIds.length} device(s)`);
          break;
        case 'set_blue_led':
          await setLEDColor(selectedIds, 0, 0, 255);
          showSnackbar(`Set LED to blue on ${selectedIds.length} device(s)`);
          break;
        case 'ping':
          await pingDevices(selectedIds);
          showSnackbar(`Pinged ${selectedIds.length} device(s)`);
          break;
        case 'boot_screen':
          await displayBootScreen(selectedIds);
          showSnackbar(`Showing boot screen on ${selectedIds.length} device(s)`);
          break;
        default:
          showSnackbar(`Unknown action: ${action}`);
      }
    } catch (error) {
      showSnackbar(`Error executing action: ${error}`);
    }
  };

  // Filter devices based on search and filters
  const filteredDevices = devices.filter(device => {
    const matchesSearch = device.device_id.toLowerCase().includes(searchQuery.toLowerCase()) ||
                         (device.location || '').toLowerCase().includes(searchQuery.toLowerCase());
    
    const matchesStatus = statusFilter === 'all' || device.status === statusFilter;
    
    const matchesLocation = locationFilter === 'all' || device.location === locationFilter;
    
    return matchesSearch && matchesStatus && matchesLocation;
  });

  // Get unique locations for filter
  const locations = Array.from(new Set(devices
    .map(d => d.location)
    .filter(Boolean)
  ));

  const getBatteryIcon = (battery?: number) => {
    if (!battery) return <BatteryIcon />;
    if (battery > 75) return <BatteryIcon color="success" />;
    if (battery > 25) return <BatteryIcon color="warning" />;
    return <BatteryIcon color="error" />;
  };

  const getStatusColor = (status: string): 'success' | 'warning' | 'error' => {
    switch (status) {
      case 'online': return 'success';
      case 'offline': return 'warning';
      case 'error': return 'error';
      default: return 'warning';
    }
  };

  const getBatteryColor = (battery: number) => {
    if (battery > 60) return 'success';
    if (battery > 30) return 'warning';
    return 'error';
  };

  // Get unique locations for filter dropdown
  const uniqueLocations = Array.from(new Set(devices.map(d => d.location?.split(' - ')[0] || 'Unknown')));

  // Filter devices based on search and filters
  const filteredTricorders = devices.filter(device => {
    const matchesSearch = device.device_id.toLowerCase().includes(searchQuery.toLowerCase()) ||
                         (device.location?.toLowerCase().includes(searchQuery.toLowerCase()) || false) ||
                         (device.ip_address?.includes(searchQuery) || false);
    
    const matchesStatus = statusFilter === 'all' || device.status === statusFilter;
    
    const matchesLocation = locationFilter === 'all' || device.location?.startsWith(locationFilter);
    
    return matchesSearch && matchesStatus && matchesLocation;
  });

  const handleMenuClick = (event: React.MouseEvent<HTMLElement>) => {
    setAnchorEl(event.currentTarget);
  };

  const handleMenuClose = () => {
    setAnchorEl(null);
  };

  const getStatusIcon = (status: string) => {
    switch (status) {
      case 'online': return <WifiIcon color="success" />;
      case 'offline': return <WifiOffIcon color="disabled" />;
      case 'error': return <ErrorIcon color="error" />;
      default: return <WifiOffIcon color="disabled" />;
    }
  };

  const openDeviceSettings = (device: TricorderDevice) => {
    setSelectedDevice(device);
    setSettingsOpen(true);
  };

  const closeDeviceSettings = () => {
    setSettingsOpen(false);
    setSelectedDevice(null);
  };

  const saveDeviceSettings = (deviceId: string, settings: any) => {
    console.log('Saving settings for device:', deviceId, settings);
    // Here you would typically send the settings to your backend
    // For now, we'll just log them
  };

  return (
    <Box sx={{ flexGrow: 1 }}>
      {/* Header Bar */}
      <AppBar position="static" sx={{ mb: 3 }}>
        <Toolbar>
          <DashboardIcon sx={{ mr: 2 }} />
          <Typography variant="h6" component="div" sx={{ flexGrow: 1 }}>
            🚀 Tricorder Control Farm
          </Typography>
          <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
            <Badge badgeContent={filteredTricorders.filter(d => d.status === 'online').length} color="success">
              <Chip 
                label={`${filteredTricorders.length} Total`} 
                color="primary" 
                size="small"
              />
            </Badge>
            <Badge badgeContent={filteredTricorders.filter(d => d.status === 'error').length} color="error">
              <Chip 
                label="Errors" 
                color="error" 
                size="small"
                sx={{ display: filteredTricorders.filter(d => d.status === 'error').length > 0 ? 'flex' : 'none' }}
              />
            </Badge>
            <IconButton color="inherit" onClick={handleMenuClick}>
              <MoreVertIcon />
            </IconButton>
          </Box>
        </Toolbar>
      </AppBar>

      <Container maxWidth="xl">
        {/* Filter Controls */}
        <Paper elevation={2} sx={{ p: 3, mb: 3 }}>
          <Grid container spacing={3} alignItems="center">
            <Grid item xs={12} md={4}>
              <TextField
                fullWidth
                variant="outlined"
                label="Search devices..."
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                InputProps={{
                  startAdornment: (
                    <InputAdornment position="start">
                      <SearchIcon />
                    </InputAdornment>
                  ),
                }}
              />
            </Grid>
            <Grid item xs={6} md={2}>
              <FormControl fullWidth>
                <InputLabel>Status</InputLabel>
                <Select
                  value={statusFilter}
                  label="Status"
                  onChange={(e) => setStatusFilter(e.target.value)}
                >
                  <MenuItem value="all">All Status</MenuItem>
                  <MenuItem value="online">Online</MenuItem>
                  <MenuItem value="offline">Offline</MenuItem>
                  <MenuItem value="error">Error</MenuItem>
                </Select>
              </FormControl>
            </Grid>
            <Grid item xs={6} md={2}>
              <FormControl fullWidth>
                <InputLabel>Location</InputLabel>
                <Select
                  value={locationFilter}
                  label="Location"
                  onChange={(e) => setLocationFilter(e.target.value)}
                >
                  <MenuItem value="all">All Locations</MenuItem>
                  {uniqueLocations.map(location => (
                    <MenuItem key={location} value={location}>{location}</MenuItem>
                  ))}
                </Select>
              </FormControl>
            </Grid>
            <Grid item xs={12} md={4}>
              <Box sx={{ display: 'flex', gap: 1, justifyContent: 'flex-end' }}>
                <Button 
                  variant="outlined" 
                  startIcon={<FilterIcon />}
                  onClick={() => {
                    setSearchQuery('');
                    setStatusFilter('all');
                    setLocationFilter('all');
                  }}
                >
                  Clear Filters
                </Button>
                <Button 
                  variant="contained" 
                  startIcon={<RefreshIcon />}
                  onClick={refreshDevices}
                  disabled={isLoading}
                >
                  {isLoading ? 'Refreshing...' : 'Refresh'}
                </Button>
              </Box>
            </Grid>
          </Grid>
        </Paper>

        {/* Device Grid */}
        <Grid container spacing={3}>
          {filteredTricorders.map((device) => (
            <Grid item xs={12} sm={6} lg={4} xl={3} key={device.device_id}>
              <Card
                onClick={() => toggleSelection(device.device_id)}
                sx={{
                  cursor: 'pointer',
                  transition: 'all 0.3s ease',
                  backgroundColor: selectedDevices.has(device.device_id) ? '#e3f2fd' : '#fff',
                  border: selectedDevices.has(device.device_id) ? '2px solid #2196f3' : '1px solid #ddd',
                  position: 'relative',
                  '&:hover': {
                    boxShadow: 6,
                    transform: 'translateY(-4px)',
                  },
                }}
              >
                <CardContent>
                  {/* Device Header */}
                  <Box display="flex" justifyContent="space-between" alignItems="center" mb={2}>
                    <Typography variant="h6" component="h3" sx={{ color: '#333', fontWeight: 'bold' }}>
                      {device.device_id}
                    </Typography>
                    <Box display="flex" alignItems="center" gap={1}>
                      {getStatusIcon(device.status)}
                      <Chip
                        label={device.status.toUpperCase()}
                        color={getStatusColor(device.status)}
                        size="small"
                        sx={{ fontWeight: 'bold' }}
                      />
                    </Box>
                  </Box>

                  {/* Network Info */}
                  <Box mb={2}>
                    <Typography variant="body2" color="text.secondary" sx={{ fontFamily: 'monospace' }}>
                      IP: {device.ip_address || 'Unknown'}
                    </Typography>
                    <Typography variant="body2" color="text.secondary">
                      FW: v{device.firmware_version || 'Unknown'}
                    </Typography>
                  </Box>

                  {/* Location */}
                  <Box display="flex" alignItems="center" mb={2}>
                    <LocationIcon sx={{ color: '#666', mr: 1, fontSize: 18 }} />
                    <Typography variant="body2" color="text.secondary">
                      {device.location || 'Unknown Location'}
                    </Typography>
                  </Box>

                  {/* Temperature */}
                  {device.temperature && (
                    <Box display="flex" justifyContent="space-between" alignItems="center" mb={2}>
                      <Typography variant="body2" color="text.secondary">
                        Temperature
                      </Typography>
                      <Typography 
                        variant="body2" 
                        sx={{ 
                          fontWeight: 'bold',
                          color: device.temperature > 30 ? '#f44336' : device.temperature < 15 ? '#2196f3' : '#4caf50'
                        }}
                      >
                        {device.temperature}°C
                      </Typography>
                    </Box>
                  )}

                  {/* Battery Status */}
                  {device.battery !== undefined && (
                    <Box mb={2}>
                      <Box display="flex" justifyContent="space-between" alignItems="center" mb={1}>
                        <Box display="flex" alignItems="center">
                          <BatteryIcon sx={{ color: '#666', mr: 1, fontSize: 18 }} />
                          <Typography variant="body2" color="text.secondary">
                            Battery
                          </Typography>
                        </Box>
                        <Typography 
                          variant="body2" 
                          sx={{ 
                            fontWeight: 'bold',
                            color: device.battery > 60 ? '#4caf50' : device.battery > 30 ? '#ff9800' : '#f44336'
                          }}
                        >
                          {device.battery}%
                        </Typography>
                      </Box>
                      <LinearProgress
                        variant="determinate"
                        value={device.battery}
                        color={getBatteryColor(device.battery)}
                        sx={{ height: 8, borderRadius: 4 }}
                      />
                    </Box>
                  )}

                  {/* Video Status */}
                  <Box display="flex" justifyContent="space-between" alignItems="center" mb={2}>
                    <Box display="flex" alignItems="center">
                      {device.video_playing ? (
                        <VideoIcon sx={{ color: '#4caf50', mr: 1, fontSize: 18 }} />
                      ) : (
                        <VideocamOffIcon sx={{ color: '#666', mr: 1, fontSize: 18 }} />
                      )}
                      <Typography variant="body2" color="text.secondary">
                        Video
                      </Typography>
                    </Box>
                    <Chip
                      label={device.video_playing ? 'Playing' : 'Stopped'}
                      color={device.video_playing ? 'success' : 'default'}
                      size="small"
                      variant="outlined"
                    />
                  </Box>

                  {/* Current Video */}
                  {device.current_video && (
                    <Box mb={2}>
                      <Typography variant="body2" color="text.secondary" sx={{ fontStyle: 'italic' }}>
                        📹 {device.current_video}
                      </Typography>
                    </Box>
                  )}

                  {/* Last Seen */}
                  <Typography variant="caption" color="text.disabled">
                    Last seen: {device.last_seen ? new Date(device.last_seen).toLocaleTimeString() : 'Unknown'}
                  </Typography>
                </CardContent>

                {/* Action Buttons */}
                <CardActions sx={{ px: 2, pb: 2 }}>
                  <Button
                    size="small"
                    variant="contained"
                    color="success"
                    startIcon={<PlayIcon />}
                    sx={{ flex: 1, mr: 1 }}
                    disabled={device.status !== 'online' || isExecutingCommand}
                    onClick={(e) => {
                      e.stopPropagation();
                      executeCommand(device.device_id, 'play');
                    }}
                  >
                    Play
                  </Button>
                  <Button
                    size="small"
                    variant="contained"
                    color="warning"
                    startIcon={<StopIcon />}
                    sx={{ flex: 1, mr: 1 }}
                    disabled={device.status !== 'online' || isExecutingCommand}
                    onClick={(e) => {
                      e.stopPropagation();
                      executeCommand(device.device_id, 'stop');
                    }}
                  >
                    Stop
                  </Button>
                  <Button
                    size="small"
                    variant="contained"
                    color="info"
                    startIcon={<UpdateIcon />}
                    sx={{ flex: 1, mr: 1 }}
                    disabled={device.status !== 'online'}
                    onClick={(e) => {
                      e.stopPropagation();
                      setUpdateDevice(device);
                      setFirmwareUpdateOpen(true);
                    }}
                  >
                    Update
                  </Button>
                  <Button
                    size="small"
                    variant="contained"
                    color="primary"
                    startIcon={<SettingsIcon />}
                    sx={{ flex: 1 }}
                    onClick={(e) => {
                      e.stopPropagation();
                      openDeviceSettings(device);
                    }}
                  >
                    Settings
                  </Button>
                </CardActions>

                {/* Selection Indicator */}
                {selectedDevices.has(device.device_id) && (
                  <Box
                    sx={{
                      position: 'absolute',
                      top: 8,
                      right: 8,
                      backgroundColor: '#2196f3',
                      color: 'white',
                      borderRadius: '50%',
                      width: 24,
                      height: 24,
                      display: 'flex',
                      alignItems: 'center',
                      justifyContent: 'center',
                      fontSize: 14,
                      fontWeight: 'bold',
                    }}
                  >
                    ✓
                  </Box>
                )}
              </Card>
            </Grid>
          ))}
        </Grid>

        {/* Selection Counter and Menu */}
        {selectedDevices.size > 0 && (
          <Paper
            elevation={6}
            sx={{
              position: 'fixed',
              bottom: 80,
              right: 20,
              px: 3,
              py: 2,
              backgroundColor: '#2196f3',
              color: 'white',
              borderRadius: 3,
              display: 'flex',
              alignItems: 'center',
              gap: 2,
            }}
          >
            <Typography variant="h6" fontWeight="bold">
              {selectedDevices.size} selected
            </Typography>
            <Box sx={{ display: 'flex', gap: 1 }}>
              <Button 
                size="small" 
                variant="contained" 
                color="success" 
                startIcon={<PlayIcon />}
                disabled={isExecutingCommand}
                onClick={() => executeBulkCommand('play')}
              >
                Play All
              </Button>
              <Button 
                size="small" 
                variant="contained" 
                color="warning" 
                startIcon={<StopIcon />}
                disabled={isExecutingCommand}
                onClick={() => executeBulkCommand('stop')}
              >
                Stop All
              </Button>
            </Box>
          </Paper>
        )}

        {/* Add Device Button */}
        <Fab
          color="primary"
          aria-label="add device"
          sx={{
            position: 'fixed',
            bottom: 20,
            right: 20,
          }}
        >
          <AddIcon />
        </Fab>

        {/* More Menu */}
        <Menu
          anchorEl={anchorEl}
          open={Boolean(anchorEl)}
          onClose={handleMenuClose}
          anchorOrigin={{
            vertical: 'bottom',
            horizontal: 'right',
          }}
          transformOrigin={{
            vertical: 'top',
            horizontal: 'right',
          }}
        >
          <MenuItem onClick={() => {
            handleMenuClose();
            refreshDevices();
          }}>
            <ListItemIcon>
              <RefreshIcon fontSize="small" />
            </ListItemIcon>
            <ListItemText>Refresh All Devices</ListItemText>
          </MenuItem>
          <MenuItem onClick={() => {
            handleMenuClose();
            // Open global firmware management modal
            setUpdateDevice(null); // No specific device
            setFirmwareUpdateOpen(true);
          }}>
            <ListItemIcon>
              <UpdateIcon fontSize="small" />
            </ListItemIcon>
            <ListItemText>Firmware Management</ListItemText>
          </MenuItem>
          <MenuItem onClick={handleMenuClose}>
            <ListItemIcon>
              <SettingsIcon fontSize="small" />
            </ListItemIcon>
            <ListItemText>System Settings</ListItemText>
          </MenuItem>
        </Menu>

        {/* Statistics Footer */}
        <Paper elevation={1} sx={{ mt: 4, p: 2, backgroundColor: '#f8f9fa' }}>
          <Grid container spacing={2} justifyContent="center">
            <Grid item>
              <Chip 
                icon={<WifiIcon />}
                label={`${filteredTricorders.filter(d => d.status === 'online').length} Online`} 
                color="success" 
                variant="outlined"
              />
            </Grid>
            <Grid item>
              <Chip 
                icon={<WifiOffIcon />}
                label={`${filteredTricorders.filter(d => d.status === 'offline').length} Offline`} 
                color="warning" 
                variant="outlined"
              />
            </Grid>
            <Grid item>
              <Chip 
                icon={<ErrorIcon />}
                label={`${filteredTricorders.filter(d => d.status === 'error').length} Errors`} 
                color="error" 
                variant="outlined"
              />
            </Grid>
            <Grid item>
              <Chip 
                icon={<VideoIcon />}
                label={`${filteredTricorders.filter(d => d.video_playing).length} Playing Video`} 
                color="info" 
                variant="outlined"
              />
            </Grid>
          </Grid>
        </Paper>
      </Container>

      {/* Settings Modal */}
      <TricorderSettingsModal
        open={settingsOpen}
        onClose={closeDeviceSettings}
        device={selectedDevice}
        onSave={saveDeviceSettings}
      />

      {/* Firmware Update Modal */}
      {updateDevice && (
        <FirmwareUpdateModal
          open={firmwareUpdateOpen}
          onClose={() => {
            setFirmwareUpdateOpen(false);
            setUpdateDevice(null);
          }}
          device={updateDevice}
        />
      )}
    </Box>
  );
};

export default TricorderFarmDashboard;