#include "GDTFParser.h"
#include "miniz.h"
#include <iostream>
#include <DirectXMath.h>

namespace GDTF {

bool GDTFParser::Load(const std::string& fileName) {
    m_gdtfPath = fileName;
    
    std::vector<uint8_t> xmlData;
    if (!ExtractFile("description.xml", xmlData)) {
        std::cerr << "Failed to extract description.xml from " << fileName << std::endl;
        return false;
    }

    std::string xmlContent(xmlData.begin(), xmlData.end());
    return ParseXML(xmlContent);
}

bool GDTFParser::ExtractFile(const std::string& internalPath, std::vector<uint8_t>& outData) {
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (!mz_zip_reader_init_file(&zip_archive, m_gdtfPath.c_str(), 0)) {
        return false;
    }

    int file_index = mz_zip_reader_locate_file(&zip_archive, internalPath.c_str(), nullptr, 0);
    if (file_index < 0) {
        mz_zip_reader_end(&zip_archive);
        return false;
    }

    mz_zip_archive_file_stat file_stat;
    if (!mz_zip_reader_file_stat(&zip_archive, file_index, &file_stat)) {
        mz_zip_reader_end(&zip_archive);
        return false;
    }

    outData.resize((size_t)file_stat.m_uncomp_size);
    if (!mz_zip_reader_extract_to_mem(&zip_archive, file_index, outData.data(), outData.size(), 0)) {
        mz_zip_reader_end(&zip_archive);
        return false;
    }

    mz_zip_reader_end(&zip_archive);
    return true;
}

bool GDTFParser::ParseXML(const std::string& xmlContent) {
    pugi::xml_parse_result result = m_doc.load_string(xmlContent.c_str());
    if (!result) {
        return false;
    }

    pugi::xml_node fixtureType = m_doc.child("GDTF").child("FixtureType");
    m_fixtureTypeName = fixtureType.attribute("Name").as_string();

    // Parse Geometries
    pugi::xml_node geometries = fixtureType.child("Geometries");
    // Find the primary root (usually the first child of Geometries)
    for (pugi::xml_node child : geometries.children()) {
        m_geometryRoot = ParseGeometry(child);
        if (m_geometryRoot) break;
    }

    // Parse DMX Modes (take the first one for simplicity)
    pugi::xml_node dmxMode = fixtureType.child("DMXModes").child("DMXMode");
    if (dmxMode) {
        pugi::xml_node channels = dmxMode.child("DMXChannels");
        int offset = 0;
        for (pugi::xml_node chan : channels.children("DMXChannel")) {
            DMXChannel dc;
            dc.name = chan.attribute("Geometry").as_string();
            if (dc.name.empty()) dc.name = chan.attribute("Attribute").as_string();
            
            // DMX offset
            dc.offset = offset;
            
            // Byte count from coarse/fine/etc.
            dc.byteCount = 0;
            for (pugi::xml_node logical : chan.children("LogicalChannel")) {
                dc.byteCount++;
            }
            if (dc.byteCount == 0) dc.byteCount = 1;

            dc.defaultValue = chan.attribute("Default").as_float();
            m_dmxChannels.push_back(dc);
            
            offset += dc.byteCount;
        }
    }
    
    return true;
}

std::shared_ptr<GeometryNode> GDTFParser::ParseGeometry(pugi::xml_node node) {
    std::string type = node.name();
    if (type == "Geometry" || type == "Axis" || type == "Beam") {
        auto gn = std::make_shared<GeometryNode>();
        gn->name = node.attribute("Name").as_string();
        gn->type = type;
        gn->model = node.attribute("Model").as_string();

        // Matrix parsing (GDTF stores 4x4 matrices as space-separated floats)
        // For simplicity, we'll just extract position for now
        // This would need a full matrix parser for real-world GDTF
        
        for (pugi::xml_node child : node.children()) {
            auto childNode = ParseGeometry(child);
            if (childNode) {
                gn->children.push_back(childNode);
            }
        }
        return gn;
    }
    return nullptr;
}

} // namespace GDTF
