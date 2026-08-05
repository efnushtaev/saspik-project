import { mockUnits, mockObjects, MockObject } from './mock-data';

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

  addObject: async (payload: {
    id: string;
    name: string;
    type: 'sensor' | 'device';
    spec: { key: string; model: string; unit?: string; minorPart?: number }[];
    description?: string;
    unitId: string;
  }): Promise<{ object: MockObject }> => {
    await new Promise(resolve => setTimeout(resolve, 300));
    const object: MockObject = {
      id: payload.id,
      name: payload.name,
      type: payload.type,
      spec: payload.spec.map(s => ({
        key: s.key,
        value: null,
        spec: {
          model: s.model,
          ...(s.unit ? { unit: s.unit } : {}),
          ...(s.minorPart !== undefined ? { minorPart: s.minorPart } : {}),
        },
      })),
      description: payload.description,
      topic: `${payload.type}/${payload.unitId}/${payload.id}`,
    };
    mockObjects.push(object);
    return { object };
  },

  getObject: async (id: string): Promise<{ object: MockObject }> => {
    await new Promise(resolve => setTimeout(resolve, 300));
    const object = mockObjects.find(o => o.id === id);
    if (!object) {
      throw new Error('Object not found');
    }
    return { object };
  },

  updateObject: async (
    id: string,
    payload: {
      name?: string;
      type?: 'sensor' | 'device';
      spec?: { key: string; model: string; unit?: string; minorPart?: number }[];
      description?: string;
      unitId?: string;
    },
  ): Promise<{ object: MockObject }> => {
    await new Promise(resolve => setTimeout(resolve, 300));
    const object = mockObjects.find(o => o.id === id);
    if (!object) {
      throw new Error('Object not found');
    }
    if (payload.name !== undefined) object.name = payload.name;
    if (payload.type !== undefined) object.type = payload.type;
    if (payload.description !== undefined) object.description = payload.description;
    if (payload.spec) {
      object.spec = payload.spec.map(s => ({
        key: s.key,
        value: object.spec.find(prev => prev.key === s.key)?.value ?? null,
        spec: {
          model: s.model,
          ...(s.unit ? { unit: s.unit } : {}),
          ...(s.minorPart !== undefined ? { minorPart: s.minorPart } : {}),
        },
      }));
    }
    object.topic = `${payload.type ?? object.type}/${payload.unitId ?? ''}/${id}`;
    return { object };
  },

  deleteObject: async (id: string): Promise<{ success: boolean }> => {
    await new Promise(resolve => setTimeout(resolve, 300));
    const index = mockObjects.findIndex(o => o.id === id);
    if (index === -1) {
      throw new Error('Object not found');
    }
    mockObjects.splice(index, 1);
    return { success: true };
  },
};

// Function to check if we're in mock mode
export const isMockMode = (): boolean => {
  return process.env.REACT_APP_MOCK_MODE === 'true';
};