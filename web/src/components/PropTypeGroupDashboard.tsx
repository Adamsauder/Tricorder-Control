import React, { useState, useEffect } from 'react';
import {
  AppBar,
  Toolbar,
  Box,
  Container,
  Typography,
  Grid,
  Alert,
  Snackbar,
  CircularProgress,
  Fab,
  IconButton,
  Menu,
  MenuItem,
  ListItemIcon,
  ListItemText,
} from '@mui/material';
import {
  Add as AddIcon,
  Dashboard as DashboardIcon,
  Refresh as RefreshIcon,
  MoreVert as MoreVertIcon,
  Settings as SettingsIcon,
} from '@mui/icons-material';
import PropTypeCard from './PropTypeCard';

// API functions for prop-type operations
const API_BASE = 'http://localhost:8080';

interface PropType {
  type: string;
  total_devices: number;
  online_devices: number;
  devices: any[];
}

interface PropTypesResponse {
  prop_types: PropType[];
  total_types: number;
}

const PropTypeGroupDashboard: React.FC = () => {
  const [propTypes, setPropTypes] = useState<PropType[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [snackbarOpen, setSnackbarOpen] = useState(false);
  const [snackbarMessage, setSnackbarMessage] = useState('');
  const [anchorEl, setAnchorEl] = useState<null | HTMLElement>(null);

  // Fetch prop types from the server
  const fetchPropTypes = async () => {
    setLoading(true);
    setError(null);
    
    try {
      const response = await fetch(`${API_BASE}/api/props`);
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
      
      const data: PropTypesResponse = await response.json();
      setPropTypes(data.prop_types);
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Unknown error occurred';
      setError(errorMessage);
      showSnackbar(`Failed to fetch prop types: ${errorMessage}`);
    } finally {
      setLoading(false);
    }
  };

  // Show snackbar notification
  const showSnackbar = (message: string) => {
    setSnackbarMessage(message);
    setSnackbarOpen(true);
  };

  // Handle SACN address change for a prop type
  const handleSacnAddressChange = async (propType: string, universe: number, address: number) => {
    try {
      const response = await fetch(`${API_BASE}/api/props/${propType}/sacn/address`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ universe, address }),
      });

      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
      }

      const result = await response.json();
      showSnackbar(`SACN address updated for ${result.devices_updated}/${result.total_devices} ${propType} devices`);
      
      // Refresh prop types to get updated data
      fetchPropTypes();
    } catch (err) {
      showSnackbar(`Failed to update SACN address: ${err instanceof Error ? err.message : 'Unknown error'}`);
    }
  };

  // Handle firmware update for a prop type
  const handleFirmwareUpdate = async (propType: string, file: File) => {
    try {
      const formData = new FormData();
      formData.append('firmware', file);

      const response = await fetch(`${API_BASE}/api/props/${propType}/firmware/update`, {
        method: 'POST',
        body: formData,
      });

      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
      }

      const result = await response.json();
      showSnackbar(`Firmware updated on ${result.successful_updates}/${result.total_devices} ${propType} devices`);
      
      // Refresh prop types to get updated data
      fetchPropTypes();
    } catch (err) {
      showSnackbar(`Failed to update firmware: ${err instanceof Error ? err.message : 'Unknown error'}`);
    }
  };

  // Handle bulk command for a prop type
  const handleBulkCommand = async (propType: string, action: string, parameters: any = {}) => {
    try {
      const response = await fetch(`${API_BASE}/api/props/${propType}/command`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ action, parameters }),
      });

      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
      }

      const result = await response.json();
      showSnackbar(`Command '${action}' sent to ${result.devices_updated}/${result.total_devices} ${propType} devices`);
    } catch (err) {
      showSnackbar(`Failed to send command: ${err instanceof Error ? err.message : 'Unknown error'}`);
    }
  };

  // Handle menu actions
  const handleMenuClick = (event: React.MouseEvent<HTMLElement>) => {
    setAnchorEl(event.currentTarget);
  };

  const handleMenuClose = () => {
    setAnchorEl(null);
  };

  // Initial data load
  useEffect(() => {
    fetchPropTypes();
    
    // Set up auto-refresh every 10 seconds
    const interval = setInterval(fetchPropTypes, 10000);
    return () => clearInterval(interval);
  }, []);

  const totalDevices = propTypes.reduce((sum, type) => sum + type.total_devices, 0);
  const totalOnline = propTypes.reduce((sum, type) => sum + type.online_devices, 0);

  return (
    <Box sx={{ flexGrow: 1 }}>
      {/* Header Bar */}
      <AppBar position="static" sx={{ mb: 3 }}>
        <Toolbar>
          <DashboardIcon sx={{ mr: 2 }} />
          <Typography variant="h6" component="div" sx={{ flexGrow: 1 }}>
            🎬 Prop Control System - Group Management
          </Typography>
          <Box sx={{ display: 'flex', alignItems: 'center', gap: 2 }}>
            <Typography variant="body2">
              {totalOnline}/{totalDevices} devices online
            </Typography>
            <IconButton color="inherit" onClick={fetchPropTypes} disabled={loading}>
              <RefreshIcon />
            </IconButton>
            <IconButton color="inherit" onClick={handleMenuClick}>
              <MoreVertIcon />
            </IconButton>
          </Box>
        </Toolbar>
      </AppBar>

      <Container maxWidth="xl">
        {/* Error Alert */}
        {error && (
          <Alert severity="error" sx={{ mb: 3 }} onClose={() => setError(null)}>
            {error}
          </Alert>
        )}

        {/* Loading Indicator */}
        {loading && (
          <Box display="flex" justifyContent="center" my={4}>
            <CircularProgress />
          </Box>
        )}

        {/* No Data Message */}
        {!loading && propTypes.length === 0 && (
          <Alert severity="info" sx={{ mb: 3 }}>
            No prop devices found. Make sure devices are powered on and connected to the network.
          </Alert>
        )}

        {/* Prop Type Cards */}
        <Grid container spacing={3}>
          {propTypes.map((propType) => (
            <Grid item xs={12} lg={6} xl={4} key={propType.type}>
              <PropTypeCard
                propType={propType.type}
                devices={propType.devices}
                onSacnAddressChange={handleSacnAddressChange}
                onFirmwareUpdate={handleFirmwareUpdate}
                onBulkCommand={handleBulkCommand}
              />
            </Grid>
          ))}
        </Grid>

        {/* System Statistics */}
        {propTypes.length > 0 && (
          <Box mt={4} p={3} bgcolor="background.paper" borderRadius={2}>
            <Typography variant="h6" gutterBottom>
              System Overview
            </Typography>
            <Grid container spacing={2}>
              <Grid item xs={6} md={3}>
                <Typography variant="h4" color="primary">
                  {propTypes.length}
                </Typography>
                <Typography variant="body2" color="text.secondary">
                  Prop Types
                </Typography>
              </Grid>
              <Grid item xs={6} md={3}>
                <Typography variant="h4" color="success.main">
                  {totalOnline}
                </Typography>
                <Typography variant="body2" color="text.secondary">
                  Online Devices
                </Typography>
              </Grid>
              <Grid item xs={6} md={3}>
                <Typography variant="h4" color="info.main">
                  {totalDevices}
                </Typography>
                <Typography variant="body2" color="text.secondary">
                  Total Devices
                </Typography>
              </Grid>
              <Grid item xs={6} md={3}>
                <Typography variant="h4" color="warning.main">
                  {totalDevices - totalOnline}
                </Typography>
                <Typography variant="body2" color="text.secondary">
                  Offline Devices
                </Typography>
              </Grid>
            </Grid>
          </Box>
        )}
      </Container>

      {/* Add Device Button */}
      <Fab
        color="primary"
        aria-label="refresh"
        sx={{ position: 'fixed', bottom: 20, right: 20 }}
        onClick={fetchPropTypes}
      >
        <RefreshIcon />
      </Fab>

      {/* More Menu */}
      <Menu
        anchorEl={anchorEl}
        open={Boolean(anchorEl)}
        onClose={handleMenuClose}
        anchorOrigin={{ vertical: 'bottom', horizontal: 'right' }}
        transformOrigin={{ vertical: 'top', horizontal: 'right' }}
      >
        <MenuItem onClick={() => { handleMenuClose(); fetchPropTypes(); }}>
          <ListItemIcon>
            <RefreshIcon fontSize="small" />
          </ListItemIcon>
          <ListItemText>Refresh All Data</ListItemText>
        </MenuItem>
        <MenuItem onClick={handleMenuClose}>
          <ListItemIcon>
            <SettingsIcon fontSize="small" />
          </ListItemIcon>
          <ListItemText>System Settings</ListItemText>
        </MenuItem>
      </Menu>

      {/* Snackbar Notifications */}
      <Snackbar
        open={snackbarOpen}
        autoHideDuration={6000}
        onClose={() => setSnackbarOpen(false)}
        message={snackbarMessage}
      />
    </Box>
  );
};

export default PropTypeGroupDashboard;
