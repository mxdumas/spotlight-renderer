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
    DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};     ///< Local relative position.
    DirectX::XMFLOAT3 rotation = {0.0f, 0.0f, 0.0f};     ///< Local relative rotation (Euler).
    std::vector<std::shared_ptr<GeometryNode>> children; ///< Child nodes in the hierarchy.
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

    pugi::xml_document doc_; ///< Persistent XML document.
};

} // namespace GDTF
