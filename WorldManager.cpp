#include "WorldManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

// ============================================================================
// VULKAN GRAPHICS STRUCTS & STRUCTURAL ALIGNMENT
// ============================================================================

struct VulkanObjectUniformData {
    float modelMatrix[16]; 
    uint32_t textureIndex; 
    float padding[3];      
};

// Helper function to build a standard 4x4 column-major transformation matrix
void BuildMatrix4x4(float* outMatrix, WorldVector3 pos, WorldVector3 rot, WorldVector3 scale) {
    // Convert Euler degrees to radians
    float radX = rot.x * (3.14159265f / 180.0f);
    float radY = rot.y * (3.14159265f / 180.0f);
    float radZ = rot.z * (3.14159265f / 180.0f);

    float cx = std::cos(radX), sx = std::sin(radX);
    float cy = std::cos(radY), sy = std::sin(radY);
    float cz = std::cos(radZ), sz = std::sin(radZ);

    // Identity Initialization with scale factors applied
    outMatrix[0] = (cy * cz) * scale.x;
    outMatrix[1] = (cx * sz + sx * sy * cz) * scale.x;
    outMatrix[2] = (sx * sz - cx * sy * cz) * scale.x;
    outMatrix[3] = 0.0f;

    outMatrix[4] = (-cy * sz) * scale.y;
    outMatrix[5] = (cx * cz - sx * sy * sz) * scale.y;
    outMatrix[6] = (sx * cz + cx * sy * sz) * scale.y;
    outMatrix[7] = 0.0f;

    outMatrix[8] = sy * scale.z;
    outMatrix[9] = -sx * cy * scale.z;
    outMatrix[10] = cx * cy * scale.z;
    outMatrix[11] = 0.0f;

    outMatrix[12] = pos.x;
    outMatrix[13] = pos.y;
    outMatrix[14] = pos.z;
    outMatrix[15] = 1.0f;
}

// Mock loader systems representing asset registries inside your Vulkan setup
uint32_t GetTextureRegistryID(const std::string& path) {
    if (path.find("skyscraper") != std::string::npos) return 1;
    if (path.find("road") != std::string::npos) return 2;
    if (path.find("tree") != std::string::npos) return 3;
    return 0; // Default material index
}

// ============================================================================
// CORE CONSTRUCTOR AND SPACE CONVERSION MATHEMATICS
// ============================================================================

WorldManager::WorldManager() 
    : mOriginLat(40.7484f), mOriginLon(-73.9857f) { // Empire State Building Origin Anchor
}

void WorldManager::SetMapOriginGPS(float lat, float lon) {
    mOriginLat = lat;
    mOriginLon = lon;
}

WorldVector3 WorldManager::ConvertGPSWithMath(float lat, float lon) {
    WorldVector3 localPos;
    const float EarthRadius = 6371000.0f; 

    float latRad = lat * (3.14159265f / 180.0f);
    float lonRad = lon * (3.14159265f / 180.0f);
    float latOriginRad = mOriginLat * (3.14159265f / 180.0f);
    float lonOriginRad = mOriginLon * (3.14159265f / 180.0f);

    // Mercator mapping calculation loops mapping GPS coordinates to relative meters
    localPos.x = EarthRadius * (lonRad - lonOriginRad) * std::cos(latOriginRad);
    localPos.z = EarthRadius * (latRad - latOriginRad);
    localPos.y = CalculateMountainElevation(localPos.x, localPos.z);

    return localPos;
}

float WorldManager::CalculateMountainElevation(float x, float z) {
    float baseHeight = std::sin(x * 0.002f) * std::cos(z * 0.002f) * 15.0f; 
    return (baseHeight < 0.0f) ? 0.0f : baseHeight;
}

std::pair<int, int> WorldManager::GetChunkCoordinates(const WorldVector3& position) {
    int chunkX = static_cast<int>(std::floor(position.x / mChunkSizeMeters));
    int chunkZ = static_cast<int>(std::floor(position.z / mChunkSizeMeters));
    return { chunkX, chunkZ };
}

// ============================================================================
// PROCEDURAL ASSET ROUTING MATRIX
// ============================================================================

std::string WorldManager::ResolveModelPathFromOSM(const std::string& key, const std::string& val, float heightFactor) {
    if (key == "building") {
        if (val == "commercial" || val == "retail") return "assets/models/buildings/nyc_corner_deli.obj";
        if (val == "apartments" || val == "residential") {
            return (heightFactor > 15.0f) ? "assets/models/buildings/nyc_apartment_highrise.obj" 
                                          : "assets/models/buildings/nyc_brownstone_short.obj";
        }
        if (val == "office" || val == "skyscraper") {
            return (heightFactor > 50.0f) ? "assets/models/buildings/manhattan_tower_tall.obj" 
                                          : "assets/models/buildings/midrise_glass_office.obj";
        }
        return "assets/models/buildings/nyc_generic_block.obj";
    }
    if (key == "natural") {
        if (val == "tree" || val == "vegetation") return "assets/models/vegetation/nyc_oak_tree.obj";
    }
    if (key == "landuse" && (val == "grass" || val == "park")) {
        return "assets/models/terrain/grass_patch_tile.obj";
    }
    if (key == "natural" && val == "water") return "assets/models/terrain/nyc_water_plane.obj";

    return "";
}

void WorldManager::PlaceStaticAssetOnMap(const std::string& assetMeshPath, WorldVector3 position, float rotationY, WorldVector3 scale) {
    static int globalIdCounter = 1000;
    
    MapObjectInstance dynamicElement;
    dynamicElement.modelMeshPath = assetMeshPath;
    dynamicElement.objectId = globalIdCounter++;
    dynamicElement.position = position;
    dynamicElement.rotation = { 0.0f, rotationY, 0.0f };
    dynamicElement.scale = scale;
    dynamicElement.isStaticGeometry = true;

    auto sectorID = GetChunkCoordinates(position);
    mWorldGridSectors[sectorID].sectorObjects.push_back(dynamicElement);
}

void WorldManager::ProcedurallyGenerateRoadSegments(const std::vector<WorldVector3>& wayPoints, const std::string& highwayType) {
    if (wayPoints.size() < 2) return;

    for (size_t i = 0; i < wayPoints.size() - 1; ++i) {
        WorldVector3 p1 = wayPoints[i];
        WorldVector3 p2 = wayPoints[i + 1];

        float dx = p2.x - p1.x;
        float dz = p2.z - p1.z;
        float segmentLength = std::sqrt(dx * dx + dz * dz);
        if (segmentLength < 0.2f) continue;

        float headingYaw = std::atan2(dx, dz) * (180.0f / 3.14159265f);
        WorldVector3 midPoint = { p1.x + dx * 0.5f, p1.y, p1.z + dz * 0.5f };

        std::string roadMesh = "assets/models/roads/nyc_street_2lane.obj";
        float sidewalkWidthOffset = 4.0f;
        
        if (highwayType == "motorway" || highwayType == "trunk") {
            roadMesh = "assets/models/roads/highway_4lane.obj";
            sidewalkWidthOffset = 6.5f;
        }

        PlaceStaticAssetOnMap(roadMesh, midPoint, headingYaw, {1.0f, 1.0f, segmentLength});

        float nx = -dz / segmentLength; 
        float nz = dx / segmentLength;

        WorldVector3 leftSidewalkPos = { midPoint.x + (nx * sidewalkWidthOffset), midPoint.y, midPoint.z + (nz * sidewalkWidthOffset) };
        PlaceStaticAssetOnMap("assets/models/roads/sidewalk_concrete.obj", leftSidewalkPos, headingYaw, {1.0f, 1.0f, segmentLength});

        WorldVector3 rightSidewalkPos = { midPoint.x - (nx * sidewalkWidthOffset), midPoint.y, midPoint.z - (nz * sidewalkWidthOffset) };
        PlaceStaticAssetOnMap("assets/models/roads/sidewalk_concrete.obj", rightSidewalkPos, headingYaw, {1.0f, 1.0f, segmentLength});
    }
}

// ============================================================================
// GLOBAL NYC XML FILE STREAM PARSER ENGINE
// ============================================================================

bool WorldManager::ParseNewYorkCityData(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[CORE ERROR]: OpenStreetMap layout asset source footprint missing at: " << filePath << "\n";
        return false;
    }

    std::string line;
    std::map<std::string, WorldVector3> geoNodeCoordinates;
    std::vector<WorldVector3> activeWayPoints;
    std::vector<std::pair<std::string, std::string>> extractedTags;
    bool parsingWayContainer = false;
    float parsedBuildingHeightValue = 0.0f;

    std::cout << "[ENGINE PROCESSING]: Compiling geographic layers across the 5 boroughs...\n";

    while (std::getline(file, line)) {
        if (line.find("<node") != std::string::npos) {
            std::string id = ExtractXMLAttribute(line, "id");
            float lat = std::stof(ExtractXMLAttribute(line, "lat"));
            float lon = std::stof(ExtractXMLAttribute(line, "lon"));

            WorldVector3 convertedPos = ConvertGPSWithMath(lat, lon);
            geoNodeCoordinates[id] = convertedPos;

            size_t scanningCursor = 0;
            while ((scanningCursor = line.find("<tag k=", scanningCursor)) != std::string::npos) {
                std::string segment = line.substr(scanningCursor);
                std::string k = ExtractXMLAttribute(segment, "k");
                std::string v = ExtractXMLAttribute(segment, "v");
                scanningCursor += 7;

                std::string assetPath = ResolveModelPathFromOSM(k, v, 0.0f);
                if (!assetPath.empty()) {
                    float orientation = static_cast<float>(rand() % 360);
                    PlaceStaticAssetOnMap(assetPath, convertedPos, orientation, {1.0f, 1.0f, 1.0f});
                }
            }
        }
        else if (line.find("<way") != std::string::npos) {
            activeWayPoints.clear();
            extractedTags.clear();
            parsingWayContainer = true;
            parsedBuildingHeightValue = 0.0f;
        }
        else if (line.find("<nd ref=") != std::string::npos && parsingWayContainer) {
            std::string nodeRef = ExtractXMLAttribute(line, "ref");
            if (geoNodeCoordinates.find(nodeRef) != geoNodeCoordinates.end()) {
                activeWayPoints.push_back(geoNodeCoordinates[nodeRef]);
            }
        }
        else if (line.find("<tag k=") != std::string::npos && parsingWayContainer) {
            std::string k = ExtractXMLAttribute(line, "k");
            std::string v = ExtractXMLAttribute(line, "v");
            extractedTags.push_back({k, v});

            if (k == "height" || k == "building:levels") {
                parsedBuildingHeightValue = std::stof(v) * 3.5f; 
            }
        }
        else if (line.find("</way>") != std::string::npos) {
            parsingWayContainer = false;
            if (activeWayPoints.empty()) continue;

            std::string highwayCategoryType = "";
            bool areaIsProcessed = false;

            for (const auto& [key, value] : extractedTags) {
                if (key == "highway") {
                    highwayCategoryType = value;
                    break;
                }

                std::string resolvedMeshPath = ResolveModelPathFromOSM(key, value, parsedBuildingHeightValue);
                if (!resolvedMeshPath.empty()) {
                    if (key == "landuse" || value == "water" || value == "park") {
                        PlaceStaticAssetOnMap(resolvedMeshPath, activeWayPoints[0], 0.0f, {15.0f, 1.0f, 15.0f});
                    } 
                    else if (key == "building") {
                        float randomDirection = static_cast<float>((rand() % 4) * 90);
                        WorldVector3 scaleDimensions = {1.0f, 1.0f + (parsedBuildingHeightValue * 0.1f), 1.0f};
                        PlaceStaticAssetOnMap(resolvedMeshPath, activeWayPoints[0], randomDirection, scaleDimensions);
                    }
                    areaIsProcessed = true;
                    break;
                }
            }

            if (!highwayCategoryType.empty() && !areaIsProcessed) {
                ProcedurallyGenerateRoadSegments(activeWayPoints, highwayCategoryType);
            }
        }
    }
    return true;
}

// ============================================================================
// LOW-LEVEL VULKAN STREAMING AND HARDWARE PIPELINE HANDOFF RENDERING
// ============================================================================

void WorldManager::UpdateStreamingGrid(WorldVector3 playerPos, float viewRadius) {
    mActiveSectorsThisFrame.clear();
    auto playerChunk = GetChunkCoordinates(playerPos);
    int chunkRange = static_cast<int>(std::ceil(viewRadius / mChunkSizeMeters));

    for (int x = playerChunk.first - chunkRange; x <= playerChunk.first + chunkRange; ++x) {
        for (int z = playerChunk.second - chunkRange; z <= playerChunk.second + chunkRange; ++z) {
            std::pair<int, int> targetedSector = { x, z };
            if (mWorldGridSectors.find(targetedSector) != mWorldGridSectors.end()) {
                mActiveSectorsThisFrame.push_back(targetedSector);
                mWorldGridSectors[targetedSector].isCurrentlyLoadedInGPU = true;
            }
        }
    }
}

void WorldManager::RenderSectorsToVulkan(VkCommandBuffer cmdBuffer, VkPipelineLayout pipelineLayout, 
                                        WorldVector3 cameraPos, WorldVector3 cameraLookDir, float maxViewRadius) {
    
    // Iterate strictly over the chunks designated as loaded by the streaming thread frame window
    for (const auto& sectorCoords : mActiveSectorsThisFrame) {
        const auto& sector = mWorldGridSectors[sectorCoords];

        for (const auto& obj : sector.sectorObjects) {
            
            // --- DETAILED FRUSTUM OUT-OF-BOUNDS CULLING MATRIX MATH ---
            float distanceToCam = obj.position.Distance(cameraPos);
            if (distanceToCam > maxViewRadius) continue; 

            WorldVector3 toObjectVec = { obj.position.x - cameraPos.x, obj.position.y - cameraPos.y, obj.position.z - cameraPos.z };
            float dotProductResult = (toObjectVec.Normalize().x * cameraLookDir.x) + 
                                     (toObjectVec.Normalize().y * cameraLookDir.y) + 
                                     (toObjectVec.Normalize().z * cameraLookDir.z);
            if (dotProductResult < 0.35f) continue; // Behind camera lens horizon plane bounds, skip it

            // --- COMPUTING MEMORY PUSH FOR THE GPU BUFFER STREAMS ---
            VulkanObjectUniformData pushConstants;
            BuildMatrix4x4(pushConstants.modelMatrix, obj.position, obj.rotation, obj.scale);
            pushConstants.textureIndex = GetTextureRegistryID(obj.modelMeshPath);

            // Execute low-level Vulkan state command pipeline injection strings
            vkCmdPushConstants(
                cmdBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(VulkanObjectUniformData),
                &pushConstants
            );

            // Fetch structural pointers from your physical VMA/Hardware staging classes
            // VulkanMeshBuffer hardwareGeometry = EngineBufferCache::GetGPUTerrainBuffer(obj.modelMeshPath);
            // VkDeviceSize internalOffsets[] = { 0 };
            
            // vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &hardwareGeometry.vertexBuffer, internalOffsets);
            // vkCmdBindIndexBuffer(cmdBuffer, hardwareGeometry.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            // vkCmdDrawIndexed(cmdBuffer, hardwareGeometry.indexCount, 1, 0, 0, 0);
        }
    }
}

std::string WorldManager::ExtractXMLAttribute(const std::string& line, const std::string& attributeName) {
    std::string searchStr = attributeName + "=\"";
    size_t startPos = line.find(searchStr);
    if (startPos == std::string::npos) return "";
    startPos += searchStr.length();
    size_t endPos = line.find("\"", startPos);
    return line.substr(startPos, endPos - startPos);
}
