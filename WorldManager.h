#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cmath>

#ifdef WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>

// ============================================================================
// CORE MATH VECTOR STRUCT
// ============================================================================
struct WorldVector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    float Distance(const WorldVector3& o) const {
        return std::sqrt((x - o.x) * (x - o.x) + (y - o.y) * (y - o.y) + (z - o.z) * (z - o.z));
    }
    
    WorldVector3 Normalize() const {
        float len = std::sqrt(x * x + y * y + z * z);
        if (len > 0.0f) return { x / len, y / len, z / len };
        return { 0.0f, 0.0f, 0.0f };
    }
};

// ============================================================================
// MAP ASSET REFERENCE STRUCTS
// ============================================================================

// Individual physical 3D asset placed anywhere across the NYC grid
struct MapObjectInstance {
    std::string modelMeshPath; // Path to .obj/.fbx mesh file
    int objectId;              // Unique engine lookup tag
    WorldVector3 position;     // 3D coordinates in meters
    WorldVector3 rotation;     // Pitch, Yaw, Roll angles
    WorldVector3 scale;        // Height/Width sizing scale multipliers
    bool isStaticGeometry;     // True for skyscrapers, False for breakable props
};

// Represents a 1km x 1km streaming grid sector cell
struct MapChunkSector {
    int chunkX = 0;
    int chunkZ = 0;
    std::vector<MapObjectInstance> sectorObjects;
    bool isCurrentlyLoadedInGPU = false;
};

// ============================================================================
// MAIN WORLD MANAGER ENGINE CLASS
// ============================================================================
class WorldManager {
public:
    WorldManager();
    ~WorldManager() = default;

    // Configures the master GPS anchor coordinate pivot (Default: Empire State Building)
    void SetMapOriginGPS(float lat, float lon);

    // Deep parsing framework sweeping the raw OSM XML New York City file datasets
    bool ParseNewYorkCityData(const std::string& filePath);

    // Grid Allocator: Manages memory streaming budgets around the player's 3D footprint
    void UpdateStreamingGrid(WorldVector3 playerPos, float viewRadius);

    // Low-Level Hardware Render execution stream passing culled items to Vulkan
    void RenderSectorsToVulkan(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, 
                               WorldVector3 cameraPos, WorldVector3 cameraLookDir, float maxViewRadius);

    // Exposes current structural grid references for engine debug inspection loops
    const std::map<std::pair<int, int>, MapChunkSector>& GetWorldGridSectors() const { return mWorldGridSectors; }

private:
    // Core Geographic Coordinate Mercator transformation formulas
    WorldVector3 ConvertGPSWithMath(float lat, float lon);
    float CalculateMountainElevation(float x, float z);
    
    // Spatial grid indexing function (Resolves 3D meters to its specific 1km cell ID pair)
    std::pair<int, int> GetChunkCoordinates(const WorldVector3& position);

    // Data-to-Asset Translation lookup registry maps
    std::string ResolveModelPathFromOSM(const std::string& key, const std::string& val, float heightFactor);
    
    // Core procedural generator pipeline extensions
    void PlaceStaticAssetOnMap(const std::string& assetMeshPath, WorldVector3 position, float rotationY, WorldVector3 scale);
    void ProcedurallyGenerateRoadSegments(const std::vector<WorldVector3>& wayPoints, const std::string& highwayType);
    
    // XML Document parsing helper string manipulation utilities
    std::string ExtractXMLAttribute(const std::string& line, const std::string& attributeName);

private:
    // NYC Global Origin Lock Anchors
    float mOriginLat;
    float mOriginLon;
    const float mChunkSizeMeters = 1000.0f; // Structural bounds length scale of each streamable block

    // Sector Grid Maps managing memory caches of the 5 Boroughs
    std::map<std::pair<int, int>, MapChunkSector> mWorldGridSectors;
    std::vector<std::pair<int, int>> mActiveSectorsThisFrame;
};
