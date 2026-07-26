import { mockUnits, mockObjects } from './mock-data';

describe('Mock Data', () => {
  test('should have correct structure for mockUnits', () => {
    expect(mockUnits).toBeDefined();
    expect(Array.isArray(mockUnits)).toBe(true);
    expect(mockUnits.length).toBeGreaterThan(0);

    // Check the structure of the first unit
    const firstUnit = mockUnits[0];
    expect(firstUnit).toHaveProperty('id');
    expect(firstUnit).toHaveProperty('name');
    expect(firstUnit).toHaveProperty('description');
    expect(firstUnit).toHaveProperty('objects');
    expect(firstUnit).toHaveProperty('rules');
    expect(Array.isArray(firstUnit.objects)).toBe(true);
    expect(firstUnit.objects.length).toBeGreaterThan(0);
  });

  test('should have correct structure for mockObjects', () => {
    expect(mockObjects).toBeDefined();
    expect(Array.isArray(mockObjects)).toBe(true);
    expect(mockObjects.length).toBeGreaterThan(0);

    // Check the structure of the first object
    const firstObject = mockObjects[0];
    expect(firstObject).toHaveProperty('id');
    expect(firstObject).toHaveProperty('name');
    expect(firstObject).toHaveProperty('type');
    expect(firstObject).toHaveProperty('spec');
    expect(Array.isArray(firstObject.spec)).toBe(true);
    expect(firstObject.spec.length).toBeGreaterThan(0);

    const firstSpec = firstObject.spec[0];
    expect(firstSpec).toHaveProperty('key');
    expect(firstSpec).toHaveProperty('value');
    expect(firstSpec).toHaveProperty('spec');
    expect(firstSpec.spec).toHaveProperty('model');
    expect(firstSpec.spec).toHaveProperty('unit');
  });
});
