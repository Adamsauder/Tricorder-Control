import axios from 'axios';
import { io, Socket } from 'socket.io-client';

// API base configuration
const API_BASE = '/api';
const WS_BASE = '/';

// Create axios instance with default config
const api = axios.create({
  baseURL: API_BASE,
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json',
  },
});

// Types for API responses
export interface TricorderDevice {
  device_id: string;
  status: 'online' | 'offline' | 'error';
  battery?: number;
  video_playing?: boolean;
  location?: string;
  ip_address: string;
  firmware_version: string;
  last_seen: string;
  temperature?: number;
  current_video?: string;
  prop_profile?: string;
  dmx_start_address?: number;
  dmx_universe?: number;
  free_heap?: number;
  uptime?: number;
  sd_card_initialized?: boolean;
  video_looping?: boolean;
  current_frame?: number;
}

export interface ServerInfo {
  server_ip: string;
  udp_port: number;
  web_port: number;
  device_count: number;
  uptime: number;
  status: string;
}

export interface CommandRequest {
  device_id: string;
  action: string;
  data?: string;
  parameters?: Record<string, any>;
}

export interface CommandResponse {
  success: boolean;
  command_id?: string;
  message?: string;
  error?: string;
}

export interface LEDParameters {
  r: number;
  g: number;
  b: number;
}

export interface IndividualLEDParameters {
  ledIndex: number;
  r: number;
  g: number;
  b: number;
}

export interface LEDEffectParameters {
  r: number;
  g: number;
  b: number;
  delay?: number;
  duration?: number;
}

export interface BrightnessParameters {
  brightness: number;
}

export interface VideoParameters {
  filename: string;
  loop?: boolean;
}

// WebSocket connection
class WebSocketService {
  private socket: Socket | null = null;
  private callbacks: Map<string, Function[]> = new Map();

  connect(): void {
    if (this.socket?.connected) return;

    this.socket = io(WS_BASE, {
      transports: ['websocket', 'polling'],
      autoConnect: true,
    });

    this.socket.on('connect', () => {
      console.log('✅ WebSocket connected');
      this.emit('connected', true);
    });

    this.socket.on('disconnect', () => {
      console.log('❌ WebSocket disconnected');
      this.emit('connected', false);
    });

    this.socket.on('device_update', (data) => {
      console.log('📱 Device update received:', data);
      this.emit('device_update', data);
    });

    this.socket.on('command_response', (data) => {
      console.log('📨 Command response received:', data);
      this.emit('command_response', data);
    });

    this.socket.on('server_stats', (data) => {
      console.log('📊 Server stats received:', data);
      this.emit('server_stats', data);
    });
  }

  disconnect(): void {
    if (this.socket) {
      this.socket.disconnect();
      this.socket = null;
    }
  }

  on(event: string, callback: Function): void {
    if (!this.callbacks.has(event)) {
      this.callbacks.set(event, []);
    }
    this.callbacks.get(event)!.push(callback);
  }

  off(event: string, callback?: Function): void {
    if (!callback) {
      this.callbacks.delete(event);
      return;
    }
    
    const callbacks = this.callbacks.get(event);
    if (callbacks) {
      const index = callbacks.indexOf(callback);
      if (index !== -1) {
        callbacks.splice(index, 1);
      }
    }
  }

  private emit(event: string, data: any): void {
    const callbacks = this.callbacks.get(event);
    if (callbacks) {
      callbacks.forEach(callback => callback(data));
    }
  }
}

// Create singleton WebSocket service
export const wsService = new WebSocketService();

// API functions
export const tricorderAPI = {
  // Device management
  async getDevices(): Promise<TricorderDevice[]> {
    try {
      const response = await api.get('/devices');
      return response.data;
    } catch (error) {
      console.error('Error fetching devices:', error);
      throw error;
    }
  },

  async getServerInfo(): Promise<ServerInfo> {
    try {
      const response = await api.get('/server_info');
      return response.data;
    } catch (error) {
      console.error('Error fetching server info:', error);
      throw error;
    }
  },

  async startDiscovery(): Promise<{ success: boolean; message: string }> {
    try {
      const response = await api.post('/discovery');
      return response.data;
    } catch (error) {
      console.error('Error starting discovery:', error);
      throw error;
    }
  },

  async addDevice(deviceData: Partial<TricorderDevice>): Promise<{ success: boolean; message: string }> {
    try {
      const response = await api.post('/add_device', deviceData);
      return response.data;
    } catch (error) {
      console.error('Error adding device:', error);
      throw error;
    }
  },

  // Command functions
  async sendCommand(command: CommandRequest): Promise<CommandResponse> {
    try {
      const response = await api.post('/command', command);
      return response.data;
    } catch (error) {
      console.error('Error sending command:', error);
      throw error;
    }
  },

  // Specific device commands
  async setLEDColor(deviceId: string, color: LEDParameters): Promise<CommandResponse> {
    return this.sendCommand({
      device_id: deviceId,
      action: 'set_led_color',
      parameters: color,
    });
  },

  async setLEDBrightness(deviceId: string, brightness: BrightnessParameters): Promise<CommandResponse> {
    return this.sendCommand({
      device_id: deviceId,
      action: 'set_led_brightness',
      parameters: brightness,
    });
  },

  async setBuiltinLED(deviceId: string, color: LEDParameters): Promise<CommandResponse> {
    return this.sendCommand({
      device_id: deviceId,
      action: 'set_builtin_led',
      parameters: color,
    });
  },

  async setIndividualLED(deviceId: string, params: IndividualLEDParameters): Promise<CommandResponse> {
    return this.sendCommand({
      device_id: deviceId,
      action: 'set_individual_led',
      parameters: params,
    });
  },

  async scannerEffect(deviceId: string, params: LEDEffectParameters): Promise<CommandResponse> {
    return this.sendCommand({
      device_id: deviceId,
      action: 'scanner_effect',
      parameters: params,
    });
  },

  async pulseEffect(deviceId: string, params: LEDEffectParameters): Promise<CommandResponse> {
    return this.sendCommand({
      device_id: deviceId,
      action: 'pulse_effect',
      parameters: params,
    });
  },

  async playVideo(deviceId: string, params: VideoParameters): Promise<CommandResponse> {
    return this.sendCommand({
      device_id: deviceId,
      action: 'play_video',
      parameters: params,
    });
  },

  async stopVideo(deviceId: string): Promise<CommandResponse> {
    return this.sendCommand({
      device_id: deviceId,
      action: 'stop_video',
    });
  },

  async listVideos(deviceId: string): Promise<CommandResponse> {
    return this.sendCommand({
      device_id: deviceId,
      action: 'list_videos',
    });
  },

  async displayImage(deviceId: string, filename: string): Promise<CommandResponse> {
    return this.sendCommand({
      device_id: deviceId,
      action: 'display_image',
      parameters: { filename },
    });
  },

  async displayBootScreen(deviceId: string): Promise<CommandResponse> {
    return this.sendCommand({
      device_id: deviceId,
      action: 'display_boot_screen',
    });
  },

  async getDeviceStatus(deviceId: string): Promise<CommandResponse> {
    return this.sendCommand({
      device_id: deviceId,
      action: 'status',
    });
  },

  async pingDevice(deviceId: string): Promise<CommandResponse> {
    return this.sendCommand({
      device_id: deviceId,
      action: 'ping',
    });
  },

  // Bulk operations for multiple devices
  async sendBulkCommand(deviceIds: string[], command: Omit<CommandRequest, 'device_id'>): Promise<CommandResponse[]> {
    const promises = deviceIds.map(deviceId => 
      this.sendCommand({ ...command, device_id: deviceId })
    );
    
    try {
      return await Promise.all(promises);
    } catch (error) {
      console.error('Error sending bulk command:', error);
      throw error;
    }
  },

  async setBulkLEDColor(deviceIds: string[], color: LEDParameters): Promise<CommandResponse[]> {
    return this.sendBulkCommand(deviceIds, {
      action: 'set_led_color',
      parameters: color,
    });
  },

  async setBulkLEDBrightness(deviceIds: string[], brightness: BrightnessParameters): Promise<CommandResponse[]> {
    return this.sendBulkCommand(deviceIds, {
      action: 'set_led_brightness',
      parameters: brightness,
    });
  },

  async setBulkIndividualLED(deviceIds: string[], params: IndividualLEDParameters): Promise<CommandResponse[]> {
    return this.sendBulkCommand(deviceIds, {
      action: 'set_individual_led',
      parameters: params,
    });
  },

  async setBulkScannerEffect(deviceIds: string[], params: LEDEffectParameters): Promise<CommandResponse[]> {
    return this.sendBulkCommand(deviceIds, {
      action: 'scanner_effect',
      parameters: params,
    });
  },

  async setBulkPulseEffect(deviceIds: string[], params: LEDEffectParameters): Promise<CommandResponse[]> {
    return this.sendBulkCommand(deviceIds, {
      action: 'pulse_effect',
      parameters: params,
    });
  },

  async playBulkVideo(deviceIds: string[], params: VideoParameters): Promise<CommandResponse[]> {
    return this.sendBulkCommand(deviceIds, {
      action: 'play_video',
      parameters: params,
    });
  },

  async stopBulkVideo(deviceIds: string[]): Promise<CommandResponse[]> {
    return this.sendBulkCommand(deviceIds, {
      action: 'stop_video',
    });
  },
};

// Initialize WebSocket connection on service creation
wsService.connect();

export default tricorderAPI;
