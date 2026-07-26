import { mockUnits, mockObjects } from './mock-data';

// Mock API service
export const mockApi = {
  getUnitsList: async (): Promise<{ units: typeof mockUnits }> => {
    // Simulate network delay
    await new Promise(resolve => setTimeout(resolve, 500));
    
    // Return mock units data
    return { units: mockUnits };
  },

  getSensorsList: async (id: string): Promise<{ objects: typeof mockObjects }> => {
    // Simulate network delay
    await new Promise(resolve => setTimeout(resolve, 500));
    
    // In a real scenario, we might filter objects based on the unit ID
    // For now, we'll return all mock objects
    return { objects: mockObjects };
  },

  getAutomationsList: async (id: string): Promise<{ objects: typeof mockObjects }> => {
    // Simulate network delay
    await new Promise(resolve => setTimeout(resolve, 500));
    
    // In a real scenario, we might filter objects based on the unit ID
    // For now, we'll return all mock objects
    return { objects: mockObjects };
  },

  callCommand: async (deviceId: string, value: string): Promise<{ success: boolean }> => {
    await new Promise(resolve => setTimeout(resolve, 200));
    return { success: true };
  },
};

// Function to check if we're in mock mode
export const isMockMode = (): boolean => {
  return process.env.REACT_APP_MOCK_MODE === 'true';
};