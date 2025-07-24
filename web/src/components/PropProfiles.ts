// LED Prop Profiles Configuration
export interface LEDElement {
  id: string;
  name: string;
  type: 'strip' | 'diode' | 'matrix';
  channels: number; // Number of DMX channels this element uses
  x: number; // Position on prop image (percentage)
  y: number; // Position on prop image (percentage)
  color: string; // Visual color for UI
  description?: string;
}

export interface PropProfile {
  id: string;
  name: string;
  description: string;
  image?: string; // Path to prop image
  totalChannels: number; // Total DMX channels used by this prop
  elements: LEDElement[];
  defaultStartAddress: number;
}

// Predefined prop profiles
export const PROP_PROFILES: Record<string, PropProfile> = {
  tricorder_mk1: {
    id: 'tricorder_mk1',
    name: 'Tricorder Mark I',
    description: 'Standard Tricorder with main display strip and scanner elements',
    totalChannels: 15,
    defaultStartAddress: 1,
    elements: [
      {
        id: 'main_strip',
        name: 'Main Display Strip',
        type: 'strip',
        channels: 3, // RGB
        x: 50,
        y: 35,
        color: '#00ff00',
        description: 'Primary display LED strip across the screen area'
      },
      {
        id: 'scanner_led',
        name: 'Scanner LED',
        type: 'diode',
        channels: 3, // RGB
        x: 75,
        y: 45,
        color: '#ff0000',
        description: 'Red scanning LED in the sensor area'
      },
      {
        id: 'side_strip_left',
        name: 'Left Side Strip',
        type: 'strip',
        channels: 3, // RGB
        x: 25,
        y: 60,
        color: '#0066ff',
        description: 'Side indicator strip - port side'
      },
      {
        id: 'side_strip_right',
        name: 'Right Side Strip',
        type: 'strip',
        channels: 3, // RGB
        x: 75,
        y: 60,
        color: '#0066ff',
        description: 'Side indicator strip - starboard side'
      },
      {
        id: 'status_led',
        name: 'Status LED',
        type: 'diode',
        channels: 3, // RGB
        x: 50,
        y: 85,
        color: '#ffaa00',
        description: 'Device status indicator LED'
      }
    ]
  },
  tricorder_mk2: {
    id: 'tricorder_mk2',
    name: 'Tricorder Mark II',
    description: 'Advanced Tricorder with additional LED matrix and enhanced lighting',
    totalChannels: 21,
    defaultStartAddress: 1,
    elements: [
      {
        id: 'main_strip',
        name: 'Main Display Strip',
        type: 'strip',
        channels: 3, // RGB
        x: 50,
        y: 30,
        color: '#00ff00',
        description: 'Primary display LED strip'
      },
      {
        id: 'led_matrix',
        name: 'LED Matrix',
        type: 'matrix',
        channels: 9, // 3x3 RGB matrix
        x: 50,
        y: 50,
        color: '#ff6600',
        description: '3x3 LED matrix for advanced displays'
      },
      {
        id: 'scanner_array',
        name: 'Scanner Array',
        type: 'strip',
        channels: 6, // 2 RGB LEDs
        x: 75,
        y: 40,
        color: '#ff0000',
        description: 'Dual scanner LED array'
      },
      {
        id: 'status_led',
        name: 'Status LED',
        type: 'diode',
        channels: 3, // RGB
        x: 50,
        y: 85,
        color: '#ffaa00',
        description: 'Device status indicator LED'
      }
    ]
  },
  communicator: {
    id: 'communicator',
    name: 'Communicator',
    description: 'Classic flip communicator with simple LED indicators',
    totalChannels: 9,
    defaultStartAddress: 1,
    elements: [
      {
        id: 'signal_led',
        name: 'Signal LED',
        type: 'diode',
        channels: 3, // RGB
        x: 50,
        y: 25,
        color: '#00ff00',
        description: 'Communication signal indicator'
      },
      {
        id: 'power_led',
        name: 'Power LED',
        type: 'diode',
        channels: 3, // RGB
        x: 30,
        y: 70,
        color: '#0066ff',
        description: 'Power status indicator'
      },
      {
        id: 'activity_led',
        name: 'Activity LED',
        type: 'diode',
        channels: 3, // RGB
        x: 70,
        y: 70,
        color: '#ffaa00',
        description: 'Activity indicator LED'
      }
    ]
  },
  padd: {
    id: 'padd',
    name: 'PADD',
    description: 'Personal Access Display Device with edge lighting',
    totalChannels: 12,
    defaultStartAddress: 1,
    elements: [
      {
        id: 'edge_strip_top',
        name: 'Top Edge Strip',
        type: 'strip',
        channels: 3, // RGB
        x: 50,
        y: 15,
        color: '#0066ff',
        description: 'Top edge lighting strip'
      },
      {
        id: 'edge_strip_bottom',
        name: 'Bottom Edge Strip',
        type: 'strip',
        channels: 3, // RGB
        x: 50,
        y: 85,
        color: '#0066ff',
        description: 'Bottom edge lighting strip'
      },
      {
        id: 'edge_strip_left',
        name: 'Left Edge Strip',
        type: 'strip',
        channels: 3, // RGB
        x: 15,
        y: 50,
        color: '#0066ff',
        description: 'Left edge lighting strip'
      },
      {
        id: 'edge_strip_right',
        name: 'Right Edge Strip',
        type: 'strip',
        channels: 3, // RGB
        x: 85,
        y: 50,
        color: '#0066ff',
        description: 'Right edge lighting strip'
      }
    ]
  }
};

export const getProfileById = (profileId: string): PropProfile | undefined => {
  return PROP_PROFILES[profileId];
};

export const getProfileChannelMap = (profile: PropProfile, startAddress: number) => {
  const channelMap: Array<{
    element: LEDElement;
    startChannel: number;
    endChannel: number;
  }> = [];
  
  let currentChannel = startAddress;
  
  profile.elements.forEach(element => {
    channelMap.push({
      element,
      startChannel: currentChannel,
      endChannel: currentChannel + element.channels - 1
    });
    currentChannel += element.channels;
  });
  
  return channelMap;
};
