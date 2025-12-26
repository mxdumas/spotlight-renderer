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

    GeometryNode() {
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
    std::string name;          ///< Name of the wheel.
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
     * @param file_name The absolute or relative path to the .gdtf file.
     * @return true if the file was opened and parsed successfully, false otherwise.
     */
    bool load(const std::string &file_name);

    /**
     * @brief Extracts a specific file from the GDTF archive into memory.
     *
     * @param internal_path The path of the file inside the ZIP archive.
     * @param out_data Vector to store the raw binary data of the extracted file.
     * @return true if the file exists and was extracted, false otherwise.
     */
    bool extractFile(const std::string &internal_path, std::vector<uint8_t> &out_data);

    /**
     * @brief Gets the name of the fixture type defined in the GDTF.
     * @return A const reference to the fixture type name string.
     */
    const std::string &getFixtureTypeName() const
    {
        return fixture_type_name_;
    }

    /**
     * @brief Gets the root of the parsed geometry hierarchy.
     * @return A shared pointer to the root GeometryNode.
     */
    std::shared_ptr<GeometryNode> getGeometryRoot() const
    {
        return geometry_root_;
    }

    /**
     * @brief Gets the list of DMX channels for the primary mode.
     * @return A const reference to the vector of DMXChannel structures.
     */
    const std::vector<DMXChannel> &getDMXChannels() const
    {
        return dmx_channels_;
    }

    /**
     * @brief Gets the list of Gobo Wheels.
     * @return A const reference to the vector of GoboWheel structures.
     */
    const std::vector<GoboWheel> &getGoboWheels() const
    {
        return gobo_wheels_;
    }

    /**
     * @brief Extracts all gobo images from the GDTF archive.
     *
     * Iterates through all gobo wheels and extracts images for slots
     * that have a MediaFileName defined.
     *
     * @return Vector of raw image data (PNG/JPG bytes), one per gobo.
     */
    std::vector<std::vector<uint8_t>> extractGoboImages();

    /**
     * @brief Gets the actual file name for a model name.
     * @param model_name The name of the model in the geometry tree.
     * @return The file name (usually GLB), or the model_name if not found.
     */
    std::string getModelFile(const std::string &model_name) const;

private:
    /**
     * @brief Internal helper to parse the XML content of description.xml.
     * @param xml_content The raw XML string.
     * @return true if parsing succeeded.
     */
    bool parseXML(const std::string &xml_content);

    /**
     * @brief Recursively parses geometry nodes from the XML.
     * @param node The current XML node in the Geometries section.
     * @return A shared pointer to the created GeometryNode, or nullptr if invalid.
     */
    std::shared_ptr<GeometryNode> parseGeometry(pugi::xml_node node);

    std::string gdtf_path_;                       ///< Path to the source .gdtf archive.
    std::string fixture_type_name_;               ///< Name extracted from the XML.
    std::shared_ptr<GeometryNode> geometry_root_; ///< Root of the logical geometry tree.
    std::vector<DMXChannel> dmx_channels_;        ///< List of DMX attributes.
    std::vector<GoboWheel> gobo_wheels_;          ///< List of Gobo Wheels.
    std::map<std::string, std::string> model_to_file_; ///< Mapping from model name to file name.

    pugi::xml_document doc_; ///< Persistent XML document.
};

} // namespace GDTF
