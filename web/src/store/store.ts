import { configureStore } from '@reduxjs/toolkit';
import deviceSlice from './deviceSlice';

export const store = configureStore({
  reducer: {
    devices: deviceSlice,
  },
});

export type RootState = ReturnType<typeof store.getState>;
export type AppDispatch = typeof store.dispatch;
