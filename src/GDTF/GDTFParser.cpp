#include "GDTFParser.h"
#include <iostream>
#include <miniz/miniz.h>
#include <sstream>
#include <algorithm>
#include <fstream>

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
    // std::ofstream log("debug.log", std::ios::app);
    // log << "Extracting: " << internal_path << std::endl;

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (!mz_zip_reader_init_file(&zip_archive, gdtf_path_.c_str(), 0))
    {
        return false;
    }

    int file_index = mz_zip_reader_locate_file(&zip_archive, internal_path.c_str(), nullptr, 0);
    if (file_index < 0)
    {
        // Try to search case-insensitive if exact match fails
        int num_files = (int)mz_zip_reader_get_num_files(&zip_archive);
        for (int i = 0; i < num_files; i++)
        {
             mz_zip_archive_file_stat file_stat;
             if (mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
             {
                 std::string fname = file_stat.m_filename;
                 std::string target = internal_path;
                 
                 // Simple lowercase compare
                 std::transform(fname.begin(), fname.end(), fname.begin(), ::tolower);
                 std::transform(target.begin(), target.end(), target.begin(), ::tolower);
                 
                 if (fname == target) {
                     file_index = i;
                     break;
                 }
             }
        }
    }

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

    // Parse Models
    model_to_file_.clear();
    pugi::xml_node models = fixture_type.child("Models");
    for (pugi::xml_node model : models.children("Model"))
    {
        std::string name = model.attribute("Name").as_string();
        std::string file = model.attribute("File").as_string();
        if (!name.empty() && !file.empty())
        {
            model_to_file_[name] = file;
        }
    }

    // Parse Geometries
    pugi::xml_node geometries = fixture_type.child("Geometries");
    for (pugi::xml_node child : geometries.children())
    {
        geometry_root_ = parseGeometry(child);
        if (geometry_root_)
            break;
    }

    // Parse Wheels
    gobo_wheels_.clear();
    pugi::xml_node wheels_node = fixture_type.child("Wheels");
    for (pugi::xml_node wheel_node : wheels_node.children("Wheel"))
    {
        GoboWheel wheel;
        wheel.name = wheel_node.attribute("Name").as_string();

        for (pugi::xml_node slot_node : wheel_node.children("Slot"))
        {
            GoboSlot slot;
            slot.name = slot_node.attribute("Name").as_string();
            slot.media_file_name = slot_node.attribute("MediaFileName").as_string();
            
            wheel.slots.push_back(slot);
        }
        gobo_wheels_.push_back(wheel);
    }

    // Parse DMX Modes (take the first one)
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

            dc.offset = current_offset;
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
    if (type == "Geometry" || type == "Axis" || type == "Beam" || type == "Filter" || type == "ColorBeam")
    {
        auto gn = std::make_shared<GeometryNode>();
        gn->name = node.attribute("Name").as_string();
        gn->type = type;
        gn->model = node.attribute("Model").as_string();

        std::string matrix_str = node.attribute("Matrix").as_string();
        if (matrix_str.empty())
            matrix_str = node.attribute("Position").as_string();

        if (!matrix_str.empty())
        {
            std::string cleaned = matrix_str;
            std::replace(cleaned.begin(), cleaned.end(), '{', ' ');
            std::replace(cleaned.begin(), cleaned.end(), '}', ' ');
            std::replace(cleaned.begin(), cleaned.end(), ',', ' ');
            
            std::stringstream ss(cleaned);
            float m[16];
            for (int i = 0; i < 16; ++i) {
                if (!(ss >> m[i])) m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
            }

                                                            // GDTF is Row-Major with translation in 4th column.

                                                            // Transpose to move translation to 4th row for DirectX.

                                                            DirectX::XMFLOAT4X4 raw(m);

                                                            DirectX::XMMATRIX gdtfMat = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&raw));

                                                

                                                            // Apply translation inversion to match the physical model offsets

                                                            DirectX::XMFLOAT4X4 finalMat;

                                                            DirectX::XMStoreFloat4x4(&finalMat, gdtfMat);

                                                            finalMat._41 = -finalMat._41;

                                                            finalMat._42 = -finalMat._42;

                                                            finalMat._43 = -finalMat._43;

                                                

                                                            gn->matrix = finalMat;

                                                        }        for (pugi::xml_node child : node.children())
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

std::string GDTFParser::getModelFile(const std::string &model_name) const
{
    auto it = model_to_file_.find(model_name);
    if (it != model_to_file_.end())
    {
        return it->second;
    }
    return model_name;
}

} // namespace GDTF