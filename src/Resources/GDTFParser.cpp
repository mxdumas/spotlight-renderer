#include "GDTFParser.h"
#include <iostream>
#include <miniz/miniz.h>

namespace GDTF
{

bool GDTFParser::load(const std::string &file_name)
{
    gdtf_path_ = file_name;

    std::vector<uint8_t> xml_data;
    if (!extractFile("description.xml", xml_data))
    {
        std::cerr << "Failed to extract description.xml from " << file_name << std::endl;
        return false;
    }

    std::string xml_content(xml_data.begin(), xml_data.end());
    return parseXML(xml_content);
}

bool GDTFParser::extractFile(const std::string &internal_path, std::vector<uint8_t> &out_data)
{
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (!mz_zip_reader_init_file(&zip_archive, gdtf_path_.c_str(), 0))
    {
        return false;
    }

    int file_index = mz_zip_reader_locate_file(&zip_archive, internal_path.c_str(), nullptr, 0);
    if (file_index < 0)
    {
        mz_zip_reader_end(&zip_archive);
        return false;
    }

    mz_zip_archive_file_stat file_stat;
    if (!mz_zip_reader_file_stat(&zip_archive, file_index, &file_stat))
    {
        mz_zip_reader_end(&zip_archive);
        return false;
    }

    out_data.resize((size_t)file_stat.m_uncomp_size);
    if (!mz_zip_reader_extract_to_mem(&zip_archive, file_index, out_data.data(), out_data.size(), 0))
    {
        mz_zip_reader_end(&zip_archive);
        return false;
    }

    mz_zip_reader_end(&zip_archive);
    return true;
}

bool GDTFParser::parseXML(const std::string &xml_content)
{
    pugi::xml_parse_result result = doc_.load_string(xml_content.c_str());
    if (!result)
    {
        return false;
    }

    pugi::xml_node fixture_type = doc_.child("GDTF").child("FixtureType");
    fixture_type_name_ = fixture_type.attribute("Name").as_string();

    // Parse Geometries
    pugi::xml_node geometries = fixture_type.child("Geometries");
    // Find the primary root (usually the first child of Geometries)
    for (pugi::xml_node child : geometries.children())
    {
        geometry_root_ = parseGeometry(child);
        if (geometry_root_)
            break;
    }

    // Parse DMX Modes (take the first one for simplicity)
    pugi::xml_node dmx_mode = fixture_type.child("DMXModes").child("DMXMode");
    if (dmx_mode)
    {
        pugi::xml_node channels = dmx_mode.child("DMXChannels");
        int current_offset = 0;
        for (pugi::xml_node chan : channels.children("DMXChannel"))
        {
            DMXChannel dc;
            dc.name = chan.attribute("Geometry").as_string();
            if (dc.name.empty())
                dc.name = chan.attribute("Attribute").as_string();

            // DMX offset
            dc.offset = current_offset;

            // Byte count from coarse/fine/etc.
            dc.byte_count = 0;
            for ([[maybe_unused]] pugi::xml_node logical : chan.children("LogicalChannel"))
            {
                dc.byte_count++;
            }
            if (dc.byte_count == 0)
                dc.byte_count = 1;

            dc.default_value = chan.attribute("Default").as_float();
            dmx_channels_.push_back(dc);

            current_offset += dc.byte_count;
        }
    }

    return true;
}

std::shared_ptr<GeometryNode> GDTFParser::parseGeometry(pugi::xml_node node)
{
    std::string type = node.name();
    if (type == "Geometry" || type == "Axis" || type == "Beam")
    {
        auto gn = std::make_shared<GeometryNode>();
        gn->name = node.attribute("Name").as_string();
        gn->type = type;
        gn->model = node.attribute("Model").as_string();

        // Matrix parsing (GDTF stores 4x4 matrices as space-separated floats)
        // For simplicity, we'll just extract position for now
        // This would need a full matrix parser for real-world GDTF

        for (pugi::xml_node child : node.children())
        {
            auto childNode = parseGeometry(child);
            if (childNode)
            {
                gn->children.push_back(childNode);
            }
        }
        return gn;
    }
    return nullptr;
}

} // namespace GDTF
