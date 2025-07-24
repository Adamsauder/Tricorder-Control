import React, { useState, useEffect } from 'react';
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Box,
  Typography,
  LinearProgress,
  Alert,
  Stepper,
  Step,
  StepLabel,
  Card,
  CardContent,
  List,
  ListItem,
  ListItemText,
  ListItemSecondaryAction,
  IconButton,
  Input,
  Chip,
  Divider,
  Grid
} from '@mui/material';
import {
  CloudUpload as UploadIcon,
  Download as DownloadIcon,
  Delete as DeleteIcon,
  Update as UpdateIcon,
  CheckCircle as CheckIcon,
  Error as ErrorIcon,
  Warning as WarningIcon,
  Refresh as RefreshIcon
} from '@mui/icons-material';

interface Device {
  device_id: string;
  ip_address: string;
  firmware_version: string;
  [key: string]: any;
}

interface FirmwareFile {
  filename: string;
  size: number;
  modified: string;
  path: string;
}

interface FirmwareUpdateModalProps {
  open: boolean;
  onClose: () => void;
  device: Device | null; // Allow null for global firmware management
}

const steps = [
  'Select Firmware',
  'Verify Compatibility', 
  'Upload & Install',
  'Complete'
];

const FirmwareUpdateModal: React.FC<FirmwareUpdateModalProps> = ({
  open,
  onClose,
  device
}) => {
  const [activeStep, setActiveStep] = useState(0);
  const [firmwareFiles, setFirmwareFiles] = useState<FirmwareFile[]>([]);
  const [selectedFile, setSelectedFile] = useState<FirmwareFile | null>(null);
  const [uploading, setUploading] = useState(false);
  const [updating, setUpdating] = useState(false);
  const [uploadProgress, setUploadProgress] = useState(0);
  const [updateProgress, setUpdateProgress] = useState(0);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);
  const [otaStatus, setOtaStatus] = useState<any>(null);

  useEffect(() => {
    if (open) {
      loadFirmwareFiles();
      if (device) {
        checkOTAStatus();
      }
    }
  }, [open, device?.device_id]);

  const loadFirmwareFiles = async () => {
    try {
      const response = await fetch('/api/firmware/list');
      const data = await response.json();
      setFirmwareFiles(data.firmware_files || []);
    } catch (err) {
      setError('Failed to load firmware files');
    }
  };

  const checkOTAStatus = async () => {
    if (!device) return;
    
    try {
      const response = await fetch(`/api/devices/${device.device_id}/ota_status`);
      const data = await response.json();
      setOtaStatus(data);
    } catch (err) {
      console.warn('Could not check OTA status:', err);
    }
  };

  const handleFileUpload = async (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (!file) return;

    if (!file.name.endsWith('.bin')) {
      setError('Please select a .bin firmware file');
      return;
    }

    setUploading(true);
    setError(null);

    const formData = new FormData();
    formData.append('firmware', file);

    try {
      const response = await fetch('/api/firmware/upload', {
        method: 'POST',
        body: formData,
      });

      if (response.ok) {
        const result = await response.json();
        setSuccess(`Firmware uploaded successfully: ${result.filename}`);
        loadFirmwareFiles(); // Refresh the list
      } else {
        const error = await response.json();
        setError(error.error || 'Upload failed');
      }
    } catch (err) {
      setError('Upload failed: ' + (err as Error).message);
    } finally {
      setUploading(false);
    }
  };

  const handleFirmwareUpdate = async () => {
    if (!selectedFile) {
      setError('Please select a firmware file');
      return;
    }

    if (!device) {
      setError('No device selected for update');
      return;
    }

    setUpdating(true);
    setError(null);
    setActiveStep(2);

    try {
      const response = await fetch(`/api/devices/${device.device_id}/firmware/update`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          firmware_file: selectedFile.filename
        })
      });

      if (response.ok) {
        const result = await response.json();
        setSuccess('Firmware update completed successfully!');
        setActiveStep(3);
        
        // The device will restart, so we'll lose connection temporarily
        setTimeout(() => {
          setSuccess(result.message + ' Device is restarting...');
        }, 1000);
      } else {
        const error = await response.json();
        setError(error.error || 'Firmware update failed');
        setActiveStep(1); // Go back to verification step
      }
    } catch (err) {
      setError('Firmware update failed: ' + (err as Error).message);
      setActiveStep(1);
    } finally {
      setUpdating(false);
    }
  };

  const handleDeleteFirmware = async (filename: string) => {
    // This would require implementing a delete endpoint
    console.log('Delete firmware:', filename);
  };

  const formatFileSize = (bytes: number) => {
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    if (bytes === 0) return '0 Bytes';
    const i = Math.floor(Math.log(bytes) / Math.log(1024));
    return Math.round(bytes / Math.pow(1024, i) * 100) / 100 + ' ' + sizes[i];
  };

  const handleClose = () => {
    setActiveStep(0);
    setSelectedFile(null);
    setError(null);
    setSuccess(null);
    setUploading(false);
    setUpdating(false);
    onClose();
  };

  const renderStepContent = () => {
    switch (activeStep) {
      case 0: // Select Firmware
        return (
          <Box>
            <Typography variant="h6" gutterBottom>
              Available Firmware Files
            </Typography>
            
            {/* Upload new firmware */}
            <Card sx={{ mb: 3, p: 2 }}>
              <Box display="flex" alignItems="center" gap={2}>
                <Input
                  type="file"
                  inputProps={{ accept: '.bin' }}
                  onChange={handleFileUpload}
                  disabled={uploading}
                  sx={{ display: 'none' }}
                  id="firmware-upload"
                />
                <label htmlFor="firmware-upload">
                  <Button
                    component="span"
                    variant="outlined"
                    startIcon={<UploadIcon />}
                    disabled={uploading}
                  >
                    Upload New Firmware
                  </Button>
                </label>
                {uploading && (
                  <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                    <LinearProgress sx={{ width: 100 }} />
                    <Typography variant="body2">Uploading...</Typography>
                  </Box>
                )}
              </Box>
            </Card>

            {/* Firmware files list */}
            <List>
              {firmwareFiles.map((file) => (
                <ListItem
                  key={file.filename}
                  button
                  selected={selectedFile?.filename === file.filename}
                  onClick={() => setSelectedFile(file)}
                  sx={{
                    border: 1,
                    borderColor: selectedFile?.filename === file.filename ? 'primary.main' : 'divider',
                    borderRadius: 1,
                    mb: 1
                  }}
                >
                  <ListItemText
                    primary={file.filename}
                    secondary={
                      <Box>
                        <Typography variant="body2" color="text.secondary">
                          Size: {formatFileSize(file.size)}
                        </Typography>
                        <Typography variant="body2" color="text.secondary">
                          Modified: {new Date(file.modified).toLocaleString()}
                        </Typography>
                      </Box>
                    }
                  />
                  <ListItemSecondaryAction>
                    <IconButton
                      edge="end"
                      onClick={(e) => {
                        e.stopPropagation();
                        handleDeleteFirmware(file.filename);
                      }}
                    >
                      <DeleteIcon />
                    </IconButton>
                  </ListItemSecondaryAction>
                </ListItem>
              ))}
            </List>

            {firmwareFiles.length === 0 && (
              <Alert severity="info">
                No firmware files available. Upload a .bin file to begin.
              </Alert>
            )}
          </Box>
        );

      case 1: // Verify Compatibility
        return (
          <Box>
            <Typography variant="h6" gutterBottom>
              Verify Update Compatibility
            </Typography>
            
            <Grid container spacing={3}>
              <Grid item xs={12} md={6}>
                <Card>
                  <CardContent>
                    <Typography variant="subtitle1" gutterBottom>
                      Current Device Info
                    </Typography>
                    {device ? (
                      <Box>
                        <Typography variant="body2">Device ID: {device.device_id}</Typography>
                        <Typography variant="body2">IP Address: {device.ip_address}</Typography>
                        <Typography variant="body2">Current Firmware: {device.firmware_version}</Typography>
                        <Typography variant="body2">
                          OTA Status: 
                          <Chip 
                            size="small" 
                            color={otaStatus?.ota_available ? 'success' : 'error'}
                            label={otaStatus?.ota_available ? 'Available' : 'Not Available'}
                            sx={{ ml: 1 }}
                          />
                        </Typography>
                      </Box>
                    ) : (
                      <Typography variant="body2" color="text.secondary">
                        Global firmware management - no specific device selected
                      </Typography>
                    )}
                  </CardContent>
                </Card>
              </Grid>
              
              <Grid item xs={12} md={6}>
                <Card>
                  <CardContent>
                    <Typography variant="subtitle1" gutterBottom>
                      Selected Firmware
                    </Typography>
                    {selectedFile ? (
                      <Box>
                        <Typography variant="body2">File: {selectedFile.filename}</Typography>
                        <Typography variant="body2">Size: {formatFileSize(selectedFile.size)}</Typography>
                        <Typography variant="body2">Modified: {new Date(selectedFile.modified).toLocaleString()}</Typography>
                      </Box>
                    ) : (
                      <Typography variant="body2" color="text.secondary">
                        No firmware file selected
                      </Typography>
                    )}
                  </CardContent>
                </Card>
              </Grid>
            </Grid>

            <Alert severity="warning" sx={{ mt: 3 }}>
              <Typography variant="body2">
                <strong>Warning:</strong> Firmware updates can potentially brick your device if interrupted. 
                Ensure stable power and network connection before proceeding.
              </Typography>
            </Alert>

            {!otaStatus?.ota_available && (
              <Alert severity="error" sx={{ mt: 2 }}>
                <Typography variant="body2">
                  OTA updates are not available on this device. Check device connection and firmware compatibility.
                </Typography>
              </Alert>
            )}
          </Box>
        );

      case 2: // Upload & Install
        return (
          <Box textAlign="center">
            <Typography variant="h6" gutterBottom>
              Updating Firmware
            </Typography>
            
            <Box sx={{ my: 4 }}>
              <LinearProgress 
                variant={updating ? "indeterminate" : "determinate"} 
                value={updateProgress}
                sx={{ height: 10, borderRadius: 5 }}
              />
              <Typography variant="body2" sx={{ mt: 1 }}>
                {updating ? 'Uploading firmware to device...' : `${updateProgress}%`}
              </Typography>
            </Box>

            <Alert severity="info">
              <Typography variant="body2">
                Do not close this window or disconnect the device during the update process.
                The device will restart automatically when the update is complete.
              </Typography>
            </Alert>
          </Box>
        );

      case 3: // Complete
        return (
          <Box textAlign="center">
            <CheckIcon color="success" sx={{ fontSize: 64, mb: 2 }} />
            <Typography variant="h6" gutterBottom>
              Firmware Update Complete
            </Typography>
            <Typography variant="body1" color="text.secondary">
              The device has been updated successfully and is restarting.
              It may take a few moments to reconnect to the network.
            </Typography>
          </Box>
        );

      default:
        return null;
    }
  };

  return (
    <Dialog 
      open={open} 
      onClose={handleClose}
      maxWidth="md"
      fullWidth
      disableEscapeKeyDown={updating}
    >
      <DialogTitle>
        <Box display="flex" alignItems="center" gap={2}>
          <UpdateIcon />
          {device ? `Firmware Update - ${device.device_id}` : 'Firmware Management'}
        </Box>
      </DialogTitle>

      <DialogContent>
        <Stepper activeStep={activeStep} sx={{ mb: 4 }}>
          {steps.map((label) => (
            <Step key={label}>
              <StepLabel>{label}</StepLabel>
            </Step>
          ))}
        </Stepper>

        {error && (
          <Alert severity="error" sx={{ mb: 3 }} onClose={() => setError(null)}>
            {error}
          </Alert>
        )}

        {success && (
          <Alert severity="success" sx={{ mb: 3 }} onClose={() => setSuccess(null)}>
            {success}
          </Alert>
        )}

        {renderStepContent()}
      </DialogContent>

      <DialogActions sx={{ p: 2 }}>
        <Button onClick={handleClose} disabled={updating}>
          {activeStep === 3 ? 'Close' : 'Cancel'}
        </Button>
        
        {activeStep === 0 && (
          <Button
            onClick={() => setActiveStep(1)}
            variant="contained"
            disabled={!selectedFile}
          >
            Next
          </Button>
        )}
        
        {activeStep === 1 && (
          <>
            <Button onClick={() => setActiveStep(0)}>
              Back
            </Button>
            <Button
              onClick={handleFirmwareUpdate}
              variant="contained"
              disabled={!selectedFile || !otaStatus?.ota_available}
              startIcon={<UpdateIcon />}
            >
              Start Update
            </Button>
          </>
        )}

        {activeStep === 2 && updating && (
          <Button disabled>
            Updating...
          </Button>
        )}
      </DialogActions>
    </Dialog>
  );
};

export default FirmwareUpdateModal;
