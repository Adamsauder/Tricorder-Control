import { createSlice, PayloadAction } from '@reduxjs/toolkit';

interface Device {
  device_id: string;
  ip_address: string;
  firmware_version: string;
  status: 'online' | 'offline';
  last_seen: string;
  battery_voltage?: number;
  temperature?: number;
  current_video?: string;
  video_playing: boolean;
}

interface DeviceState {
  devices: Device[];
  loading: boolean;
  error: string | null;
}

const initialState: DeviceState = {
  devices: [],
  loading: false,
  error: null,
};

const deviceSlice = createSlice({
  name: 'devices',
  initialState,
  reducers: {
    setDevices: (state, action: PayloadAction<Device[]>) => {
      state.devices = action.payload;
    },
    updateDevice: (state, action: PayloadAction<Device>) => {
      const index = state.devices.findIndex(d => d.device_id === action.payload.device_id);
      if (index !== -1) {
        state.devices[index] = action.payload;
      } else {
        state.devices.push(action.payload);
      }
    },
    removeDevice: (state, action: PayloadAction<string>) => {
      state.devices = state.devices.filter(d => d.device_id !== action.payload);
    },
    setLoading: (state, action: PayloadAction<boolean>) => {
      state.loading = action.payload;
    },
    setError: (state, action: PayloadAction<string | null>) => {
      state.error = action.payload;
    },
  },
});

export const { setDevices, updateDevice, removeDevice, setLoading, setError } = deviceSlice.actions;
export default deviceSlice.reducer;
