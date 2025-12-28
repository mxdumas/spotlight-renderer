/**
 * @file GDTFParser.h
 * @brief GDTF archive extraction and XML parsing for fixture definitions.
 */

#pragma once

#include <DirectXMath.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "pugixml.hpp"

namespace GDTF
{

/**
 * @struct GeometryNode
 * @brief Represents a node in the GDTF geometry hierarchy.
 */
struct GeometryNode
{
    std::string name;                                    ///< Unique name of the geometry.
    std::string type;                                    ///< Type of geometry (Base, Yoke, Head, Beam, etc.).
    std::string model;                                   ///< Reference name to the 3D model (GLB).
    DirectX::XMFLOAT4X4 matrix;                          ///< Local transformation matrix.
    std::vector<std::shared_ptr<GeometryNode>> children; ///< Child nodes in the hierarchy.

    /// @brief Default constructor, initializes matrix to identity.
    GeometryNode()
    {
        DirectX::XMStoreFloat4x4(&matrix, DirectX::XMMatrixIdentity());
    }
};

/**
 * @struct DMXChannel
 * @brief Basic DMX channel information extracted from GDTF.
 */
struct DMXChannel
{
    std::string name;           ///< Attribute name (e.g., Pan, Tilt, Dimmer).
    int offset = 0;             ///< 0-based channel offset in the DMX universe.
    int byte_count = 1;         ///< Resolution (1 for 8-bit, 2 for 16-bit).
    float default_value = 0.0f; ///< Default DMX value (0.0 to 1.0).
};

/**
 * @struct GoboSlot
 * @brief Represents a single slot in a gobo wheel.
 */
struct GoboSlot
{
    std::string name;            ///< Name of the gobo slot.
    std::string media_file_name; ///< Path to the image file in the archive.
};

/**
 * @struct GoboWheel
 * @brief Represents a gobo wheel containing multiple slots.
 */
struct GoboWheel
{
    std::string name;            ///< Name of the wheel.
    std::vector<GoboSlot> slots; ///< List of slots on this wheel.
};

/**
 * @class GDTFParser
 * @brief Handles unzipping and XML parsing of GDTF (.gdtf) archives.
 *
 * This class uses miniz for decompression and pugixml for extracting fixture
 * definitions and geometry hierarchy.
 */
class GDTFParser
{
public:
    /**
     * @brief Default constructor.
     */
    GDTFParser() = default;

    /**
     * @brief Default destructor.
     */
    ~GDTFParser() = default;

    /**
     * @brief Loads and parses a GDTF file from the disk.
     *
     * @param fileName The absolute or relative path to the .gdtf file.
     * @return true if the file was opened and parsed successfully, false otherwise.
     */
    bool Load(const std::string &fileName);

    /**
     * @brief Extracts a specific file from the GDTF archive into memory.
     *
     * @param internalPath The path of the file inside the ZIP archive.
     * @param outData Vector to store the raw binary data of the extracted file.
     * @return true if the file exists and was extracted, false otherwise.
     */
    bool ExtractFile(const std::string &internalPath, std::vector<uint8_t> &outData);

    /**
     * @brief Gets the name of the fixture type defined in the GDTF.
     * @return A const reference to the fixture type name string.
     */
    [[nodiscard]] const std::string &GetFixtureTypeName() const
    {
        return m_fixtureTypeName;
    }

    /**
     * @brief Gets the root of the parsed geometry hierarchy.
     * @return A shared pointer to the root GeometryNode.
     */
    [[nodiscard]] std::shared_ptr<GeometryNode> GetGeometryRoot() const
    {
        return m_geometryRoot;
    }

    /**
     * @brief Gets the list of DMX channels for the primary mode.
     * @return A const reference to the vector of DMXChannel structures.
     */
    [[nodiscard]] const std::vector<DMXChannel> &GetDMXChannels() const
    {
        return m_dmxChannels;
    }

    /**
     * @brief Gets the list of Gobo Wheels.
     * @return A const reference to the vector of GoboWheel structures.
     */
    [[nodiscard]] const std::vector<GoboWheel> &GetGoboWheels() const
    {
        return m_goboWheels;
    }

    /**
     * @brief Extracts all gobo images from the GDTF archive.
     *
     * Iterates through all gobo wheels and extracts images for slots
     * that have a MediaFileName defined.
     *
     * @return Vector of raw image data (PNG/JPG bytes), one per gobo.
     */
    std::vector<std::vector<uint8_t>> ExtractGoboImages();

    /**
     * @brief Gets the actual file name for a model name.
     * @param modelName The name of the model in the geometry tree.
     * @return The file name (usually GLB), or the modelName if not found.
     */
    [[nodiscard]] std::string GetModelFile(const std::string &modelName) const;

private:
    /**
     * @brief Internal helper to parse the XML content of description.xml.
     * @param xmlContent The raw XML string.
     * @return true if parsing succeeded.
     */
    bool ParseXML(const std::string &xmlContent);

    /**
     * @brief Recursively parses geometry nodes from the XML.
     * @param node The current XML node in the Geometries section.
     * @return A shared pointer to the created GeometryNode, or nullptr if invalid.
     */
    static std::shared_ptr<GeometryNode> ParseGeometry(pugi::xml_node node);

    std::string m_gdtfPath;                           ///< Path to the source .gdtf archive.
    std::string m_fixtureTypeName;                    ///< Name extracted from the XML.
    std::shared_ptr<GeometryNode> m_geometryRoot;     ///< Root of the logical geometry tree.
    std::vector<DMXChannel> m_dmxChannels;            ///< List of DMX attributes.
    std::vector<GoboWheel> m_goboWheels;              ///< List of Gobo Wheels.
    std::map<std::string, std::string> m_modelToFile; ///< Mapping from model name to file name.

    pugi::xml_document m_doc; ///< Persistent XML document.
};

} // namespace GDTF
