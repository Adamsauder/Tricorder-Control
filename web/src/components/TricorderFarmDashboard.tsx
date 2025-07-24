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
} from '@mui/icons-material';
import TricorderSettingsModal from './TricorderSettingsModal';
import FirmwareUpdateModal from './FirmwareUpdateModal';

interface TricorderDevice {
  device_id: string;
  status: 'online' | 'offline' | 'error';
  battery: number;
  video_playing: boolean;
  location: string;
  ip_address: string;
  firmware_version: string;
  last_seen: string;
  temperature?: number;
  current_video?: string;
  prop_profile?: string;
  dmx_start_address?: number;
  dmx_universe?: number;
}

const TricorderFarmDashboard: React.FC = () => {
  const [selectedDevices, setSelectedDevices] = useState<Set<string>>(new Set());
  const [searchQuery, setSearchQuery] = useState('');
  const [statusFilter, setStatusFilter] = useState<string>('all');
  const [locationFilter, setLocationFilter] = useState<string>('all');
  const [anchorEl, setAnchorEl] = useState<null | HTMLElement>(null);
  const [settingsOpen, setSettingsOpen] = useState(false);
  const [selectedDevice, setSelectedDevice] = useState<TricorderDevice | null>(null);
  const [firmwareUpdateOpen, setFirmwareUpdateOpen] = useState(false);
  const [updateDevice, setUpdateDevice] = useState<TricorderDevice | null>(null);

  const mockTricorders: TricorderDevice[] = [
    { 
      device_id: 'TRICORDER_001', 
      status: 'online', 
      battery: 85, 
      video_playing: true, 
      location: 'Set A - Bridge',
      ip_address: '192.168.1.100',
      firmware_version: '1.2.3',
      last_seen: '2024-01-15T10:30:00Z',
      temperature: 24.5,
      current_video: 'bridge_scan.mp4',
      prop_profile: 'tricorder_mk1',
      dmx_start_address: 1,
      dmx_universe: 1
    },
    { 
      device_id: 'TRICORDER_002', 
      status: 'offline', 
      battery: 42, 
      video_playing: false, 
      location: 'Set B - Engineering',
      ip_address: '192.168.1.101',
      firmware_version: '1.2.3',
      last_seen: '2024-01-15T09:45:00Z',
      temperature: 28.2,
      prop_profile: 'tricorder_mk2',
      dmx_start_address: 16,
      dmx_universe: 1
    },
    { 
      device_id: 'TRICORDER_003', 
      status: 'online', 
      battery: 95, 
      video_playing: true, 
      location: 'Set C - Sick Bay',
      ip_address: '192.168.1.102',
      firmware_version: '1.2.3',
      last_seen: '2024-01-15T10:32:00Z',
      temperature: 22.1,
      current_video: 'medical_readout.mp4',
      prop_profile: 'tricorder_mk1',
      dmx_start_address: 32,
      dmx_universe: 1
    },
    { 
      device_id: 'TRICORDER_004', 
      status: 'error', 
      battery: 12, 
      video_playing: false, 
      location: 'Set D - Ready Room',
      ip_address: '192.168.1.103',
      firmware_version: '1.1.8',
      last_seen: '2024-01-15T08:15:00Z',
      temperature: 31.0,
      prop_profile: 'communicator',
      dmx_start_address: 48,
      dmx_universe: 2
    },
    { 
      device_id: 'TRICORDER_005', 
      status: 'online', 
      battery: 78, 
      video_playing: true, 
      location: 'Set A - Transporter',
      ip_address: '192.168.1.104',
      firmware_version: '1.2.3',
      last_seen: '2024-01-15T10:29:00Z',
      temperature: 26.7,
      current_video: 'transport_pattern.mp4',
      prop_profile: 'padd',
      dmx_start_address: 64,
      dmx_universe: 2
    },
    { 
      device_id: 'TRICORDER_006', 
      status: 'online', 
      battery: 62, 
      video_playing: false, 
      location: 'Set E - Cargo Bay',
      ip_address: '192.168.1.105',
      firmware_version: '1.2.3',
      last_seen: '2024-01-15T10:31:00Z',
      temperature: 20.3,
      prop_profile: 'tricorder_mk2',
      dmx_start_address: 80,
      dmx_universe: 2
    },
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
  const uniqueLocations = Array.from(new Set(mockTricorders.map(d => d.location.split(' - ')[0])));

  // Filter devices based on search and filters
  const filteredTricorders = mockTricorders.filter(device => {
    const matchesSearch = device.device_id.toLowerCase().includes(searchQuery.toLowerCase()) ||
                         device.location.toLowerCase().includes(searchQuery.toLowerCase()) ||
                         device.ip_address.includes(searchQuery);
    
    const matchesStatus = statusFilter === 'all' || device.status === statusFilter;
    
    const matchesLocation = locationFilter === 'all' || device.location.startsWith(locationFilter);
    
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
                >
                  Refresh
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
                onClick={() => toggleDeviceSelection(device.device_id)}
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
                      IP: {device.ip_address}
                    </Typography>
                    <Typography variant="body2" color="text.secondary">
                      FW: v{device.firmware_version}
                    </Typography>
                  </Box>

                  {/* Location */}
                  <Box display="flex" alignItems="center" mb={2}>
                    <LocationIcon sx={{ color: '#666', mr: 1, fontSize: 18 }} />
                    <Typography variant="body2" color="text.secondary">
                      {device.location}
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
                    Last seen: {new Date(device.last_seen).toLocaleTimeString()}
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
                    disabled={device.status !== 'online'}
                  >
                    Play
                  </Button>
                  <Button
                    size="small"
                    variant="contained"
                    color="warning"
                    startIcon={<StopIcon />}
                    sx={{ flex: 1, mr: 1 }}
                    disabled={device.status !== 'online'}
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
              <Button size="small" variant="contained" color="success" startIcon={<PlayIcon />}>
                Play All
              </Button>
              <Button size="small" variant="contained" color="warning" startIcon={<StopIcon />}>
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
          <MenuItem onClick={handleMenuClose}>
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