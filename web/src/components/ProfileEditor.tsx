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
  Card,
  CardContent,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
  List,
  ListItem,
  ListItemText,
  ListItemSecondaryAction,
  Divider,
  Alert,
  Tooltip,
  Fab,
} from '@mui/material';
import {
  Close as CloseIcon,
  Edit as EditIcon,
  Add as AddIcon,
  Delete as DeleteIcon,
  Save as SaveIcon,
  Cancel as CancelIcon,
  Lightbulb as LightbulbIcon,
  LinearScale as LinearScaleIcon,
  GridOn as MatrixIcon,
  DragIndicator as DragIcon,
} from '@mui/icons-material';
import { PropProfile, LEDElement, PROP_PROFILES } from './PropProfiles';

interface ProfileEditorProps {
  open: boolean;
  onClose: () => void;
  onSave: (profiles: Record<string, PropProfile>) => void;
}

const ProfileEditor: React.FC<ProfileEditorProps> = ({ open, onClose, onSave }) => {
  const [profiles, setProfiles] = useState<Record<string, PropProfile>>(PROP_PROFILES);
  const [selectedProfile, setSelectedProfile] = useState<string>('');
  const [editingProfile, setEditingProfile] = useState<PropProfile | null>(null);
  const [editingElement, setEditingElement] = useState<LEDElement | null>(null);
  const [isCreatingProfile, setIsCreatingProfile] = useState(false);
  const [isCreatingElement, setIsCreatingElement] = useState(false);

  useEffect(() => {
    if (selectedProfile && profiles[selectedProfile]) {
      setEditingProfile({ ...profiles[selectedProfile] });
    }
  }, [selectedProfile, profiles]);

  const handleCreateProfile = () => {
    const newProfile: PropProfile = {
      id: `custom_${Date.now()}`,
      name: 'New Profile',
      description: 'Custom profile description',
      totalChannels: 0,
      defaultStartAddress: 1,
      elements: []
    };
    setEditingProfile(newProfile);
    setSelectedProfile(newProfile.id);
    setIsCreatingProfile(true);
  };

  const handleSaveProfile = () => {
    if (editingProfile) {
      // Calculate total channels
      const totalChannels = editingProfile.elements.reduce((sum, element) => sum + element.channels, 0);
      const updatedProfile = { ...editingProfile, totalChannels };
      
      const updatedProfiles = {
        ...profiles,
        [updatedProfile.id]: updatedProfile
      };
      setProfiles(updatedProfiles);
      setIsCreatingProfile(false);
    }
  };

  const handleDeleteProfile = (profileId: string) => {
    const updatedProfiles = { ...profiles };
    delete updatedProfiles[profileId];
    setProfiles(updatedProfiles);
    if (selectedProfile === profileId) {
      setSelectedProfile('');
      setEditingProfile(null);
    }
  };

  const handleCreateElement = () => {
    const newElement: LEDElement = {
      id: `element_${Date.now()}`,
      name: 'New LED Element',
      type: 'diode',
      channels: 3,
      x: 50,
      y: 50,
      color: '#ff0000',
      description: 'New LED element description'
    };
    setEditingElement(newElement);
    setIsCreatingElement(true);
  };

  const handleSaveElement = () => {
    if (editingElement && editingProfile) {
      let updatedElements;
      if (isCreatingElement) {
        updatedElements = [...editingProfile.elements, editingElement];
      } else {
        updatedElements = editingProfile.elements.map(el => 
          el.id === editingElement.id ? editingElement : el
        );
      }
      setEditingProfile({ ...editingProfile, elements: updatedElements });
      setEditingElement(null);
      setIsCreatingElement(false);
    }
  };

  const handleDeleteElement = (elementId: string) => {
    if (editingProfile) {
      const updatedElements = editingProfile.elements.filter(el => el.id !== elementId);
      setEditingProfile({ ...editingProfile, elements: updatedElements });
    }
  };

  const handleEditElement = (element: LEDElement) => {
    setEditingElement({ ...element });
    setIsCreatingElement(false);
  };

  const getElementIcon = (type: string) => {
    switch (type) {
      case 'strip': return <LinearScaleIcon />;
      case 'matrix': return <MatrixIcon />;
      default: return <LightbulbIcon />;
    }
  };

  const handleFinalSave = () => {
    onSave(profiles);
    onClose();
  };

  return (
    <Dialog open={open} onClose={onClose} maxWidth="xl" fullWidth PaperProps={{ sx: { height: '90vh' } }}>
      <DialogTitle>
        <Box display="flex" alignItems="center" justifyContent="space-between">
          <Typography variant="h6">LED Profile Editor</Typography>
          <IconButton onClick={onClose}>
            <CloseIcon />
          </IconButton>
        </Box>
      </DialogTitle>

      <DialogContent dividers>
        <Grid container spacing={3} sx={{ height: '100%' }}>
          {/* Profile List */}
          <Grid item xs={12} md={4}>
            <Paper elevation={2} sx={{ p: 2, height: '600px', overflow: 'auto' }}>
              <Box display="flex" justifyContent="space-between" alignItems="center" mb={2}>
                <Typography variant="h6">Profiles</Typography>
                <Button
                  size="small"
                  startIcon={<AddIcon />}
                  onClick={handleCreateProfile}
                  variant="contained"
                >
                  New
                </Button>
              </Box>
              
              <List>
                {Object.values(profiles).map((profile) => (
                  <ListItem
                    key={profile.id}
                    button
                    selected={selectedProfile === profile.id}
                    onClick={() => setSelectedProfile(profile.id)}
                  >
                    <ListItemText
                      primary={profile.name}
                      secondary={`${profile.elements.length} elements, ${profile.totalChannels} channels`}
                    />
                    <ListItemSecondaryAction>
                      <IconButton
                        size="small"
                        onClick={(e) => {
                          e.stopPropagation();
                          handleDeleteProfile(profile.id);
                        }}
                        disabled={Object.keys(PROP_PROFILES).includes(profile.id)}
                      >
                        <DeleteIcon />
                      </IconButton>
                    </ListItemSecondaryAction>
                  </ListItem>
                ))}
              </List>
            </Paper>
          </Grid>

          {/* Profile Editor */}
          <Grid item xs={12} md={8}>
            {editingProfile ? (
              <Paper elevation={2} sx={{ p: 2, height: '600px', overflow: 'auto' }}>
                <Box display="flex" justifyContent="space-between" alignItems="center" mb={2}>
                  <Typography variant="h6">Edit Profile: {editingProfile.name}</Typography>
                  <Box>
                    {isCreatingProfile && (
                      <Button
                        size="small"
                        startIcon={<SaveIcon />}
                        onClick={handleSaveProfile}
                        variant="contained"
                        sx={{ mr: 1 }}
                      >
                        Save Profile
                      </Button>
                    )}
                    <Button
                      size="small"
                      startIcon={<AddIcon />}
                      onClick={handleCreateElement}
                      variant="outlined"
                    >
                      Add Element
                    </Button>
                  </Box>
                </Box>

                {/* Profile Details */}
                <Card sx={{ mb: 3 }}>
                  <CardContent>
                    <Grid container spacing={2}>
                      <Grid item xs={6}>
                        <TextField
                          fullWidth
                          label="Profile Name"
                          value={editingProfile.name}
                          onChange={(e) => setEditingProfile({ ...editingProfile, name: e.target.value })}
                          size="small"
                        />
                      </Grid>
                      <Grid item xs={6}>
                        <TextField
                          fullWidth
                          label="Default Start Address"
                          type="number"
                          value={editingProfile.defaultStartAddress}
                          onChange={(e) => setEditingProfile({ ...editingProfile, defaultStartAddress: parseInt(e.target.value) || 1 })}
                          inputProps={{ min: 1, max: 512 }}
                          size="small"
                        />
                      </Grid>
                      <Grid item xs={12}>
                        <TextField
                          fullWidth
                          label="Description"
                          multiline
                          rows={2}
                          value={editingProfile.description}
                          onChange={(e) => setEditingProfile({ ...editingProfile, description: e.target.value })}
                          size="small"
                        />
                      </Grid>
                    </Grid>
                  </CardContent>
                </Card>

                {/* Visual Layout Preview */}
                <Card sx={{ mb: 3 }}>
                  <CardContent>
                    <Typography variant="subtitle1" gutterBottom>Visual Layout Preview</Typography>
                    <Box
                      sx={{
                        position: 'relative',
                        width: '100%',
                        height: '200px',
                        background: 'linear-gradient(135deg, #2c3e50 0%, #34495e 100%)',
                        borderRadius: 2,
                        border: '2px solid #3498db',
                        overflow: 'hidden',
                      }}
                    >
                      {/* Base prop outline */}
                      <svg width="100%" height="100%" viewBox="0 0 200 150" style={{ position: 'absolute', top: 0, left: 0 }}>
                        <rect x="40" y="30" width="120" height="90" rx="8" fill="none" stroke="#3498db" strokeWidth="2" />
                        <rect x="50" y="40" width="100" height="30" rx="3" fill="#1a1a1a" stroke="#3498db" strokeWidth="1" />
                      </svg>

                      {/* LED Elements */}
                      {editingProfile.elements.map((element) => (
                        <Tooltip key={element.id} title={element.name} arrow>
                          <Box
                            onClick={() => handleEditElement(element)}
                            sx={{
                              position: 'absolute',
                              left: `${element.x}%`,
                              top: `${element.y}%`,
                              transform: 'translate(-50%, -50%)',
                              width: element.type === 'strip' ? 20 : element.type === 'matrix' ? 16 : 12,
                              height: element.type === 'strip' ? 6 : element.type === 'matrix' ? 16 : 12,
                              backgroundColor: element.color,
                              border: '2px solid white',
                              borderRadius: element.type === 'strip' ? '3px' : element.type === 'matrix' ? '2px' : '50%',
                              cursor: 'pointer',
                              boxShadow: '0 2px 4px rgba(0,0,0,0.3)',
                              display: 'flex',
                              alignItems: 'center',
                              justifyContent: 'center',
                              '&:hover': {
                                transform: 'translate(-50%, -50%) scale(1.3)',
                                zIndex: 10,
                              },
                            }}
                          >
                            {React.cloneElement(getElementIcon(element.type), { 
                              sx: { fontSize: 8, color: 'white' } 
                            })}
                          </Box>
                        </Tooltip>
                      ))}
                    </Box>
                  </CardContent>
                </Card>

                {/* Elements List */}
                <Card>
                  <CardContent>
                    <Typography variant="subtitle1" gutterBottom>LED Elements</Typography>
                    {editingProfile.elements.length === 0 ? (
                      <Alert severity="info">No LED elements defined. Click "Add Element" to start.</Alert>
                    ) : (
                      <List>
                        {editingProfile.elements.map((element, index) => (
                          <React.Fragment key={element.id}>
                            <ListItem>
                              <Box display="flex" alignItems="center" mr={2}>
                                <DragIcon sx={{ color: 'text.secondary', mr: 1 }} />
                                {getElementIcon(element.type)}
                              </Box>
                              <ListItemText
                                primary={element.name}
                                secondary={`${element.type.toUpperCase()} • ${element.channels} channels • Position: ${element.x}%, ${element.y}%`}
                              />
                              <Box display="flex" alignItems="center" gap={1}>
                                <Box
                                  sx={{
                                    width: 20,
                                    height: 20,
                                    backgroundColor: element.color,
                                    borderRadius: element.type === 'diode' ? '50%' : '2px',
                                    border: '1px solid #ccc',
                                  }}
                                />
                                <Chip label={`${element.channels}ch`} size="small" />
                                <IconButton size="small" onClick={() => handleEditElement(element)}>
                                  <EditIcon />
                                </IconButton>
                                <IconButton size="small" onClick={() => handleDeleteElement(element.id)}>
                                  <DeleteIcon />
                                </IconButton>
                              </Box>
                            </ListItem>
                            {index < editingProfile.elements.length - 1 && <Divider />}
                          </React.Fragment>
                        ))}
                      </List>
                    )}
                  </CardContent>
                </Card>
              </Paper>
            ) : (
              <Paper elevation={2} sx={{ p: 4, height: '600px', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
                <Typography variant="h6" color="text.secondary">
                  Select a profile to edit or create a new one
                </Typography>
              </Paper>
            )}
          </Grid>
        </Grid>
      </DialogContent>

      {/* Element Editor Dialog */}
      <Dialog open={!!editingElement} onClose={() => setEditingElement(null)} maxWidth="sm" fullWidth>
        <DialogTitle>
          {isCreatingElement ? 'Create LED Element' : 'Edit LED Element'}
        </DialogTitle>
        <DialogContent>
          {editingElement && (
            <Grid container spacing={2} sx={{ mt: 1 }}>
              <Grid item xs={12}>
                <TextField
                  fullWidth
                  label="Element Name"
                  value={editingElement.name}
                  onChange={(e) => setEditingElement({ ...editingElement, name: e.target.value })}
                />
              </Grid>
              <Grid item xs={6}>
                <FormControl fullWidth>
                  <InputLabel>Type</InputLabel>
                  <Select
                    value={editingElement.type}
                    label="Type"
                    onChange={(e) => setEditingElement({ ...editingElement, type: e.target.value as any })}
                  >
                    <MenuItem value="diode">Single LED</MenuItem>
                    <MenuItem value="strip">LED Strip</MenuItem>
                    <MenuItem value="matrix">LED Matrix</MenuItem>
                  </Select>
                </FormControl>
              </Grid>
              <Grid item xs={6}>
                <TextField
                  fullWidth
                  label="Channels"
                  type="number"
                  value={editingElement.channels}
                  onChange={(e) => setEditingElement({ ...editingElement, channels: parseInt(e.target.value) || 1 })}
                  inputProps={{ min: 1, max: 100 }}
                />
              </Grid>
              <Grid item xs={4}>
                <TextField
                  fullWidth
                  label="X Position"
                  type="number"
                  value={editingElement.x}
                  onChange={(e) => setEditingElement({ ...editingElement, x: parseInt(e.target.value) || 0 })}
                  inputProps={{ min: 0, max: 100 }}
                  InputProps={{ endAdornment: '%' }}
                />
              </Grid>
              <Grid item xs={4}>
                <TextField
                  fullWidth
                  label="Y Position"
                  type="number"
                  value={editingElement.y}
                  onChange={(e) => setEditingElement({ ...editingElement, y: parseInt(e.target.value) || 0 })}
                  inputProps={{ min: 0, max: 100 }}
                  InputProps={{ endAdornment: '%' }}
                />
              </Grid>
              <Grid item xs={4}>
                <TextField
                  fullWidth
                  label="Color"
                  type="color"
                  value={editingElement.color}
                  onChange={(e) => setEditingElement({ ...editingElement, color: e.target.value })}
                />
              </Grid>
              <Grid item xs={12}>
                <TextField
                  fullWidth
                  label="Description"
                  multiline
                  rows={2}
                  value={editingElement.description || ''}
                  onChange={(e) => setEditingElement({ ...editingElement, description: e.target.value })}
                />
              </Grid>
            </Grid>
          )}
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setEditingElement(null)}>Cancel</Button>
          <Button onClick={handleSaveElement} variant="contained">
            {isCreatingElement ? 'Create' : 'Save'}
          </Button>
        </DialogActions>
      </Dialog>

      <DialogActions sx={{ p: 2 }}>
        <Button onClick={onClose} color="inherit">
          Cancel
        </Button>
        <Button onClick={handleFinalSave} variant="contained" startIcon={<SaveIcon />}>
          Save All Changes
        </Button>
      </DialogActions>
    </Dialog>
  );
};

export default ProfileEditor;
