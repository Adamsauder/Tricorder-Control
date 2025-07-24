import React, { useState, useEffect, useRef } from 'react';
import {
  Box,
  Card,
  CardContent,
  Typography,
  Button,
  ButtonGroup,
  Slider,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
  Chip,
  Paper,
  Grid,
} from '@mui/material';
import {
  PlayArrow,
  Pause,
  Stop,
  Brightness6,
  AspectRatio,
  DevicesOther,
} from '@mui/icons-material';

interface ESP32SimulatorProps {
  deviceId?: string;
  width?: number;
  height?: number;
  onCommand?: (command: any) => void;
}

interface VideoFrame {
  src: string;
  name: string;
  category: 'static' | 'animated' | 'color';
}

// Mock video frames based on your firmware structure
const mockVideoFrames: VideoFrame[] = [
  { src: '/api/simulator/frames/startup.jpg', name: 'startup.jpg', category: 'static' },
  { src: '/api/simulator/frames/startup_mid.jpg', name: 'startup_mid.jpg', category: 'static' },
  { src: '/api/simulator/frames/static_test.jpg', name: 'static_test.jpg', category: 'static' },
  { src: '/api/simulator/frames/animated_mid.jpg', name: 'animated_mid.jpg', category: 'animated' },
  { src: '/api/simulator/frames/animated_test.jpg', name: 'animated_test.jpg', category: 'animated' },
  { src: '/api/simulator/frames/color_red.jpg', name: 'color_red.jpg', category: 'color' },
  { src: '/api/simulator/frames/color_green.jpg', name: 'color_green.jpg', category: 'color' },
  { src: '/api/simulator/frames/color_blue.jpg', name: 'color_blue.jpg', category: 'color' },
  { src: '/api/simulator/frames/color_cyan.jpg', name: 'color_cyan.jpg', category: 'color' },
  { src: '/api/simulator/frames/color_magenta.jpg', name: 'color_magenta.jpg', category: 'color' },
  { src: '/api/simulator/frames/color_yellow.jpg', name: 'color_yellow.jpg', category: 'color' },
  { src: '/api/simulator/frames/color_white.jpg', name: 'color_white.jpg', category: 'color' },
];

export const ESP32Simulator: React.FC<ESP32SimulatorProps> = ({
  deviceId = 'SIMULATOR_001',
  width = 320,
  height = 240,
  onCommand
}) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [currentFrame, setCurrentFrame] = useState<VideoFrame | null>(null);
  const [isPlaying, setIsPlaying] = useState(false);
  const [brightness, setBrightness] = useState(80);
  const [scale, setScale] = useState(2);
  const [animationFrame, setAnimationFrame] = useState(0);
  const [selectedVideo, setSelectedVideo] = useState('');
  const [fps, setFps] = useState(15);
  const animationIntervalRef = useRef<NodeJS.Timeout | null>(null);

  // Simulate animated sequences
  const animatedSequences = {
    'animated_test': Array.from({ length: 30 }, (_, i) => 
      `/api/simulator/frames/animated_test_frame_${String(i + 1).padStart(3, '0')}.jpg`
    ),
    'color_cycle': [
      '/api/simulator/frames/color_red.jpg',
      '/api/simulator/frames/color_yellow.jpg',
      '/api/simulator/frames/color_green.jpg',
      '/api/simulator/frames/color_cyan.jpg',
      '/api/simulator/frames/color_blue.jpg',
      '/api/simulator/frames/color_magenta.jpg',
    ]
  };

  // Initialize canvas
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // Set canvas size
    canvas.width = width;
    canvas.height = height;

    // Draw initial black screen
    ctx.fillStyle = '#000000';
    ctx.fillRect(0, 0, width, height);

    // Draw device info
    ctx.fillStyle = '#00ff00';
    ctx.font = '12px monospace';
    ctx.textAlign = 'center';
    ctx.fillText('ESP32 Tricorder Simulator', width / 2, height / 2 - 10);
    ctx.fillText(deviceId, width / 2, height / 2 + 10);
    ctx.fillText('Ready...', width / 2, height / 2 + 30);
  }, [width, height, deviceId]);

  // Handle frame loading and display
  const displayFrame = (frameSrc: string) => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const img = new Image();
    img.onload = () => {
      // Apply brightness filter
      ctx.filter = `brightness(${brightness}%)`;
      
      // Clear canvas
      ctx.fillStyle = '#000000';
      ctx.fillRect(0, 0, width, height);
      
      // Draw image scaled to fit screen
      ctx.drawImage(img, 0, 0, width, height);
      
      // Reset filter
      ctx.filter = 'none';
    };
    
    img.onerror = () => {
      // Fallback to colored rectangle for missing images
      const colorMatch = frameSrc.match(/color_(\w+)/);
      if (colorMatch) {
        const colorMap: Record<string, string> = {
          red: '#ff0000',
          green: '#00ff00',
          blue: '#0000ff',
          cyan: '#00ffff',
          magenta: '#ff00ff',
          yellow: '#ffff00',
          white: '#ffffff',
        };
        ctx.fillStyle = colorMap[colorMatch[1]] || '#ffffff';
        ctx.fillRect(0, 0, width, height);
      } else {
        // Default pattern for missing files
        ctx.fillStyle = '#333333';
        ctx.fillRect(0, 0, width, height);
        ctx.fillStyle = '#ffffff';
        ctx.font = '16px monospace';
        ctx.textAlign = 'center';
        ctx.fillText('Frame Missing', width / 2, height / 2);
        ctx.fillText(frameSrc.split('/').pop() || '', width / 2, height / 2 + 20);
      }
    };
    
    img.src = frameSrc;
  };

  // Animation loop
  useEffect(() => {
    if (!isPlaying || !selectedVideo) return;

    const sequence = animatedSequences[selectedVideo as keyof typeof animatedSequences];
    if (!sequence) {
      // Single frame video
      const frame = mockVideoFrames.find(f => f.name === selectedVideo);
      if (frame) {
        displayFrame(frame.src);
      }
      return;
    }

    // Animated sequence
    animationIntervalRef.current = setInterval(() => {
      setAnimationFrame(prev => {
        const nextFrame = (prev + 1) % sequence.length;
        displayFrame(sequence[nextFrame]);
        return nextFrame;
      });
    }, 1000 / fps);

    return () => {
      if (animationIntervalRef.current) {
        clearInterval(animationIntervalRef.current);
      }
    };
  }, [isPlaying, selectedVideo, fps, brightness]);

  // Control handlers
  const handlePlay = () => {
    if (!selectedVideo) return;
    
    setIsPlaying(true);
    onCommand?.({
      action: 'play_video',
      device_id: deviceId,
      video: selectedVideo,
      fps: fps
    });
  };

  const handlePause = () => {
    setIsPlaying(false);
    if (animationIntervalRef.current) {
      clearInterval(animationIntervalRef.current);
    }
    onCommand?.({
      action: 'pause_video',
      device_id: deviceId
    });
  };

  const handleStop = () => {
    setIsPlaying(false);
    setAnimationFrame(0);
    if (animationIntervalRef.current) {
      clearInterval(animationIntervalRef.current);
    }
    
    // Clear screen
    const canvas = canvasRef.current;
    if (canvas) {
      const ctx = canvas.getContext('2d');
      if (ctx) {
        ctx.fillStyle = '#000000';
        ctx.fillRect(0, 0, width, height);
      }
    }
    
    onCommand?.({
      action: 'stop_video',
      device_id: deviceId
    });
  };

  const getCategoryColor = (category: string) => {
    switch (category) {
      case 'static': return 'primary';
      case 'animated': return 'secondary';
      case 'color': return 'success';
      default: return 'default';
    }
  };

  return (
    <Card sx={{ width: '100%', maxWidth: 600 }}>
      <CardContent>
        <Box display="flex" alignItems="center" gap={1} mb={2}>
          <DevicesOther />
          <Typography variant="h6">
            ESP32 Screen Simulator
          </Typography>
          <Chip label={deviceId} size="small" variant="outlined" />
        </Box>

        {/* Screen Display */}
        <Paper 
          elevation={3} 
          sx={{ 
            p: 2, 
            mb: 2, 
            backgroundColor: '#000', 
            display: 'flex', 
            justifyContent: 'center',
            border: '2px solid #333'
          }}
        >
          <canvas
            ref={canvasRef}
            style={{
              transform: `scale(${scale})`,
              transformOrigin: 'center',
              border: '1px solid #555',
              maxWidth: '100%',
              height: 'auto'
            }}
          />
        </Paper>

        {/* Controls */}
        <Grid container spacing={2}>
          <Grid item xs={12}>
            <FormControl fullWidth size="small">
              <InputLabel>Video/Animation</InputLabel>
              <Select
                value={selectedVideo}
                onChange={(e) => setSelectedVideo(e.target.value)}
                label="Video/Animation"
              >
                <MenuItem value="">
                  <em>Select video...</em>
                </MenuItem>
                {mockVideoFrames.map((frame) => (
                  <MenuItem key={frame.name} value={frame.name}>
                    <Box display="flex" alignItems="center" gap={1}>
                      <Chip 
                        label={frame.category} 
                        size="small" 
                        color={getCategoryColor(frame.category) as any}
                      />
                      {frame.name}
                    </Box>
                  </MenuItem>
                ))}
                <MenuItem value="animated_test">
                  <Box display="flex" alignItems="center" gap={1}>
                    <Chip label="animated" size="small" color="secondary" />
                    Animated Test Sequence (30 frames)
                  </Box>
                </MenuItem>
                <MenuItem value="color_cycle">
                  <Box display="flex" alignItems="center" gap={1}>
                    <Chip label="animated" size="small" color="secondary" />
                    Color Cycle Animation
                  </Box>
                </MenuItem>
              </Select>
            </FormControl>
          </Grid>

          <Grid item xs={12} sm={6}>
            <ButtonGroup variant="contained" fullWidth>
              <Button 
                startIcon={<PlayArrow />} 
                onClick={handlePlay}
                disabled={!selectedVideo || isPlaying}
              >
                Play
              </Button>
              <Button 
                startIcon={<Pause />} 
                onClick={handlePause}
                disabled={!isPlaying}
              >
                Pause
              </Button>
              <Button 
                startIcon={<Stop />} 
                onClick={handleStop}
                disabled={!selectedVideo}
              >
                Stop
              </Button>
            </ButtonGroup>
          </Grid>

          <Grid item xs={12} sm={6}>
            <Box display="flex" alignItems="center" gap={1}>
              <Brightness6 fontSize="small" />
              <Slider
                value={brightness}
                onChange={(_, value) => setBrightness(value as number)}
                min={10}
                max={100}
                size="small"
                valueLabelDisplay="auto"
                valueLabelFormat={(value) => `${value}%`}
              />
            </Box>
          </Grid>

          <Grid item xs={6}>
            <Typography variant="body2" gutterBottom>
              Scale: {scale}x
            </Typography>
            <Slider
              value={scale}
              onChange={(_, value) => setScale(value as number)}
              min={0.5}
              max={3}
              step={0.1}
              size="small"
              valueLabelDisplay="auto"
            />
          </Grid>

          <Grid item xs={6}>
            <Typography variant="body2" gutterBottom>
              FPS: {fps}
            </Typography>
            <Slider
              value={fps}
              onChange={(_, value) => setFps(value as number)}
              min={5}
              max={30}
              step={1}
              size="small"
              valueLabelDisplay="auto"
            />
          </Grid>
        </Grid>

        {/* Status Info */}
        <Box mt={2} p={1} bgcolor="grey.100" borderRadius={1}>
          <Typography variant="caption" display="block">
            Resolution: {width} x {height} px
          </Typography>
          <Typography variant="caption" display="block">
            Status: {isPlaying ? `Playing ${selectedVideo}` : 'Stopped'}
          </Typography>
          {isPlaying && animatedSequences[selectedVideo as keyof typeof animatedSequences] && (
            <Typography variant="caption" display="block">
              Frame: {animationFrame + 1} / {animatedSequences[selectedVideo as keyof typeof animatedSequences].length}
            </Typography>
          )}
        </Box>
      </CardContent>
    </Card>
  );
};

export default ESP32Simulator;
