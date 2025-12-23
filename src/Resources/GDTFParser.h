#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "pugixml.hpp"

namespace GDTF {

/**
 * @struct GeometryNode
 * @brief Represents a node in the GDTF geometry hierarchy.
 */
struct GeometryNode {
    std::string name;
    std::string type; // Base, Yoke, Head, Beam, etc.
    std::string model; // Reference to 3D model
    DirectX::XMFLOAT3 position = {0,0,0};
    DirectX::XMFLOAT3 rotation = {0,0,0}; // Euler angles
    std::vector<std::shared_ptr<GeometryNode>> children;
};

/**
 * @struct DMXChannel
 * @brief Basic DMX channel info.
 */
struct DMXChannel {
    std::string name;
    int offset; // 0-based
    int byteCount; // 1, 2, or 3 (8, 16, 24-bit)
    float defaultValue;
};

/**
 * @class GDTFParser
 * @brief Handles unzipping and XML parsing of GDTF archives.
 */
class GDTFParser {
public:
    GDTFParser() = default;
    ~GDTFParser() = default;

    /**
     * @brief Loads and parses a GDTF file.
     * @param fileName Path to the .gdtf file.
     * @return true if successful.
     */
    bool Load(const std::string& fileName);

    /**
     * @brief Extracts a file from the GDTF archive into memory.
     * @param internalPath Path inside the ZIP.
     * @param outData Vector to store the data.
     * @return true if successful.
     */
    bool ExtractFile(const std::string& internalPath, std::vector<uint8_t>& outData);

    /**
     * @brief Gets the fixture type name.
     */
    const std::string& GetFixtureTypeName() const { return m_fixtureTypeName; }

    /**
     * @brief Gets the root of the geometry hierarchy.
     */
    std::shared_ptr<GeometryNode> GetGeometryRoot() const { return m_geometryRoot; }

    /**
     * @brief Gets DMX channels for a specific mode (first mode by default).
     */
    const std::vector<DMXChannel>& GetDMXChannels() const { return m_dmxChannels; }

private:
    bool ParseXML(const std::string& xmlContent);
    std::shared_ptr<GeometryNode> ParseGeometry(pugi::xml_node node);
    
    std::string m_gdtfPath;
    std::string m_fixtureTypeName;
    std::shared_ptr<GeometryNode> m_geometryRoot;
    std::vector<DMXChannel> m_dmxChannels;
    
    // Internal XML document
    pugi::xml_document m_doc;
};

} // namespace GDTF
