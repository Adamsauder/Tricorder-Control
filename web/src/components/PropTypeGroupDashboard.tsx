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
const API_BASE = process.env.NODE_ENV === 'development' ? '' : 'http://localhost:8080';

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
  const [operationInProgress, setOperationInProgress] = useState(false);
  const [snackbarOpen, setSnackbarOpen] = useState(false);
  const [snackbarMessage, setSnackbarMessage] = useState('');
  const [anchorEl, setAnchorEl] = useState<null | HTMLElement>(null);
  const [hasError, setHasError] = useState(false);

  // Catch any errors in the component
  useEffect(() => {
    const errorHandler = (error: ErrorEvent) => {
      console.error('PropTypeGroupDashboard Error:', error);
      setHasError(true);
      setError(`JavaScript Error: ${error.message}`);
    };

    window.addEventListener('error', errorHandler);
    return () => window.removeEventListener('error', errorHandler);
  }, []);

  // Fetch prop types from the server
  const fetchPropTypes = async (showLoadingSpinner = true) => {
    if (showLoadingSpinner) {
      setLoading(true);
      setError(null);
    }
    
    try {
      const response = await fetch(`${API_BASE}/api/props`);
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
      
      const data: PropTypesResponse = await response.json();
      
      // For silent auto-refresh, only update if data has actually changed
      if (!showLoadingSpinner) {
        const currentDataStr = JSON.stringify(propTypes);
        const newDataStr = JSON.stringify(data.prop_types);
        if (currentDataStr === newDataStr) {
          // Data hasn't changed, skip update to prevent unnecessary re-render
          return;
        }
      }
      
      setPropTypes(data.prop_types);
    } catch (err) {
      if (showLoadingSpinner) {
        const errorMessage = err instanceof Error ? err.message : 'Unknown error occurred';
        setError(errorMessage);
        showSnackbar(`Failed to fetch prop types: ${errorMessage}`);
      }
      // Silent auto-refresh errors are ignored to prevent UI disruption
    } finally {
      if (showLoadingSpinner) {
        setLoading(false);
      }
    }
  };

  // Show snackbar notification
  const showSnackbar = (message: string) => {
    setSnackbarMessage(message);
    setSnackbarOpen(true);
  };

  // Handle SACN address change for a prop type
  const handleSacnAddressChange = async (propType: string, universe: number, address: number) => {
    console.log(`🔧 Setting SACN address for ${propType}: universe=${universe}, address=${address}`);
    setOperationInProgress(true);
    try {
      const payload = { universe, address };
      console.log('📤 Sending payload:', JSON.stringify(payload));
      
      const response = await fetch(`${API_BASE}/api/props/${propType}/sacn/address`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(payload),
      });

      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
      }

      const result = await response.json();
      showSnackbar(`SACN address updated for ${result.devices_updated}/${result.total_devices} ${propType} devices`);
      
      // Wait for device states to stabilize before allowing auto-refresh
      setTimeout(() => {
        setOperationInProgress(false);
        // Refresh prop types to get updated data
        fetchPropTypes();
      }, 2000);
    } catch (err) {
      setOperationInProgress(false);
      showSnackbar(`Failed to update SACN address: ${err instanceof Error ? err.message : 'Unknown error'}`);
    }
  };

  // Handle firmware update for a prop type
  const handleFirmwareUpdate = async (propType: string, file: File) => {
    setOperationInProgress(true);
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
      
      // Wait for device states to stabilize before allowing auto-refresh
      setTimeout(() => {
        setOperationInProgress(false);
        // Refresh prop types to get updated data
        fetchPropTypes();
      }, 3000); // Longer delay for firmware updates
    } catch (err) {
      setOperationInProgress(false);
      showSnackbar(`Failed to update firmware: ${err instanceof Error ? err.message : 'Unknown error'}`);
    }
  };

  // Handle bulk command for a prop type
  const handleBulkCommand = async (propType: string, action: string, parameters: any = {}) => {
    setOperationInProgress(true);
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
      
      // Wait for device states to stabilize before allowing auto-refresh
      setTimeout(() => {
        setOperationInProgress(false);
      }, 1500);
    } catch (err) {
      setOperationInProgress(false);
      showSnackbar(`Failed to send command: ${err instanceof Error ? err.message : 'Unknown error'}`);
    }
  };

  // Handle saving current SACN state as default for a prop type
  const handleSaveCurrentAsDefault = async (propType: string) => {
    setOperationInProgress(true);
    try {
      const response = await fetch(`${API_BASE}/api/props/${propType}/save-current-as-default`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
      });

      if (!response.ok) {
        throw new Error(`HTTP ${response.status}`);
      }

      const result = await response.json();
      showSnackbar(`Saved current LED state as default for ${result.devices_updated}/${result.total_devices} ${propType} devices`);
      
      // Wait for device states to stabilize before allowing auto-refresh
      setTimeout(() => {
        setOperationInProgress(false);
      }, 2000);
    } catch (err) {
      setOperationInProgress(false);
      showSnackbar(`Failed to save current state as default: ${err instanceof Error ? err.message : 'Unknown error'}`);
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
    
    // Set up auto-refresh every 10 seconds, but skip if operations are in progress
    const interval = setInterval(() => {
      if (!operationInProgress) {
        fetchPropTypes(false);
      }
    }, 10000);
    return () => clearInterval(interval);
  }, [operationInProgress]);

  const totalDevices = propTypes.reduce((sum, type) => sum + type.total_devices, 0);
  const totalOnline = propTypes.reduce((sum, type) => sum + type.online_devices, 0);

  // Show error state if there's an error
  if (hasError) {
    return (
      <Container maxWidth="lg" sx={{ mt: 4, mb: 4 }}>
        <Alert severity="error" sx={{ mb: 2 }}>
          <Typography variant="h6">Component Error</Typography>
          <Typography>{error || 'An unknown error occurred in PropTypeGroupDashboard'}</Typography>
        </Alert>
      </Container>
    );
  }

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
            <IconButton color="inherit" onClick={() => fetchPropTypes(true)} disabled={loading}>
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
                onSaveCurrentAsDefault={handleSaveCurrentAsDefault}
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
        onClick={() => fetchPropTypes(true)}
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
        <MenuItem onClick={() => { handleMenuClose(); fetchPropTypes(true); }}>
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

      {/* Fixed Loading Indicator at Bottom */}
      {loading && (
        <Box
          sx={{
            position: 'fixed',
            bottom: 20,
            left: '50%',
            transform: 'translateX(-50%)',
            zIndex: 1300,
            display: 'flex',
            alignItems: 'center',
            gap: 1,
            bgcolor: 'background.paper',
            borderRadius: 2,
            px: 2,
            py: 1,
            boxShadow: 2,
          }}
        >
          <CircularProgress size={20} />
          <Typography variant="body2" color="text.secondary">
            Refreshing...
          </Typography>
        </Box>
      )}
    </Box>
  );
};

export default PropTypeGroupDashboard;
