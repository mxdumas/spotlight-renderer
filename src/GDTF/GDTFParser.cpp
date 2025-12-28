#include "GDTFParser.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <miniz/miniz.h>
#include <sstream>

namespace GDTF
{

bool GDTFParser::Load(const std::string &file_name)
{
    m_gdtfPath = file_name;

    std::vector<uint8_t> xml_data;
    if (!ExtractFile("description.xml", xml_data))
    {
        std::cerr << "Failed to extract description.xml from " << file_name << std::endl;
        return false;
    }

    std::string xml_content(xml_data.begin(), xml_data.end());
    return ParseXML(xml_content);
}

bool GDTFParser::ExtractFile(const std::string &internal_path, std::vector<uint8_t> &out_data)
{
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (!mz_zip_reader_init_file(&zip_archive, m_gdtfPath.c_str(), 0))
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

                if (fname == target)
                {
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

bool GDTFParser::ParseXML(const std::string &xml_content)
{
    pugi::xml_parse_result result = m_doc.load_string(xml_content.c_str());
    if (!result)
    {
        return false;
    }

    pugi::xml_node fixture_type = m_doc.child("GDTF").child("FixtureType");
    m_fixtureTypeName = fixture_type.attribute("Name").as_string();

    // Parse Models
    m_modelToFile.clear();
    pugi::xml_node models = fixture_type.child("Models");
    for (pugi::xml_node model : models.children("Model"))
    {
        std::string name = model.attribute("Name").as_string();
        std::string file = model.attribute("File").as_string();
        if (!name.empty() && !file.empty())
        {
            m_modelToFile[name] = file;
        }
    }

    // Parse Geometries
    pugi::xml_node geometries = fixture_type.child("Geometries");
    for (pugi::xml_node child : geometries.children())
    {
        m_geometryRoot = ParseGeometry(child);
        if (m_geometryRoot)
            break;
    }

    // Parse Wheels
    m_goboWheels.clear();
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
        m_goboWheels.push_back(wheel);
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
            m_dmxChannels.push_back(dc);
            current_offset += dc.byte_count;
        }
    }

    return true;
}

std::shared_ptr<GeometryNode> GDTFParser::ParseGeometry(pugi::xml_node node)
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
            for (int i = 0; i < 16; ++i)
            {
                if (!(ss >> m[i]))
                    m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
            }

            // GDTF is Row-Major with translation in 4th column.
            // Transpose to move translation to 4th row for DirectX.
            DirectX::XMFLOAT4X4 raw(m);
            DirectX::XMMATRIX gdtf_mat = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&raw));

            // Apply translation inversion to match the physical model offsets
            DirectX::XMFLOAT4X4 final_mat;
            DirectX::XMStoreFloat4x4(&final_mat, gdtf_mat);
            final_mat._41 = -final_mat._41;
            final_mat._42 = -final_mat._42;
            final_mat._43 = -final_mat._43;
            gn->matrix = final_mat;
        }
        for (pugi::xml_node child : node.children())
        {
            auto child_node = ParseGeometry(child);
            if (child_node)
            {
                gn->children.push_back(child_node);
            }
        }
        return gn;
    }
    return nullptr;
}

std::string GDTFParser::GetModelFile(const std::string &model_name) const
{
    auto it = m_modelToFile.find(model_name);
    if (it != m_modelToFile.end())
    {
        return it->second;
    }
    return model_name;
}

std::vector<std::vector<uint8_t>> GDTFParser::ExtractGoboImages()
{
    std::vector<std::vector<uint8_t>> images;

    // First slot is always "Open" (radial gradient - bright center, soft falloff)
    // Create a simple TGA in memory
    {
        const int size = 512;
        std::vector<uint8_t> circle_data;
        // Simple uncompressed TGA format (easier than PNG)
        // TGA header (18 bytes)
        circle_data.resize(18 + size * size * 4);
        circle_data[2] = 2; // Uncompressed true-color
        circle_data[12] = size & 0xFF;
        circle_data[13] = (size >> 8) & 0xFF;
        circle_data[14] = size & 0xFF;
        circle_data[15] = (size >> 8) & 0xFF;
        circle_data[16] = 32;   // 32 bits per pixel
        circle_data[17] = 0x20; // Top-left origin
        // Radial gradient with hard edge cutoff (like real gobos)
        const float center = size / 2.0f;
        const float radius = center * 0.40f;       // Circle occupies 50% of diameter
        const float edge_softness = radius * 0.1f; // Soft edge zone
        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                float dx = x - center;
                float dy = y - center;
                float dist = std::sqrt(dx * dx + dy * dy);

                float brightness = 0.0f;
                if (dist < radius - edge_softness)
                {
                    // Inside: gradient from 100% center to 90% near edge
                    float t = dist / radius;
                    brightness = 1.0f - (t * t * 0.1f);
                }
                else if (dist < radius + edge_softness)
                {
                    // Soft edge transition
                    float t = (dist - (radius - edge_softness)) / (2.0f * edge_softness);
                    brightness = (1.0f - 0.1f) * (1.0f - t); // Fade from 90% to 0%
                }
                // else: black (brightness = 0)

                unsigned char val = static_cast<unsigned char>(brightness * 255.0f);
                int idx = 18 + (y * size + x) * 4;
                circle_data[idx] = val;     // B
                circle_data[idx + 1] = val; // G
                circle_data[idx + 2] = val; // R
                circle_data[idx + 3] = 255; // A
            }
        }
        images.push_back(std::move(circle_data));
    }

    for (const auto &wheel : m_goboWheels)
    {
        // Only process wheels that contain "Gobo" in the name
        if (wheel.name.find("Gobo") == std::string::npos)
            continue;

        for (const auto &slot : wheel.slots)
        {
            // Skip empty slots (Open position already added above)
            if (slot.media_file_name.empty())
                continue;

            // Try common paths and extensions
            std::vector<std::string> paths_to_try = {"wheels/" + slot.media_file_name + ".png",
                                                     "wheels/" + slot.media_file_name + ".PNG",
                                                     "wheels/" + slot.media_file_name + ".jpg",
                                                     "wheels/" + slot.media_file_name + ".jpeg",
                                                     slot.media_file_name + ".png",
                                                     slot.media_file_name};

            std::vector<uint8_t> data;
            bool found = false;
            for (const auto &path : paths_to_try)
            {
                if (ExtractFile(path, data))
                {
                    found = true;
                    break;
                }
            }

            if (found && !data.empty())
            {
                images.push_back(std::move(data));
            }
        }
    }

    return images;
}

} // namespace GDTF
