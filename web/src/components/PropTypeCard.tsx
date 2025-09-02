import React, { useState } from 'react';
import {
  Card,
  CardContent,
  CardActions,
  Typography,
  Chip,
  Button,
  Box,
  LinearProgress,
  TextField,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  List,
  ListItem,
  ListItemText,
  ListItemIcon,
  IconButton,
  Collapse,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
} from '@mui/material';
import {
  ExpandMore as ExpandMoreIcon,
  ExpandLess as ExpandLessIcon,
  Settings as SettingsIcon,
  Update as UpdateIcon,
  Wifi as WifiIcon,
  WifiOff as WifiOffIcon,
  PlayArrow as PlayIcon,
  Stop as StopIcon,
  Palette as PaletteIcon,
  Save as SaveIcon,
} from '@mui/icons-material';
import { TricorderDevice, tricorderAPI, FirmwareInfo } from '../services/tricorderAPI';

interface PropTypeCardProps {
  propType: string;
  devices: TricorderDevice[];
  onSacnAddressChange: (propType: string, universe: number, address: number) => Promise<void>;
  onFirmwareUpdate: (propType: string, file: File) => Promise<void>;
  onBulkCommand: (propType: string, action: string, parameters?: any) => Promise<void>;
  onSaveCurrentAsDefault: (propType: string) => Promise<void>;
}

const PropTypeCard: React.FC<PropTypeCardProps> = ({
  propType,
  devices,
  onSacnAddressChange,
  onFirmwareUpdate,
  onBulkCommand,
  onSaveCurrentAsDefault,
}) => {
  const [expanded, setExpanded] = useState(false);
  const [sacnDialogOpen, setSacnDialogOpen] = useState(false);
  const [firmwareDialogOpen, setFirmwareDialogOpen] = useState(false);
  const [universe, setUniverse] = useState(221);
  const [address, setAddress] = useState(1);
  const [selectedFile, setSelectedFile] = useState<File | null>(null);
  const [availableFirmware, setAvailableFirmware] = useState<FirmwareInfo[]>([]);
  const [selectedFirmwareType, setSelectedFirmwareType] = useState<'server' | 'custom'>('server');
  const [selectedServerFirmware, setSelectedServerFirmware] = useState<string>('');
  const [loadingFirmware, setLoadingFirmware] = useState(false);

  const onlineDevices = devices.filter(d => d.status === 'online');
  const offlineDevices = devices.filter(d => d.status !== 'online');

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'online': return 'success';
      case 'offline': return 'warning';
      case 'error': return 'error';
      default: return 'default';
    }
  };

  const getDeviceTypeIcon = (type: string) => {
    switch (type) {
      case 'tricorder': return '📺';
      case 'polyinoculator': return '💡';
      case 'defragmentor': return '🔧';
      case 'iv_injector': return '💉';
      case 'iv_blood_bag_station': return '🩸';
      case 'polyinoculator_cradle': return '🔌';
      default: return '🤖';
    }
  };

  const getDeviceTypeDisplayName = (type: string) => {
    switch (type) {
      case 'tricorder': return 'Tricorders';
      case 'polyinoculator': return 'Polyinoculators';
      case 'defragmentor': return 'Defragmentors';
      case 'iv_injector': return 'IV Injectors';
      case 'iv_blood_bag_station': return 'IV Blood Bag Stations';
      case 'polyinoculator_cradle': return 'Polyinoculator Cradles';
      default: return type.charAt(0).toUpperCase() + type.slice(1);
    }
  };

  const handleSacnSubmit = async () => {
    try {
      await onSacnAddressChange(propType, universe, address);
      setSacnDialogOpen(false);
    } catch (error) {
      console.error('SACN address change failed:', error);
    }
  };

  const handleFileChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (file) {
      setSelectedFile(file);
    }
  };

  const fetchAvailableFirmware = async () => {
    setLoadingFirmware(true);
    try {
      const response = await tricorderAPI.getFirmwareList();
      setAvailableFirmware(response.firmware);
      
      // Pre-select the firmware for the current prop type if available
      const matchingFirmware = response.firmware.find(fw => fw.device_type === propType);
      if (matchingFirmware) {
        setSelectedServerFirmware(matchingFirmware.device_type);
        setSelectedFirmwareType('server');
      } else {
        setSelectedFirmwareType('custom');
      }
    } catch (error) {
      console.error('Failed to fetch firmware list:', error);
      setSelectedFirmwareType('custom'); // Fallback to custom upload
    } finally {
      setLoadingFirmware(false);
    }
  };

  const openFirmwareDialog = () => {
    setFirmwareDialogOpen(true);
    fetchAvailableFirmware();
  };

  const handleFirmwareSubmit = async () => {
    let fileToUpload: File | null = null;

    if (selectedFirmwareType === 'server' && selectedServerFirmware) {
      // Download firmware from server and convert to File
      try {
        const blob = await tricorderAPI.downloadFirmware(selectedServerFirmware);
        const firmwareInfo = availableFirmware.find(fw => fw.device_type === selectedServerFirmware);
        const filename = firmwareInfo?.filename || `${selectedServerFirmware}_firmware.bin`;
        fileToUpload = new File([blob], filename, { type: 'application/octet-stream' });
      } catch (error) {
        console.error('Failed to download server firmware:', error);
        return;
      }
    } else if (selectedFirmwareType === 'custom' && selectedFile) {
      fileToUpload = selectedFile;
    }

    if (!fileToUpload) return;
    
    try {
      await onFirmwareUpdate(propType, fileToUpload);
      setFirmwareDialogOpen(false);
      setSelectedFile(null);
      setSelectedServerFirmware('');
    } catch (error) {
      console.error('Firmware update failed:', error);
    }
  };

  return (
    <>
      <Card sx={{ mb: 2, border: expanded ? '2px solid #2196f3' : '1px solid #ddd' }}>
        <CardContent>
          <Box display="flex" justifyContent="space-between" alignItems="center" mb={2}>
            <Box display="flex" alignItems="center" gap={2}>
              <Typography variant="h4" component="span">
                {getDeviceTypeIcon(propType)}
              </Typography>
              <Box>
                <Typography variant="h6" component="div">
                  {getDeviceTypeDisplayName(propType)}
                </Typography>
                <Typography variant="body2" color="text.secondary">
                  {onlineDevices.length} online, {devices.length} total
                </Typography>
              </Box>
            </Box>
            
            <Box display="flex" alignItems="center" gap={1}>
              <Chip
                label={`${onlineDevices.length}/${devices.length}`}
                color={onlineDevices.length > 0 ? 'success' : 'warning'}
                size="small"
              />
              <IconButton
                onClick={() => setExpanded(!expanded)}
                sx={{ transform: expanded ? 'rotate(180deg)' : 'rotate(0deg)', transition: 'transform 0.3s' }}
              >
                <ExpandMoreIcon />
              </IconButton>
            </Box>
          </Box>

          {/* Progress bar showing online percentage */}
          <LinearProgress
            variant="determinate"
            value={devices.length > 0 ? (onlineDevices.length / devices.length) * 100 : 0}
            color={onlineDevices.length === devices.length ? 'success' : 'primary'}
            sx={{ mb: 2, height: 6, borderRadius: 3 }}
          />

          {/* Bulk Controls */}
          <Box display="flex" gap={1} flexWrap="wrap">
            <Button
              size="small"
              variant="contained"
              color="primary"
              startIcon={<SettingsIcon />}
              onClick={() => setSacnDialogOpen(true)}
              disabled={onlineDevices.length === 0}
            >
              Set SACN Address
            </Button>
            <Button
              size="small"
              variant="contained"
              color="info"
              startIcon={<UpdateIcon />}
              onClick={openFirmwareDialog}
              disabled={onlineDevices.length === 0}
            >
              Update Firmware
            </Button>
            <Button
              size="small"
              variant="outlined"
              startIcon={<PlayIcon />}
              onClick={() => onBulkCommand(propType, 'ping')}
              disabled={onlineDevices.length === 0}
            >
              Ping All
            </Button>
            <Button
              size="small"
              variant="outlined"
              color="success"
              startIcon={<SaveIcon />}
              onClick={() => onSaveCurrentAsDefault(propType)}
              disabled={onlineDevices.length === 0}
            >
              Save Current as Default
            </Button>
          </Box>
        </CardContent>

        {/* Expandable device list */}
        <Collapse in={expanded} timeout="auto" unmountOnExit>
          <CardContent sx={{ pt: 0 }}>
            <Typography variant="subtitle2" color="text.secondary" gutterBottom>
              Device Details
            </Typography>
            <List dense>
              {devices.map((device) => (
                <ListItem key={device.device_id} divider>
                  <ListItemIcon>
                    {device.status === 'online' ? 
                      <WifiIcon color="success" /> : 
                      <WifiOffIcon color="disabled" />
                    }
                  </ListItemIcon>
                  <ListItemText
                    primary={device.device_id}
                    secondary={
                      <Box>
                        <Typography variant="caption" display="block">
                          IP: {device.ip_address} | FW: {device.firmware_version}
                        </Typography>
                        {device.last_seen && (
                          <Typography variant="caption" color="text.disabled">
                            Last seen: {new Date(device.last_seen).toLocaleTimeString()}
                          </Typography>
                        )}
                      </Box>
                    }
                  />
                  <Chip
                    label={device.status?.toUpperCase()}
                    color={getStatusColor(device.status) as any}
                    size="small"
                  />
                </ListItem>
              ))}
            </List>
          </CardContent>
        </Collapse>
      </Card>

      {/* SACN Address Dialog */}
      <Dialog open={sacnDialogOpen} onClose={() => setSacnDialogOpen(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Set SACN Address for {getDeviceTypeDisplayName(propType)}</DialogTitle>
        <DialogContent>
          <Typography variant="body2" color="text.secondary" gutterBottom>
            This will set the SACN universe and starting address for all {onlineDevices.length} online {propType} devices.
          </Typography>
          <Box display="flex" gap={2} mt={2}>
            <TextField
              label="Universe"
              type="number"
              value={universe}
              onChange={(e) => setUniverse(parseInt(e.target.value))}
              inputProps={{ min: 1, max: 63999 }}
              fullWidth
            />
            <TextField
              label="Start Address"
              type="number"
              value={address}
              onChange={(e) => setAddress(parseInt(e.target.value))}
              inputProps={{ min: 1, max: 512 }}
              fullWidth
            />
          </Box>
          <Typography variant="caption" color="text.secondary" sx={{ mt: 1, display: 'block' }}>
            Example: Universe 221, Address 1 sets devices to 221.1, 221.2, etc.
          </Typography>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setSacnDialogOpen(false)}>Cancel</Button>
          <Button onClick={handleSacnSubmit} variant="contained">Apply to All</Button>
        </DialogActions>
      </Dialog>

      {/* Firmware Update Dialog */}
      <Dialog open={firmwareDialogOpen} onClose={() => setFirmwareDialogOpen(false)} maxWidth="md" fullWidth>
        <DialogTitle>Update Firmware for {getDeviceTypeDisplayName(propType)}</DialogTitle>
        <DialogContent>
          <Typography variant="body2" color="text.secondary" gutterBottom>
            Select firmware to update all {onlineDevices.length} online {propType} devices.
          </Typography>
          
          <Box mt={3}>
            <FormControl component="fieldset">
              <Typography variant="subtitle2" gutterBottom>
                Firmware Source
              </Typography>
              
              {/* Server Firmware Selection */}
              <Box mb={2}>
                <Button
                  variant={selectedFirmwareType === 'server' ? 'contained' : 'outlined'}
                  onClick={() => setSelectedFirmwareType('server')}
                  disabled={loadingFirmware || availableFirmware.length === 0}
                  sx={{ mr: 2, mb: 1 }}
                >
                  📦 Server Firmware
                </Button>
                <Button
                  variant={selectedFirmwareType === 'custom' ? 'contained' : 'outlined'}
                  onClick={() => setSelectedFirmwareType('custom')}
                  sx={{ mb: 1 }}
                >
                  📁 Custom File
                </Button>
              </Box>

              {/* Server Firmware Options */}
              {selectedFirmwareType === 'server' && (
                <Box mb={2}>
                  {loadingFirmware ? (
                    <Typography variant="body2" color="text.secondary">
                      Loading firmware list...
                    </Typography>
                  ) : availableFirmware.length > 0 ? (
                    <FormControl fullWidth>
                      <InputLabel>Available Server Firmware</InputLabel>
                      <Select
                        value={selectedServerFirmware}
                        onChange={(e) => setSelectedServerFirmware(e.target.value)}
                        label="Available Server Firmware"
                      >
                        {availableFirmware.map((fw) => (
                          <MenuItem key={fw.device_type} value={fw.device_type}>
                            <Box>
                              <Typography variant="body2">
                                {fw.device_type} - {fw.filename}
                              </Typography>
                              <Typography variant="caption" color="text.secondary">
                                {(fw.size / 1024 / 1024).toFixed(2)} MB • Modified: {new Date(fw.modified).toLocaleDateString()}
                              </Typography>
                            </Box>
                          </MenuItem>
                        ))}
                      </Select>
                    </FormControl>
                  ) : (
                    <Typography variant="body2" color="warning.main">
                      No server firmware available. Please use custom file upload.
                    </Typography>
                  )}
                </Box>
              )}

              {/* Custom File Upload */}
              {selectedFirmwareType === 'custom' && (
                <Box mb={2}>
                  <Typography variant="subtitle2" gutterBottom>
                    Upload Custom Firmware File
                  </Typography>
                  <input
                    type="file"
                    accept=".bin"
                    onChange={handleFileChange}
                    style={{ width: '100%', padding: '10px', border: '1px solid #ccc', borderRadius: '4px' }}
                  />
                  {selectedFile && (
                    <Typography variant="caption" color="text.secondary" sx={{ mt: 1, display: 'block' }}>
                      Selected: {selectedFile.name} ({(selectedFile.size / 1024 / 1024).toFixed(2)} MB)
                    </Typography>
                  )}
                </Box>
              )}

              {/* Firmware Info Display */}
              {selectedFirmwareType === 'server' && selectedServerFirmware && (
                <Box p={2} bgcolor="background.paper" border={1} borderColor="divider" borderRadius={1}>
                  {(() => {
                    const fw = availableFirmware.find(f => f.device_type === selectedServerFirmware);
                    return fw ? (
                      <Box>
                        <Typography variant="subtitle2" color="primary">
                          📋 Selected Firmware Details
                        </Typography>
                        <Typography variant="body2">
                          <strong>File:</strong> {fw.filename}
                        </Typography>
                        <Typography variant="body2">
                          <strong>Size:</strong> {(fw.size / 1024 / 1024).toFixed(2)} MB
                        </Typography>
                        <Typography variant="body2">
                          <strong>Last Modified:</strong> {new Date(fw.modified).toLocaleString()}
                        </Typography>
                      </Box>
                    ) : null;
                  })()}
                </Box>
              )}
            </FormControl>
          </Box>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setFirmwareDialogOpen(false)}>Cancel</Button>
          <Button 
            onClick={handleFirmwareSubmit} 
            variant="contained" 
            disabled={
              (selectedFirmwareType === 'server' && !selectedServerFirmware) ||
              (selectedFirmwareType === 'custom' && !selectedFile) ||
              loadingFirmware
            }
          >
            Update All Devices
          </Button>
        </DialogActions>
      </Dialog>
    </>
  );
};

export default PropTypeCard;
