import React, { useState, useEffect } from 'react';
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Box,
  Grid,
  TextField,
  Typography,
  Paper,
  Chip,
  IconButton,
  Tabs,
  Tab,
  Card,
  CardContent,
  Divider,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
  Tooltip,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  Alert,
} from '@mui/material';
import {
  Close as CloseIcon,
  Settings as SettingsIcon,
  Lightbulb as LightbulbIcon,
  LinearScale as LinearScaleIcon,
  Palette as PaletteIcon,
  Save as SaveIcon,
  RestoreFromTrash as ResetIcon,
  GridOn as MatrixIcon,
  Info as InfoIcon,
  Edit as EditIcon,
} from '@mui/icons-material';
import { PROP_PROFILES, PropProfile, LEDElement, getProfileById, getProfileChannelMap } from './PropProfiles';
import ProfileEditor from './ProfileEditor';

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
  prop_profile?: string; // Profile ID
  dmx_start_address?: number; // Starting DMX address
  dmx_universe?: number; // DMX universe
}

interface TricorderSettingsModalProps {
  open: boolean;
  onClose: () => void;
  device: TricorderDevice | null;
  onSave: (deviceId: string, settings: any) => void;
}

const TricorderSettingsModal: React.FC<TricorderSettingsModalProps> = ({
  open,
  onClose,
  device,
  onSave,
}) => {
  const [activeTab, setActiveTab] = useState(0);
  const [selectedProfile, setSelectedProfile] = useState<string>('');
  const [startAddress, setStartAddress] = useState<number>(1);
  const [universe, setUniverse] = useState<number>(1);
  const [currentProfile, setCurrentProfile] = useState<PropProfile | null>(null);
  const [profileEditorOpen, setProfileEditorOpen] = useState(false);
  const [availableProfiles, setAvailableProfiles] = useState(PROP_PROFILES);

  // Initialize settings when device changes
  useEffect(() => {
    if (device) {
      setSelectedProfile(device.prop_profile || 'tricorder_mk1');
      setStartAddress(device.dmx_start_address || 1);
      setUniverse(device.dmx_universe || 1);
    }
  }, [device]);

  // Update current profile when selection changes
  useEffect(() => {
    if (selectedProfile) {
      const profile = availableProfiles[selectedProfile];
      setCurrentProfile(profile || null);
    }
  }, [selectedProfile, availableProfiles]);

  const handleTabChange = (event: React.SyntheticEvent, newValue: number) => {
    setActiveTab(newValue);
  };

  const handleSave = () => {
    if (device) {
      onSave(device.device_id, {
        prop_profile: selectedProfile,
        dmx_start_address: startAddress,
        dmx_universe: universe,
        updated_at: new Date().toISOString(),
      });
      onClose();
    }
  };

  const resetToDefaults = () => {
    if (currentProfile) {
      setStartAddress(currentProfile.defaultStartAddress);
      setUniverse(1);
    }
  };

  const handleProfileSave = (updatedProfiles: Record<string, PropProfile>) => {
    setAvailableProfiles(updatedProfiles);
    // You would typically save this to your backend here
    console.log('Updated profiles:', updatedProfiles);
  };

  const getElementIcon = (element: LEDElement) => {
    switch (element.type) {
      case 'strip': return <LinearScaleIcon sx={{ fontSize: 12, color: 'white' }} />;
      case 'diode': return <LightbulbIcon sx={{ fontSize: 10, color: 'white' }} />;
      case 'matrix': return <MatrixIcon sx={{ fontSize: 12, color: 'white' }} />;
      default: return <LightbulbIcon sx={{ fontSize: 10, color: 'white' }} />;
    }
  };

  const getChannelMap = () => {
    if (!currentProfile) return [];
    return getProfileChannelMap(currentProfile, startAddress);
  };

  if (!device) return null;

  return (
    <Dialog
      open={open}
      onClose={onClose}
      maxWidth="lg"
      fullWidth
      PaperProps={{
        sx: { height: '90vh' }
      }}
    >
      <DialogTitle>
        <Box display="flex" alignItems="center" justifyContent="space-between">
          <Box display="flex" alignItems="center" gap={2}>
            <SettingsIcon color="primary" />
            <Box>
              <Typography variant="h6">
                Tricorder Settings - {device.device_id}
              </Typography>
              <Typography variant="body2" color="text.secondary">
                {device.location} • {device.ip_address}
              </Typography>
            </Box>
          </Box>
          <IconButton onClick={onClose}>
            <CloseIcon />
          </IconButton>
        </Box>
      </DialogTitle>

      <DialogContent dividers>
        <Tabs value={activeTab} onChange={handleTabChange} sx={{ mb: 3 }}>
          <Tab icon={<PaletteIcon />} label="LED Configuration" />
          <Tab icon={<SettingsIcon />} label="General Settings" />
          <Tab icon={<EditIcon />} label="Profile Editor" />
        </Tabs>

        {activeTab === 0 && (
          <Grid container spacing={3}>
            {/* Visual Prop Layout */}
            <Grid item xs={12} md={6}>
              <Paper elevation={2} sx={{ p: 2, height: '500px' }}>
                <Typography variant="h6" gutterBottom>
                  {currentProfile?.name || 'Select a Profile'}
                </Typography>
                {currentProfile && (
                  <Box
                    sx={{
                      position: 'relative',
                      width: '100%',
                      height: '400px',
                      background: 'linear-gradient(135deg, #2c3e50 0%, #34495e 100%)',
                      borderRadius: 2,
                      border: '2px solid #3498db',
                      overflow: 'hidden',
                    }}
                  >
                    {/* Prop Outline */}
                    <svg
                      width="100%"
                      height="100%"
                      viewBox="0 0 200 300"
                      style={{ position: 'absolute', top: 0, left: 0 }}
                    >
                      {/* Main body */}
                      <rect
                        x="40"
                        y="50"
                        width="120"
                        height="200"
                        rx="10"
                        fill="none"
                        stroke="#3498db"
                        strokeWidth="2"
                      />
                      {/* Screen */}
                      <rect
                        x="50"
                        y="70"
                        width="100"
                        height="60"
                        rx="5"
                        fill="#1a1a1a"
                        stroke="#3498db"
                        strokeWidth="1"
                      />
                      {/* Buttons */}
                      <circle cx="70" cy="160" r="8" fill="#2c3e50" stroke="#3498db" strokeWidth="1" />
                      <circle cx="100" cy="160" r="8" fill="#2c3e50" stroke="#3498db" strokeWidth="1" />
                      <circle cx="130" cy="160" r="8" fill="#2c3e50" stroke="#3498db" strokeWidth="1" />
                      {/* Handle */}
                      <rect
                        x="80"
                        y="250"
                        width="40"
                        height="40"
                        rx="5"
                        fill="none"
                        stroke="#3498db"
                        strokeWidth="2"
                      />
                    </svg>

                    {/* LED Position Markers */}
                    {currentProfile.elements.map((element) => (
                      <Tooltip key={element.id} title={element.name} arrow>
                        <Box
                          sx={{
                            position: 'absolute',
                            left: `${element.x}%`,
                            top: `${element.y}%`,
                            transform: 'translate(-50%, -50%)',
                            width: element.type === 'strip' ? 24 : element.type === 'matrix' ? 20 : 16,
                            height: element.type === 'strip' ? 8 : element.type === 'matrix' ? 20 : 16,
                            backgroundColor: element.color,
                            border: '2px solid white',
                            borderRadius: element.type === 'strip' ? '4px' : element.type === 'matrix' ? '2px' : '50%',
                            cursor: 'pointer',
                            boxShadow: '0 2px 8px rgba(0,0,0,0.3)',
                            display: 'flex',
                            alignItems: 'center',
                            justifyContent: 'center',
                            '&:hover': {
                              transform: 'translate(-50%, -50%) scale(1.2)',
                              zIndex: 10,
                            },
                          }}
                        >
                          {getElementIcon(element)}
                        </Box>
                      </Tooltip>
                    ))}
                  </Box>
                )}
              </Paper>
            </Grid>

            {/* Profile and DMX Configuration Panel */}
            <Grid item xs={12} md={6}>
              <Paper elevation={2} sx={{ p: 2, height: '500px', overflow: 'auto' }}>
                <Box display="flex" justifyContent="space-between" alignItems="center" mb={2}>
                  <Typography variant="h6">
                    LED Profile Configuration
                  </Typography>
                  <Button
                    size="small"
                    startIcon={<ResetIcon />}
                    onClick={resetToDefaults}
                    color="warning"
                  >
                    Reset
                  </Button>
                </Box>

                <Card sx={{ mb: 3 }}>
                  <CardContent>
                    <Typography variant="subtitle1" fontWeight="bold" gutterBottom>
                      Prop Profile
                    </Typography>
                    <FormControl fullWidth sx={{ mb: 2 }}>
                      <InputLabel>Select Profile</InputLabel>
                      <Select
                        value={selectedProfile}
                        label="Select Profile"
                        onChange={(e) => setSelectedProfile(e.target.value)}
                      >
                        {Object.values(availableProfiles).map(profile => (
                          <MenuItem key={profile.id} value={profile.id}>
                            {profile.name}
                          </MenuItem>
                        ))}
                      </Select>
                    </FormControl>
                    {currentProfile && (
                      <Alert severity="info" sx={{ mb: 2 }}>
                        <Typography variant="body2">
                          {currentProfile.description}
                        </Typography>
                        <Typography variant="body2" fontWeight="bold">
                          Total Channels: {currentProfile.totalChannels}
                        </Typography>
                      </Alert>
                    )}
                    <Button
                      size="small"
                      startIcon={<EditIcon />}
                      onClick={() => setProfileEditorOpen(true)}
                      variant="outlined"
                      sx={{ mt: 1 }}
                    >
                      Edit Profiles
                    </Button>
                  </CardContent>
                </Card>

                <Card sx={{ mb: 3 }}>
                  <CardContent>
                    <Typography variant="subtitle1" fontWeight="bold" gutterBottom>
                      DMX Configuration
                    </Typography>
                    <Grid container spacing={2}>
                      <Grid item xs={6}>
                        <TextField
                          fullWidth
                          label="Start Address"
                          type="number"
                          value={startAddress}
                          onChange={(e) => setStartAddress(parseInt(e.target.value) || 1)}
                          inputProps={{ min: 1, max: 512 }}
                          size="small"
                        />
                      </Grid>
                      <Grid item xs={6}>
                        <FormControl fullWidth size="small">
                          <InputLabel>Universe</InputLabel>
                          <Select
                            value={universe}
                            label="Universe"
                            onChange={(e) => setUniverse(Number(e.target.value))}
                          >
                            <MenuItem value={1}>Universe 1</MenuItem>
                            <MenuItem value={2}>Universe 2</MenuItem>
                            <MenuItem value={3}>Universe 3</MenuItem>
                            <MenuItem value={4}>Universe 4</MenuItem>
                          </Select>
                        </FormControl>
                      </Grid>
                    </Grid>
                  </CardContent>
                </Card>

                {currentProfile && (
                  <Card>
                    <CardContent>
                      <Typography variant="subtitle1" fontWeight="bold" gutterBottom>
                        Channel Mapping
                      </Typography>
                      <TableContainer>
                        <Table size="small">
                          <TableHead>
                            <TableRow>
                              <TableCell>Element</TableCell>
                              <TableCell>Type</TableCell>
                              <TableCell>Channels</TableCell>
                              <TableCell>DMX Range</TableCell>
                            </TableRow>
                          </TableHead>
                          <TableBody>
                            {getChannelMap().map(({ element, startChannel, endChannel }) => (
                              <TableRow key={element.id}>
                                <TableCell>
                                  <Box display="flex" alignItems="center" gap={1}>
                                    <Box
                                      sx={{
                                        width: 12,
                                        height: 12,
                                        backgroundColor: element.color,
                                        borderRadius: element.type === 'diode' ? '50%' : '2px',
                                      }}
                                    />
                                    {element.name}
                                  </Box>
                                </TableCell>
                                <TableCell>
                                  <Chip
                                    label={element.type.toUpperCase()}
                                    color={element.type === 'strip' ? 'primary' : element.type === 'matrix' ? 'secondary' : 'default'}
                                    size="small"
                                  />
                                </TableCell>
                                <TableCell>{element.channels}</TableCell>
                                <TableCell>
                                  {startChannel === endChannel ? startChannel : `${startChannel}-${endChannel}`}
                                </TableCell>
                              </TableRow>
                            ))}
                          </TableBody>
                        </Table>
                      </TableContainer>
                    </CardContent>
                  </Card>
                )}
              </Paper>
            </Grid>
          </Grid>
        )}

        {activeTab === 2 && (
          <Box>
            <Alert severity="info" sx={{ mb: 3 }}>
              <Typography variant="body2">
                Use the Profile Editor to create and modify LED profiles. Changes made here will be available for all devices.
              </Typography>
            </Alert>
            <Button
              variant="contained"
              startIcon={<EditIcon />}
              onClick={() => setProfileEditorOpen(true)}
              size="large"
            >
              Open Profile Editor
            </Button>
          </Box>
        )}
      </DialogContent>

      <DialogActions sx={{ p: 2 }}>
        <Button onClick={onClose} color="inherit">
          Cancel
        </Button>
        <Button
          onClick={handleSave}
          variant="contained"
          startIcon={<SaveIcon />}
          color="primary"
        >
          Save Settings
        </Button>
      </DialogActions>

      {/* Profile Editor Modal */}
      <ProfileEditor
        open={profileEditorOpen}
        onClose={() => setProfileEditorOpen(false)}
        onSave={handleProfileSave}
      />
    </Dialog>
  );
};

export default TricorderSettingsModal;
