import React, { useState, useEffect } from 'react';
import {
  Box,
  Typography,
  Grid,
  Card,
  CardContent,
  CardActions,
  Button,
  TextField,
  Alert,
  Snackbar,
  CircularProgress,
  Select,
  MenuItem,
  FormControl,
  InputLabel,
  Chip,
  Stack,
  List,
  ListItem,
  ListItemText,
  ListItemIcon,
  ListItemSecondaryAction,
  IconButton,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Divider,
  Slider,
} from '@mui/material';
import {
  Image as ImageIcon,
  Upload as UploadIcon,
  Send as SendIcon,
  Folder as FolderIcon,
  Delete as DeleteIcon,
  Download as DownloadIcon,
  Refresh as RefreshIcon,
  Memory as SdCardIcon,
  Brightness6 as BrightnessIcon,
} from '@mui/icons-material';

// API functions
const API_BASE = '';

interface TricorderDevice {
  device_id: string;
  ip_address: string;
  status: string;
  sd_card_initialized: boolean;
  device_type?: string;
  type?: string;
}

interface SDCardFile {
  name: string;
  size: number;
  type: string;
}

interface PlaybookControlProps {
  devices: TricorderDevice[];
}

const PlaybookControl: React.FC<PlaybookControlProps> = ({ devices }) => {
  const [selectedImage, setSelectedImage] = useState<string>('');
  const [selectedDevices, setSelectedDevices] = useState<string[]>([]);
  const [loading, setLoading] = useState(false);
  const [snackbarOpen, setSnackbarOpen] = useState(false);
  const [snackbarMessage, setSnackbarMessage] = useState('');
  const [snackbarSeverity, setSnackbarSeverity] = useState<'success' | 'error'>('success');
  const [availableImages, setAvailableImages] = useState<string[]>([]);
  const [uploadDialogOpen, setUploadDialogOpen] = useState(false);
  const [selectedFile, setSelectedFile] = useState<File | null>(null);
  const [sdFiles, setSdFiles] = useState<{ [deviceId: string]: SDCardFile[] }>({});
  const [brightness, setBrightness] = useState<number>(255); // Default full brightness

  // Filter for tricorder devices only
  const tricorderDevices = devices.filter(device => 
    device.device_type === 'tricorder' || device.type === 'tricorder'
  );

  const onlineDevices = tricorderDevices.filter(device => device.status === 'online');

  useEffect(() => {
    // Select all online devices by default
    setSelectedDevices(onlineDevices.map(device => device.device_id));
  }, [devices]);

  const showSnackbar = (message: string, severity: 'success' | 'error' = 'success') => {
    setSnackbarMessage(message);
    setSnackbarSeverity(severity);
    setSnackbarOpen(true);
  };

  const handleDisplayImage = async () => {
    if (!selectedImage || selectedDevices.length === 0) {
      showSnackbar('Please select an image and at least one device', 'error');
      return;
    }

    setLoading(true);
    try {
      // Send display_image command to each selected device
      const results = await Promise.allSettled(
        selectedDevices.map(async (deviceId) => {
          const response = await fetch(`${API_BASE}/api/device/${deviceId}/command`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              action: 'display_image',
              parameters: { filename: selectedImage },
              commandId: `display_image_${Date.now()}`
            }),
          });
          
          if (!response.ok) {
            throw new Error(`Failed to send command to ${deviceId}`);
          }
          
          return await response.json();
        })
      );

      const successCount = results.filter(result => result.status === 'fulfilled').length;
      const failCount = results.length - successCount;

      if (failCount === 0) {
        showSnackbar(`Image "${selectedImage}" sent to ${successCount} devices successfully`);
      } else {
        showSnackbar(`Image sent to ${successCount} devices, ${failCount} failed`, 'error');
      }
    } catch (error) {
      console.error('Error sending display command:', error);
      showSnackbar('Failed to send image display command', 'error');
    } finally {
      setLoading(false);
    }
  };

  const handlePresetImage = async (filename: string) => {
    if (selectedDevices.length === 0) {
      showSnackbar('Please select at least one device first', 'error');
      return;
    }

    setLoading(true);
    try {
      // Send display_image command to each selected device
      const results = await Promise.allSettled(
        selectedDevices.map(async (deviceId) => {
          const response = await fetch(`${API_BASE}/api/device/${deviceId}/command`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              action: 'display_image',
              parameters: { filename },
              commandId: `preset_${filename}_${Date.now()}`
            }),
          });
          
          if (!response.ok) {
            throw new Error(`Failed to send command to ${deviceId}`);
          }
          
          return await response.json();
        })
      );

      const successCount = results.filter(result => result.status === 'fulfilled').length;
      const failCount = results.length - successCount;

      if (failCount === 0) {
        showSnackbar(`${filename} displayed on ${successCount} devices successfully`);
        // Also update the selected image field for consistency
        setSelectedImage(filename);
      } else {
        showSnackbar(`Image sent to ${successCount} devices, ${failCount} failed`, 'error');
      }
    } catch (error) {
      console.error('Error sending preset image command:', error);
      showSnackbar('Failed to send preset image command', 'error');
    } finally {
      setLoading(false);
    }
  };

  const handleBrightnessChange = async (value: number) => {
    setBrightness(value);
    
    if (selectedDevices.length === 0) {
      showSnackbar('Please select at least one device first', 'error');
      return;
    }

    try {
      // Send brightness command to each selected device
      const promises = selectedDevices.map(async (deviceId) => {
        const response = await fetch(`${API_BASE}/api/device/${deviceId}/command`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            action: 'set_display_brightness',
            parameters: { brightness: value },
            commandId: `brightness_${value}_${Date.now()}`
          }),
        });
        
        if (!response.ok) {
          throw new Error(`Failed to set brightness for ${deviceId}`);
        }
        
        return await response.json();
      });

      await Promise.allSettled(promises);
      // Note: We don't show success/error messages for brightness changes as they happen frequently
    } catch (error) {
      console.error('Error setting brightness:', error);
      // Only show error if there's a real failure, not for every slider move
    }
  };

  const handleRefreshFileLists = async () => {
    if (selectedDevices.length === 0) {
      showSnackbar('Please select at least one device', 'error');
      return;
    }

    setLoading(true);
    try {
      // Send status command to selected devices to get current file information
      const promises = selectedDevices.map(async (deviceId) => {
        const response = await fetch(`${API_BASE}/api/device/${deviceId}/command`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({
            action: 'status',
            commandId: `refresh_files_${Date.now()}`
          }),
        });

        if (!response.ok) {
          throw new Error(`Failed to refresh files for ${deviceId}`);
        }

        return response.json();
      });

      const results = await Promise.allSettled(promises);
      const successCount = results.filter(result => result.status === 'fulfilled').length;
      const failCount = results.length - successCount;

      if (failCount === 0) {
        showSnackbar(`File lists refreshed for ${successCount} devices`);
      } else {
        showSnackbar(`Refreshed ${successCount} devices, ${failCount} failed`, 'error');
      }
    } catch (error) {
      console.error('Error refreshing file lists:', error);
      showSnackbar('Failed to refresh file lists', 'error');
    } finally {
      setLoading(false);
    }
  };

  const handleFileUpload = async () => {
    if (!selectedFile || selectedDevices.length === 0) {
      showSnackbar('Please select a file and at least one device', 'error');
      return;
    }

    setLoading(true);
    try {
      // Create FormData with the file
      const formData = new FormData();
      formData.append('files', selectedFile);

      // Upload to tricorder prop type endpoint
      const response = await fetch(`${API_BASE}/api/props/tricorder/files/upload`, {
        method: 'POST',
        body: formData,
      });

      const result = await response.json();

      if (result.success) {
        showSnackbar(`Successfully uploaded ${selectedFile.name} to ${result.successful_uploads}/${result.total_uploads} targets`, 'success');
        
        // Add to available images list if it's a JPEG
        if (selectedFile.name.toLowerCase().match(/\.(jpg|jpeg)$/)) {
          setAvailableImages(prev => [...prev, selectedFile.name]);
        }
      } else {
        showSnackbar(`Upload failed: ${result.message}`, 'error');
      }
    } catch (error) {
      console.error('Error uploading file:', error);
      showSnackbar('Failed to upload file', 'error');
    } finally {
      setLoading(false);
      setUploadDialogOpen(false);
      setSelectedFile(null);
    }
  };

  const handleDeviceSelection = (deviceId: string) => {
    setSelectedDevices(prev => 
      prev.includes(deviceId) 
        ? prev.filter(id => id !== deviceId)
        : [...prev, deviceId]
    );
  };

  const selectAllDevices = () => {
    setSelectedDevices(onlineDevices.map(device => device.device_id));
  };

  const deselectAllDevices = () => {
    setSelectedDevices([]);
  };

  return (
    <Box>
      <Typography variant="h4" gutterBottom>
        📱 Playbook Control
      </Typography>
      <Typography variant="body1" color="text.secondary" gutterBottom>
        Control JPEG images displayed on tricorder screens and manage SD card files.
      </Typography>

      <Grid container spacing={3} sx={{ mt: 2 }}>
        {/* Image Display Control */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Box sx={{ display: 'flex', alignItems: 'center', mb: 2 }}>
                <ImageIcon sx={{ mr: 1 }} />
                <Typography variant="h6">Image Display Control</Typography>
              </Box>
              
              <TextField
                label="Image Filename"
                value={selectedImage}
                onChange={(e) => setSelectedImage(e.target.value)}
                fullWidth
                placeholder="e.g., tricorder_scan.jpg"
                helperText="Enter the filename of the JPEG image on the SD card"
                sx={{ mb: 3 }}
              />

              <Typography variant="subtitle2" gutterBottom>
                Target Devices ({selectedDevices.length} selected)
              </Typography>
              
              <Stack direction="row" spacing={1} sx={{ mb: 2 }}>
                <Button size="small" onClick={selectAllDevices}>
                  Select All
                </Button>
                <Button size="small" onClick={deselectAllDevices}>
                  Clear All
                </Button>
              </Stack>

              <Box sx={{ maxHeight: 200, overflow: 'auto' }}>
                {onlineDevices.map((device) => (
                  <Chip
                    key={device.device_id}
                    label={`${device.device_id} (${device.ip_address})`}
                    onClick={() => handleDeviceSelection(device.device_id)}
                    color={selectedDevices.includes(device.device_id) ? 'primary' : 'default'}
                    variant={selectedDevices.includes(device.device_id) ? 'filled' : 'outlined'}
                    sx={{ m: 0.5 }}
                    icon={<SdCardIcon />}
                  />
                ))}
              </Box>

              {onlineDevices.length === 0 && (
                <Alert severity="warning" sx={{ mt: 2 }}>
                  No online tricorder devices found
                </Alert>
              )}
            </CardContent>
            
            <CardActions>
              <Button
                variant="contained"
                startIcon={<SendIcon />}
                onClick={handleDisplayImage}
                disabled={loading || !selectedImage || selectedDevices.length === 0}
              >
                Display Image
              </Button>
            </CardActions>
          </Card>
        </Grid>

        {/* Quick Image Presets */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Box sx={{ display: 'flex', alignItems: 'center', mb: 2 }}>
                <ImageIcon sx={{ mr: 1 }} />
                <Typography variant="h6">Medical Scan Presets</Typography>
              </Box>
              
              <Typography variant="body2" color="text.secondary" gutterBottom>
                Quick access to pre-loaded medical scan images
              </Typography>

              <Grid container spacing={2} sx={{ mt: 1 }}>
                <Grid item xs={6}>
                  <Button
                    variant="outlined"
                    fullWidth
                    onClick={() => handlePresetImage('SFA2_202_211_Med_Tricorder_BODY.jpg')}
                    disabled={loading || selectedDevices.length === 0}
                    sx={{ mb: 1, textAlign: 'left', justifyContent: 'flex-start' }}
                  >
                    🦴 Full Body Scan
                  </Button>
                </Grid>
                <Grid item xs={6}>
                  <Button
                    variant="outlined"
                    fullWidth
                    onClick={() => handlePresetImage('SFA2_202_211_Med_Tricorder_BRAIN.jpg')}
                    disabled={loading || selectedDevices.length === 0}
                    sx={{ mb: 1, textAlign: 'left', justifyContent: 'flex-start' }}
                  >
                    🧠 Brain Scan
                  </Button>
                </Grid>
                <Grid item xs={6}>
                  <Button
                    variant="outlined"
                    fullWidth
                    onClick={() => handlePresetImage('SFA2_202_211_Med_Tricorder_FACE.jpg')}
                    disabled={loading || selectedDevices.length === 0}
                    sx={{ mb: 1, textAlign: 'left', justifyContent: 'flex-start' }}
                  >
                    👤 Face/Head Scan
                  </Button>
                </Grid>
                <Grid item xs={6}>
                  <Button
                    variant="outlined"
                    fullWidth
                    onClick={() => handlePresetImage('SFA2_202_211_Med_Tricorder_TRACTS.jpg')}
                    disabled={loading || selectedDevices.length === 0}
                    sx={{ mb: 1, textAlign: 'left', justifyContent: 'flex-start' }}
                  >
                    🧬 Neural Tracts
                  </Button>
                </Grid>
                <Grid item xs={12}>
                  <Button
                    variant="outlined"
                    fullWidth
                    onClick={() => handlePresetImage('greenscreen.jpg')}
                    disabled={loading || selectedDevices.length === 0}
                    sx={{ mb: 1, textAlign: 'left', justifyContent: 'flex-start' }}
                  >
                    🟢 Green Screen
                  </Button>
                </Grid>
              </Grid>

              <Alert severity="info" sx={{ mt: 2 }}>
                Select devices above, then click any preset to display that medical scan
              </Alert>
            </CardContent>
          </Card>
        </Grid>

        {/* Screen Brightness Control */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Box sx={{ display: 'flex', alignItems: 'center', mb: 2 }}>
                <BrightnessIcon sx={{ mr: 1 }} />
                <Typography variant="h6">Screen Brightness</Typography>
              </Box>
              
              <Typography variant="body2" color="text.secondary" gutterBottom>
                Adjust tricorder screen brightness for optimal viewing
              </Typography>

              <Box sx={{ mt: 3, mb: 2 }}>
                <Typography variant="body2" gutterBottom>
                  Brightness: {Math.round((brightness / 255) * 100)}%
                </Typography>
                <Slider
                  value={brightness}
                  onChange={(_, value) => handleBrightnessChange(value as number)}
                  min={0}
                  max={255}
                  step={5}
                  disabled={loading || selectedDevices.length === 0}
                  sx={{ mt: 1 }}
                  marks={[
                    { value: 0, label: '0%' },
                    { value: 64, label: '25%' },
                    { value: 128, label: '50%' },
                    { value: 192, label: '75%' },
                    { value: 255, label: '100%' }
                  ]}
                />
              </Box>

              <Grid container spacing={1} sx={{ mt: 2 }}>
                <Grid item xs={3}>
                  <Button
                    variant="outlined"
                    size="small"
                    fullWidth
                    onClick={() => handleBrightnessChange(64)}
                    disabled={loading || selectedDevices.length === 0}
                  >
                    Dim
                  </Button>
                </Grid>
                <Grid item xs={3}>
                  <Button
                    variant="outlined"
                    size="small"
                    fullWidth
                    onClick={() => handleBrightnessChange(128)}
                    disabled={loading || selectedDevices.length === 0}
                  >
                    Medium
                  </Button>
                </Grid>
                <Grid item xs={3}>
                  <Button
                    variant="outlined"
                    size="small"
                    fullWidth
                    onClick={() => handleBrightnessChange(192)}
                    disabled={loading || selectedDevices.length === 0}
                  >
                    Bright
                  </Button>
                </Grid>
                <Grid item xs={3}>
                  <Button
                    variant="outlined"
                    size="small"
                    fullWidth
                    onClick={() => handleBrightnessChange(255)}
                    disabled={loading || selectedDevices.length === 0}
                  >
                    Max
                  </Button>
                </Grid>
              </Grid>

              <Alert severity="info" sx={{ mt: 2 }}>
                Select devices first, then use the slider or preset buttons to adjust brightness
              </Alert>
            </CardContent>
          </Card>
        </Grid>

        {/* SD Card File Management */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Box sx={{ display: 'flex', alignItems: 'center', mb: 2 }}>
                <FolderIcon sx={{ mr: 1 }} />
                <Typography variant="h6">SD Card File Management</Typography>
              </Box>
              
              <Typography variant="body2" color="text.secondary" gutterBottom>
                Upload and manage files on tricorder SD cards
              </Typography>

              <Stack spacing={2}>
                <Button
                  variant="outlined"
                  startIcon={<UploadIcon />}
                  onClick={() => setUploadDialogOpen(true)}
                  disabled={loading || selectedDevices.length === 0}
                  fullWidth
                >
                  Upload File to Selected Devices
                </Button>

                <Button
                  variant="outlined"
                  startIcon={<RefreshIcon />}
                  onClick={handleRefreshFileLists}
                  disabled={loading || selectedDevices.length === 0}
                  fullWidth
                >
                  Refresh File Lists
                </Button>
              </Stack>

              <Alert severity="info" sx={{ mt: 2 }}>
                Select devices from the list below, then click "Upload File" to manage SD card content.
              </Alert>
            </CardContent>
          </Card>
        </Grid>

        {/* Device Status Overview */}
        <Grid item xs={12}>
          <Card>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                Tricorder Device Status
              </Typography>
              
              <Grid container spacing={2}>
                <Grid item xs={6} md={3}>
                  <Box textAlign="center">
                    <Typography variant="h4" color="primary">
                      {tricorderDevices.length}
                    </Typography>
                    <Typography variant="body2" color="text.secondary">
                      Total Tricorders
                    </Typography>
                  </Box>
                </Grid>
                <Grid item xs={6} md={3}>
                  <Box textAlign="center">
                    <Typography variant="h4" color="success.main">
                      {onlineDevices.length}
                    </Typography>
                    <Typography variant="body2" color="text.secondary">
                      Online
                    </Typography>
                  </Box>
                </Grid>
                <Grid item xs={6} md={3}>
                  <Box textAlign="center">
                    <Typography variant="h4" color="info.main">
                      {onlineDevices.filter(d => d.sd_card_initialized).length}
                    </Typography>
                    <Typography variant="body2" color="text.secondary">
                      SD Cards Ready
                    </Typography>
                  </Box>
                </Grid>
                <Grid item xs={6} md={3}>
                  <Box textAlign="center">
                    <Typography variant="h4" color="warning.main">
                      {selectedDevices.length}
                    </Typography>
                    <Typography variant="body2" color="text.secondary">
                      Selected
                    </Typography>
                  </Box>
                </Grid>
              </Grid>
            </CardContent>
          </Card>
        </Grid>
      </Grid>

      {/* File Upload Dialog */}
      <Dialog open={uploadDialogOpen} onClose={() => setUploadDialogOpen(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Upload File to SD Cards</DialogTitle>
        <DialogContent>
          <Typography variant="body2" color="text.secondary" gutterBottom>
            Select a JPEG image to upload to the selected tricorder devices
          </Typography>
          
          <input
            type="file"
            accept=".jpg,.jpeg,.png"
            onChange={(e) => setSelectedFile(e.target.files?.[0] || null)}
            style={{ marginTop: 16 }}
          />
          
          {selectedFile && (
            <Alert severity="info" sx={{ mt: 2 }}>
              Selected: {selectedFile.name} ({(selectedFile.size / 1024).toFixed(1)} KB)
            </Alert>
          )}
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setUploadDialogOpen(false)}>Cancel</Button>
          <Button 
            onClick={handleFileUpload} 
            variant="contained" 
            disabled={!selectedFile}
          >
            Upload
          </Button>
        </DialogActions>
      </Dialog>

      {/* Snackbar Notifications */}
      <Snackbar
        open={snackbarOpen}
        autoHideDuration={6000}
        onClose={() => setSnackbarOpen(false)}
      >
        <Alert severity={snackbarSeverity} onClose={() => setSnackbarOpen(false)}>
          {snackbarMessage}
        </Alert>
      </Snackbar>

      {/* Loading Overlay */}
      {loading && (
        <Box
          sx={{
            position: 'fixed',
            top: 0,
            left: 0,
            right: 0,
            bottom: 0,
            bgcolor: 'rgba(0, 0, 0, 0.5)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            zIndex: 2000,
          }}
        >
          <Box
            sx={{
              bgcolor: 'background.paper',
              borderRadius: 2,
              p: 3,
              display: 'flex',
              alignItems: 'center',
              gap: 2,
            }}
          >
            <CircularProgress />
            <Typography>Sending commands...</Typography>
          </Box>
        </Box>
      )}
    </Box>
  );
};

export default PlaybookControl;
