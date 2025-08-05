import { useState, useEffect, useCallback } from 'react';
import { tricorderAPI, wsService, TricorderDevice, ServerInfo } from '../services/tricorderAPI';

export interface UseTricorderFarmResult {
  // State
  devices: TricorderDevice[];
  serverInfo: ServerInfo | null;
  loading: boolean;
  error: string | null;
  connected: boolean;
  selectedDevices: Set<string>;
  
  // Actions
  refreshDevices: () => Promise<void>;
  startDiscovery: () => Promise<void>;
  selectDevice: (deviceId: string) => void;
  selectAllDevices: () => void;
  clearSelection: () => void;
  toggleDeviceSelection: (deviceId: string) => void;
  
  // Device commands
  setLEDColor: (deviceIds: string[], r: number, g: number, b: number) => Promise<void>;
  setLEDBrightness: (deviceIds: string[], brightness: number) => Promise<void>;
  setBuiltinLED: (deviceIds: string[], r: number, g: number, b: number) => Promise<void>;
  playVideo: (deviceIds: string[], filename: string, loop?: boolean) => Promise<void>;
  stopVideo: (deviceIds: string[]) => Promise<void>;
  displayImage: (deviceIds: string[], filename: string) => Promise<void>;
  displayBootScreen: (deviceIds: string[]) => Promise<void>;
  pingDevices: (deviceIds: string[]) => Promise<void>;
  getDeviceStatus: (deviceIds: string[]) => Promise<void>;
}

export const useTricorderFarm = (): UseTricorderFarmResult => {
  const [devices, setDevices] = useState<TricorderDevice[]>([]);
  const [serverInfo, setServerInfo] = useState<ServerInfo | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [connected, setConnected] = useState(false);
  const [selectedDevices, setSelectedDevices] = useState<Set<string>>(new Set());

  // WebSocket event handlers
  useEffect(() => {
    const handleConnection = (isConnected: boolean) => {
      setConnected(isConnected);
      if (isConnected) {
        refreshDevices();
        refreshServerInfo();
      }
    };

    const handleDeviceUpdate = (updatedDevice: TricorderDevice) => {
      setDevices(prev => {
        const index = prev.findIndex(d => d.device_id === updatedDevice.device_id);
        if (index !== -1) {
          const newDevices = [...prev];
          newDevices[index] = { ...newDevices[index], ...updatedDevice };
          return newDevices;
        } else {
          return [...prev, updatedDevice];
        }
      });
    };

    const handleCommandResponse = (response: any) => {
      console.log('Command response:', response);
      // Could show notifications or update UI based on command responses
    };

    const handleServerStats = (stats: any) => {
      if (stats.device_count !== undefined) {
        setServerInfo(prev => prev ? { ...prev, device_count: stats.device_count } : null);
      }
    };

    wsService.on('connected', handleConnection);
    wsService.on('device_update', handleDeviceUpdate);
    wsService.on('command_response', handleCommandResponse);
    wsService.on('server_stats', handleServerStats);

    return () => {
      wsService.off('connected', handleConnection);
      wsService.off('device_update', handleDeviceUpdate);
      wsService.off('command_response', handleCommandResponse);
      wsService.off('server_stats', handleServerStats);
    };
  }, []);

  // Fetch devices from API
  const refreshDevices = useCallback(async () => {
    try {
      setLoading(true);
      setError(null);
      const fetchedDevices = await tricorderAPI.getDevices();
      setDevices(fetchedDevices);
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Failed to fetch devices';
      setError(errorMessage);
      console.error('Error fetching devices:', err);
    } finally {
      setLoading(false);
    }
  }, []);

  // Fetch server info
  const refreshServerInfo = useCallback(async () => {
    try {
      const info = await tricorderAPI.getServerInfo();
      setServerInfo(info);
    } catch (err) {
      console.error('Error fetching server info:', err);
    }
  }, []);

  // Start device discovery
  const startDiscovery = useCallback(async () => {
    try {
      setLoading(true);
      setError(null);
      const result = await tricorderAPI.startDiscovery();
      console.log('Discovery result:', result);
      // Refresh devices after discovery
      setTimeout(refreshDevices, 2000);
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Failed to start discovery';
      setError(errorMessage);
      console.error('Error starting discovery:', err);
    } finally {
      setLoading(false);
    }
  }, [refreshDevices]);

  // Device selection management
  const selectDevice = useCallback((deviceId: string) => {
    setSelectedDevices(new Set([deviceId]));
  }, []);

  const selectAllDevices = useCallback(() => {
    setSelectedDevices(new Set(devices.map(d => d.device_id)));
  }, [devices]);

  const clearSelection = useCallback(() => {
    setSelectedDevices(new Set());
  }, []);

  const toggleDeviceSelection = useCallback((deviceId: string) => {
    setSelectedDevices(prev => {
      const newSet = new Set(prev);
      if (newSet.has(deviceId)) {
        newSet.delete(deviceId);
      } else {
        newSet.add(deviceId);
      }
      return newSet;
    });
  }, []);

  // Command execution helpers
  const executeCommand = useCallback(async (
    deviceIds: string[],
    commandFn: (deviceId: string) => Promise<any>,
    actionName: string
  ) => {
    if (deviceIds.length === 0) return;

    try {
      setError(null);
      
      if (deviceIds.length === 1) {
        await commandFn(deviceIds[0]);
      } else {
        // For multiple devices, execute in parallel
        await Promise.all(deviceIds.map(commandFn));
      }
      
      console.log(`${actionName} executed for devices:`, deviceIds);
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : `Failed to execute ${actionName}`;
      setError(errorMessage);
      console.error(`Error executing ${actionName}:`, err);
      throw err;
    }
  }, []);

  // Device command functions
  const setLEDColor = useCallback(async (deviceIds: string[], r: number, g: number, b: number) => {
    await executeCommand(
      deviceIds,
      (deviceId) => tricorderAPI.setLEDColor(deviceId, { r, g, b }),
      'LED Color'
    );
  }, [executeCommand]);

  const setLEDBrightness = useCallback(async (deviceIds: string[], brightness: number) => {
    await executeCommand(
      deviceIds,
      (deviceId) => tricorderAPI.setLEDBrightness(deviceId, { brightness }),
      'LED Brightness'
    );
  }, [executeCommand]);

  const setBuiltinLED = useCallback(async (deviceIds: string[], r: number, g: number, b: number) => {
    await executeCommand(
      deviceIds,
      (deviceId) => tricorderAPI.setBuiltinLED(deviceId, { r, g, b }),
      'Builtin LED'
    );
  }, [executeCommand]);

  const playVideo = useCallback(async (deviceIds: string[], filename: string, loop = false) => {
    await executeCommand(
      deviceIds,
      (deviceId) => tricorderAPI.playVideo(deviceId, { filename, loop }),
      'Play Video'
    );
  }, [executeCommand]);

  const stopVideo = useCallback(async (deviceIds: string[]) => {
    await executeCommand(
      deviceIds,
      (deviceId) => tricorderAPI.stopVideo(deviceId),
      'Stop Video'
    );
  }, [executeCommand]);

  const displayImage = useCallback(async (deviceIds: string[], filename: string) => {
    await executeCommand(
      deviceIds,
      (deviceId) => tricorderAPI.displayImage(deviceId, filename),
      'Display Image'
    );
  }, [executeCommand]);

  const displayBootScreen = useCallback(async (deviceIds: string[]) => {
    await executeCommand(
      deviceIds,
      (deviceId) => tricorderAPI.displayBootScreen(deviceId),
      'Display Boot Screen'
    );
  }, [executeCommand]);

  const pingDevices = useCallback(async (deviceIds: string[]) => {
    await executeCommand(
      deviceIds,
      (deviceId) => tricorderAPI.pingDevice(deviceId),
      'Ping Device'
    );
  }, [executeCommand]);

  const getDeviceStatus = useCallback(async (deviceIds: string[]) => {
    await executeCommand(
      deviceIds,
      (deviceId) => tricorderAPI.getDeviceStatus(deviceId),
      'Get Status'
    );
  }, [executeCommand]);

  // Initial data load
  useEffect(() => {
    refreshDevices();
    refreshServerInfo();
  }, [refreshDevices, refreshServerInfo]);

  return {
    // State
    devices,
    serverInfo,
    loading,
    error,
    connected,
    selectedDevices,
    
    // Actions
    refreshDevices,
    startDiscovery,
    selectDevice,
    selectAllDevices,
    clearSelection,
    toggleDeviceSelection,
    
    // Commands
    setLEDColor,
    setLEDBrightness,
    setBuiltinLED,
    playVideo,
    stopVideo,
    displayImage,
    displayBootScreen,
    pingDevices,
    getDeviceStatus,
  };
};

export default useTricorderFarm;
