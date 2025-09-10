import React, { useState, useEffect } from 'react';
import {
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  Paper,
  TextField,
  Switch,
  IconButton,
  Chip,
  Box,
  Typography,
  Alert,
  CircularProgress,
  FormControlLabel,
  InputAdornment,
  Tooltip,
  Button,
  Stack,
  Divider,
  Grid,
  Select,
  MenuItem,
  FormControl,
  InputLabel,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Checkbox,
} from '@mui/material';
import {
  Save as SaveIcon,
  Cancel as CancelIcon,
  Edit as EditIcon,
  Check as CheckIcon,
  Wifi as WifiIcon,
  WifiOff as WifiOffIcon,
  Refresh as RefreshIcon,
  Settings as SettingsIcon,
} from '@mui/icons-material';

const API_BASE = 'http://192.168.1.24:8080';

interface Device {
  device_id: string;
  device_type: string;
  device_label: string;
  ip_address: string;
  sacn_universe?: number;
  dmx_address?: number;
  sacn_enabled?: boolean;
  status: string;
  last_seen: string;
  firmware_version?: string;
  brightness?: number;
  num_leds?: number;
  uptime?: number;
  wifi_connected?: boolean;
  // Network configuration
  useDHCP?: boolean;
  staticIP?: string;
  staticGateway?: string;
  staticSubnet?: string;
  staticDNS?: string;
}

interface EditableCell {
  deviceId: string;
  field: string;
  value: any;
}

const DeviceTableView: React.FC = () => {
  const [devices, setDevices] = useState<Device[]>([]);
  const [filteredDevices, setFilteredDevices] = useState<Device[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [editingCell, setEditingCell] = useState<EditableCell | null>(null);
  const [tempValue, setTempValue] = useState<string>('');
  const [saving, setSaving] = useState<string | null>(null);
  const [deviceTypeFilter, setDeviceTypeFilter] = useState<string>('all');
  const [bulkConfigOpen, setBulkConfigOpen] = useState(false);
  const [selectedDevices, setSelectedDevices] = useState<Set<string>>(new Set());
  const [bulkUniverse, setBulkUniverse] = useState<string>('1');
  const [bulkStartAddress, setBulkStartAddress] = useState<string>('1');

  // Fetch all devices from the server
  const fetchDevices = async () => {
    try {
      setLoading(true);
      const response = await fetch(`${API_BASE}/api/devices`);
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
      
      const data = await response.json();
      
      // Handle both old format (list) and new format (object with devices)
      let deviceList: Device[];
      if (Array.isArray(data)) {
        deviceList = data;
      } else if (data.devices) {
        deviceList = Object.values(data.devices) as Device[];
      } else if (data.device_list) {
        deviceList = data.device_list;
      } else {
        throw new Error('Unexpected data format from server');
      }
      
      // Sort devices by type and then by device_id
      deviceList.sort((a, b) => {
        if (a.device_type !== b.device_type) {
          return a.device_type.localeCompare(b.device_type);
        }
        return a.device_id.localeCompare(b.device_id);
      });
      
      setDevices(deviceList);
      setFilteredDevices(deviceList);
      setError(null);
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Unknown error occurred';
      setError(errorMessage);
    } finally {
      setLoading(false);
    }
  };

  // Update device configuration
  const updateDeviceConfig = async (deviceId: string, field: string, value: any) => {
    try {
      setSaving(deviceId);
      
      // Prepare the configuration update based on the field
      let configUpdate: any = {};
      
      switch (field) {
        case 'device_label':
          configUpdate = { deviceLabel: value };
          break;
        case 'sacn_universe':
          configUpdate = { sacnUniverse: parseInt(value) };
          break;
        case 'dmx_address':
          configUpdate = { dmxStartAddress: parseInt(value) };
          break;
        case 'sacn_enabled':
          configUpdate = { sacnEnabled: value };
          break;
        case 'brightness':
          configUpdate = { brightness: parseInt(value) };
          break;
        case 'ip_address':
          // IP address changes go through network configuration endpoint
          return handleNetworkConfigChange(deviceId, 'staticIP', value);
        default:
          throw new Error(`Unknown field: ${field}`);
      }

      const response = await fetch(`${API_BASE}/api/config/${deviceId}`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(configUpdate),
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(errorData.error || `HTTP ${response.status}`);
      }

      // Update local state immediately for responsive UI
      setDevices(prevDevices =>
        prevDevices.map(device =>
          device.device_id === deviceId
            ? { ...device, [field]: value }
            : device
        )
      );

      // Refresh device data after a short delay to get updated info
      setTimeout(() => {
        fetchDevices();
      }, 1000);

    } catch (err) {
      setError(`Failed to update ${field}: ${err instanceof Error ? err.message : 'Unknown error'}`);
    } finally {
      setSaving(null);
    }
  };

  // Handle cell editing
  const startEditing = (deviceId: string, field: string, currentValue: any) => {
    setEditingCell({ deviceId, field, value: currentValue });
    setTempValue(String(currentValue || ''));
  };

  const cancelEditing = () => {
    setEditingCell(null);
    setTempValue('');
  };

  const saveEditing = async () => {
    if (!editingCell) return;
    
    let processedValue: any = tempValue;
    
    // Convert values based on field type
    if (editingCell.field === 'sacn_universe' || editingCell.field === 'dmx_address' || editingCell.field === 'brightness') {
      processedValue = parseInt(tempValue);
      if (isNaN(processedValue)) {
        setError('Please enter a valid number');
        return;
      }
      
      // Validation
      if (editingCell.field === 'sacn_universe' && (processedValue < 1 || processedValue > 63999)) {
        setError('SACN Universe must be between 1 and 63999');
        return;
      }
      if (editingCell.field === 'dmx_address' && (processedValue < 1 || processedValue > 512)) {
        setError('DMX Address must be between 1 and 512');
        return;
      }
      if (editingCell.field === 'brightness' && (processedValue < 0 || processedValue > 255)) {
        setError('Brightness must be between 0 and 255');
        return;
      }
    }

    await updateDeviceConfig(editingCell.deviceId, editingCell.field, processedValue);
    setEditingCell(null);
    setTempValue('');
  };

  // Handle bulk SACN configuration
  const handleBulkConfig = async () => {
    try {
      const promises = Array.from(selectedDevices).map(async (deviceId) => {
        const configUpdate = {
          sacnUniverse: parseInt(bulkUniverse),
          dmxStartAddress: parseInt(bulkStartAddress),
        };

        const response = await fetch(`${API_BASE}/api/config/${deviceId}`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(configUpdate),
        });

        if (!response.ok) {
          throw new Error(`Failed to update ${deviceId}`);
        }
      });

      await Promise.all(promises);
      
      setBulkConfigOpen(false);
      setSelectedDevices(new Set());
      fetchDevices();
      
    } catch (err) {
      setError(`Bulk configuration failed: ${err instanceof Error ? err.message : 'Unknown error'}`);
    }
  };

  // Handle device selection
  const handleDeviceSelect = (deviceId: string, checked: boolean) => {
    const newSelected = new Set(selectedDevices);
    if (checked) {
      newSelected.add(deviceId);
    } else {
      newSelected.delete(deviceId);
    }
    setSelectedDevices(newSelected);
  };

  // Handle select all
  const handleSelectAll = (checked: boolean) => {
    if (checked) {
      setSelectedDevices(new Set(filteredDevices.map(d => d.device_id)));
    } else {
      setSelectedDevices(new Set());
    }
  };
  const handleSacnToggle = async (deviceId: string, enabled: boolean) => {
    await updateDeviceConfig(deviceId, 'sacn_enabled', enabled);
  };

  // Handle network configuration changes
  const handleNetworkConfigChange = async (deviceId: string, field: string, value: any) => {
    try {
      setSaving(deviceId);
      setError(null);

      const networkData: any = {};
      networkData[field] = value;

      console.log(`Updating network config for ${deviceId}:`, networkData);

      const response = await fetch(`${API_BASE}/api/config/${deviceId}/network`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(networkData),
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(errorData.error || `HTTP ${response.status}`);
      }

      // Update local state immediately for better UX
      setDevices(prevDevices =>
        prevDevices.map(device =>
          device.device_id === deviceId
            ? { ...device, [field]: value }
            : device
        )
      );

      // Show success message for network changes
      if (field === 'useDHCP' || field === 'ip_address') {
        setError(`Network configuration updated for ${deviceId} - device will restart`);
        setTimeout(() => setError(null), 3000);
      }

      // Refresh device data after restart delay
      setTimeout(() => {
        fetchDevices();
      }, 10000); // Wait longer for device restart

    } catch (err) {
      setError(`Failed to update network config: ${err instanceof Error ? err.message : 'Unknown error'}`);
    } finally {
      setSaving(null);
    }
  };

  // Filter devices by type
  useEffect(() => {
    if (deviceTypeFilter === 'all') {
      setFilteredDevices(devices);
    } else {
      setFilteredDevices(devices.filter(device => device.device_type === deviceTypeFilter));
    }
  }, [devices, deviceTypeFilter]);

  // Get unique device types for filter
  const deviceTypes = Array.from(new Set(devices.map(d => d.device_type))).sort();
  const getDeviceTypeColor = (deviceType: string) => {
    const colors: { [key: string]: 'primary' | 'secondary' | 'success' | 'warning' | 'error' | 'info' } = {
      tricorder: 'primary',
      polyinoculator: 'secondary',
      defragmentor: 'success',
      iv_injector: 'warning',
      iv_blood_bag_station: 'error',
      polyinoculator_cradle: 'info',
    };
    return colors[deviceType] || 'default';
  };

  // Format device type for display
  const formatDeviceType = (deviceType: string) => {
    return deviceType.split('_').map(word => 
      word.charAt(0).toUpperCase() + word.slice(1)
    ).join(' ');
  };

  // Initial load
  useEffect(() => {
    fetchDevices();
    
    // Auto-refresh every 30 seconds
    const interval = setInterval(fetchDevices, 30000);
    return () => clearInterval(interval);
  }, []);

  if (loading) {
    return (
      <Box display="flex" justifyContent="center" alignItems="center" minHeight="400px">
        <CircularProgress />
        <Typography variant="body1" sx={{ ml: 2 }}>
          Loading devices...
        </Typography>
      </Box>
    );
  }

  if (error) {
    return (
      <Alert severity="error" sx={{ mb: 2 }}>
        {error}
      </Alert>
    );
  }

  const renderEditableCell = (device: Device, field: string, value: any, isNumeric = false) => {
    const isEditing = editingCell?.deviceId === device.device_id && editingCell?.field === field;
    const isSaving = saving === device.device_id;

    if (isEditing) {
      return (
        <Box display="flex" alignItems="center" gap={1}>
          <TextField
            value={tempValue}
            onChange={(e) => setTempValue(e.target.value)}
            size="small"
            variant="outlined"
            type={isNumeric ? 'number' : 'text'}
            inputProps={isNumeric ? { min: 1 } : {}}
            sx={{ minWidth: 120 }}
            autoFocus
            onKeyPress={(e) => {
              if (e.key === 'Enter') {
                saveEditing();
              } else if (e.key === 'Escape') {
                cancelEditing();
              }
            }}
          />
          <IconButton size="small" onClick={saveEditing} color="primary">
            <CheckIcon />
          </IconButton>
          <IconButton size="small" onClick={cancelEditing} color="secondary">
            <CancelIcon />
          </IconButton>
        </Box>
      );
    }

    return (
      <Box display="flex" alignItems="center" gap={1}>
        <Typography variant="body2" sx={{ minWidth: 80 }}>
          {value || '-'}
        </Typography>
        <IconButton 
          size="small" 
          onClick={() => startEditing(device.device_id, field, value)}
          disabled={isSaving}
        >
          {isSaving ? <CircularProgress size={16} /> : <EditIcon fontSize="small" />}
        </IconButton>
      </Box>
    );
  };

  return (
    <Box>
      <Stack direction="row" justifyContent="space-between" alignItems="center" sx={{ mb: 2 }}>
        <Box>
          <Typography variant="h6" gutterBottom>
            Device Configuration Table
          </Typography>
          <Typography variant="body2" color="text.secondary">
            Click the edit icon next to any value to modify device settings. Changes are sent directly to each device.
          </Typography>
        </Box>
        <Stack direction="row" spacing={2} alignItems="center">
          <FormControl size="small" sx={{ minWidth: 150 }}>
            <InputLabel>Filter by Type</InputLabel>
            <Select
              value={deviceTypeFilter}
              onChange={(e) => setDeviceTypeFilter(e.target.value)}
              label="Filter by Type"
            >
              <MenuItem value="all">All Types</MenuItem>
              {deviceTypes.map(type => (
                <MenuItem key={type} value={type}>
                  {formatDeviceType(type)}
                </MenuItem>
              ))}
            </Select>
          </FormControl>
          <Button
            variant="outlined"
            onClick={fetchDevices}
            disabled={loading}
            startIcon={loading ? <CircularProgress size={16} /> : <RefreshIcon />}
          >
            {loading ? 'Refreshing...' : 'Refresh'}
          </Button>
          <Button
            variant="contained"
            onClick={() => setBulkConfigOpen(true)}
            disabled={selectedDevices.size === 0}
            startIcon={<SettingsIcon />}
          >
            Bulk Config ({selectedDevices.size})
          </Button>
        </Stack>
      </Stack>

      <Divider sx={{ mb: 2 }} />

      {/* Summary Statistics */}
      <Box sx={{ mb: 3, p: 2, bgcolor: 'background.paper', borderRadius: 1, border: 1, borderColor: 'divider' }}>
        <Grid container spacing={3}>
          <Grid item xs={6} sm={3}>
            <Typography variant="h4" color="primary">
              {filteredDevices.length}
            </Typography>
            <Typography variant="body2" color="text.secondary">
              {deviceTypeFilter === 'all' ? 'Total Devices' : 'Filtered Devices'}
            </Typography>
          </Grid>
          <Grid item xs={6} sm={3}>
            <Typography variant="h4" color="success.main">
              {filteredDevices.filter(d => d.status === 'online').length}
            </Typography>
            <Typography variant="body2" color="text.secondary">
              Online
            </Typography>
          </Grid>
          <Grid item xs={6} sm={3}>
            <Typography variant="h4" color="info.main">
              {new Set(filteredDevices.map(d => d.device_type)).size}
            </Typography>
            <Typography variant="body2" color="text.secondary">
              Device Types
            </Typography>
          </Grid>
          <Grid item xs={6} sm={3}>
            <Typography variant="h4" color="warning.main">
              {filteredDevices.filter(d => d.status !== 'online').length}
            </Typography>
            <Typography variant="body2" color="text.secondary">
              Offline
            </Typography>
          </Grid>
        </Grid>
      </Box>

      <TableContainer component={Paper}>
        <Table size="small" stickyHeader>
          <TableHead>
            <TableRow>
              <TableCell padding="checkbox">
                <Checkbox
                  checked={selectedDevices.size === filteredDevices.length && filteredDevices.length > 0}
                  indeterminate={selectedDevices.size > 0 && selectedDevices.size < filteredDevices.length}
                  onChange={(e) => handleSelectAll(e.target.checked)}
                />
              </TableCell>
              <TableCell>Device ID</TableCell>
              <TableCell>Type</TableCell>
              <TableCell>Status</TableCell>
              <TableCell>Label</TableCell>
              <TableCell>IP Address</TableCell>
              <TableCell>DHCP</TableCell>
              <TableCell>SACN Universe</TableCell>
              <TableCell>DMX Address</TableCell>
              <TableCell>SACN Control</TableCell>
              <TableCell>Brightness</TableCell>
              <TableCell>LEDs</TableCell>
              <TableCell>Uptime</TableCell>
              <TableCell>Firmware</TableCell>
            </TableRow>
          </TableHead>
          <TableBody>
            {filteredDevices.map((device) => (
              <TableRow key={device.device_id} hover>
                <TableCell padding="checkbox">
                  <Checkbox
                    checked={selectedDevices.has(device.device_id)}
                    onChange={(e) => handleDeviceSelect(device.device_id, e.target.checked)}
                  />
                </TableCell>
                <TableCell>
                  <Typography variant="body2" fontFamily="monospace">
                    {device.device_id}
                  </Typography>
                </TableCell>
                
                <TableCell>
                  <Chip
                    label={formatDeviceType(device.device_type)}
                    color={getDeviceTypeColor(device.device_type)}
                    size="small"
                  />
                </TableCell>
                
                <TableCell>
                  <Box display="flex" alignItems="center" gap={1}>
                    {device.status === 'online' ? (
                      <WifiIcon color="success" fontSize="small" />
                    ) : (
                      <WifiOffIcon color="error" fontSize="small" />
                    )}
                    <Chip
                      label={device.status}
                      color={device.status === 'online' ? 'success' : 'error'}
                      size="small"
                      variant="outlined"
                    />
                  </Box>
                </TableCell>
                
                <TableCell>
                  {renderEditableCell(device, 'device_label', device.device_label)}
                </TableCell>
                
                <TableCell>
                  {renderEditableCell(device, 'ip_address', device.ip_address)}
                </TableCell>
                
                <TableCell>
                  <FormControlLabel
                    control={
                      <Switch
                        checked={device.useDHCP !== false} // Default to true if undefined
                        onChange={(e) => handleNetworkConfigChange(device.device_id, 'useDHCP', e.target.checked)}
                        size="small"
                        disabled={saving === device.device_id}
                      />
                    }
                    label={device.useDHCP !== false ? 'DHCP' : 'Static'}
                    sx={{ margin: 0 }}
                  />
                </TableCell>
                
                <TableCell>
                  {renderEditableCell(device, 'sacn_universe', device.sacn_universe || 1, true)}
                </TableCell>
                
                <TableCell>
                  {renderEditableCell(device, 'dmx_address', device.dmx_address || 1, true)}
                </TableCell>
                
                <TableCell>
                  <FormControlLabel
                    control={
                      <Switch
                        checked={device.sacn_enabled || false}
                        onChange={(e) => handleSacnToggle(device.device_id, e.target.checked)}
                        size="small"
                        disabled={saving === device.device_id}
                      />
                    }
                    label={device.sacn_enabled ? 'Enabled' : 'Disabled'}
                    sx={{ margin: 0 }}
                  />
                </TableCell>
                
                <TableCell>
                  {device.brightness !== undefined 
                    ? renderEditableCell(device, 'brightness', device.brightness, true)
                    : '-'
                  }
                </TableCell>
                
                <TableCell>
                  <Typography variant="body2" color="text.secondary">
                    {device.num_leds || '-'}
                  </Typography>
                </TableCell>
                
                <TableCell>
                  <Typography variant="body2" color="text.secondary">
                    {device.uptime ? `${Math.floor(device.uptime / 3600)}h ${Math.floor((device.uptime % 3600) / 60)}m` : '-'}
                  </Typography>
                </TableCell>
                
                <TableCell>
                  <Typography variant="caption" color="text.secondary">
                    {device.firmware_version || 'Unknown'}
                  </Typography>
                </TableCell>
              </TableRow>
            ))}
          </TableBody>
        </Table>
      </TableContainer>

      {filteredDevices.length === 0 && !loading && (
        <Box textAlign="center" py={4}>
          <Typography variant="body1" color="text.secondary">
            {deviceTypeFilter === 'all' 
              ? 'No devices found. Make sure devices are powered on and connected to the network.'
              : `No ${formatDeviceType(deviceTypeFilter)} devices found.`
            }
          </Typography>
        </Box>
      )}
      
      {/* Bulk Configuration Dialog */}
      <Dialog open={bulkConfigOpen} onClose={() => setBulkConfigOpen(false)} maxWidth="sm" fullWidth>
        <DialogTitle>
          Bulk SACN Configuration
        </DialogTitle>
        <DialogContent>
          <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
            Configure SACN settings for {selectedDevices.size} selected device(s).
          </Typography>
          
          <Stack spacing={2} sx={{ mt: 2 }}>
            <TextField
              label="SACN Universe"
              type="number"
              value={bulkUniverse}
              onChange={(e) => setBulkUniverse(e.target.value)}
              inputProps={{ min: 1, max: 63999 }}
              fullWidth
            />
            <TextField
              label="DMX Start Address"
              type="number"
              value={bulkStartAddress}
              onChange={(e) => setBulkStartAddress(e.target.value)}
              inputProps={{ min: 1, max: 512 }}
              fullWidth
            />
          </Stack>
          
          <Typography variant="caption" color="text.secondary" sx={{ mt: 2, display: 'block' }}>
            Selected devices: {Array.from(selectedDevices).join(', ')}
          </Typography>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setBulkConfigOpen(false)}>Cancel</Button>
          <Button 
            onClick={handleBulkConfig} 
            variant="contained"
            disabled={!bulkUniverse || !bulkStartAddress}
          >
            Apply to All Selected
          </Button>
        </DialogActions>
      </Dialog>
    </Box>
  );
};

export default DeviceTableView;
