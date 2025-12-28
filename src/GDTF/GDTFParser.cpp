#include "GDTFParser.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <miniz/miniz.h>
#include <sstream>

namespace GDTF
{

bool GDTFParser::Load(const std::string &fileName)
{
    m_gdtfPath = fileName;

    std::vector<uint8_t> xmlData;
    if (!ExtractFile("description.xml", xmlData))
    {
        std::cerr << "Failed to extract description.xml from " << fileName << '\n';
        return false;
    }

    return ParseXML(std::string(xmlData.begin(), xmlData.end()));
}

bool GDTFParser::ExtractFile(const std::string &internalPath, std::vector<uint8_t> &outData)
{
    mz_zip_archive zipArchive;
    memset(&zipArchive, 0, sizeof(zipArchive));

    if (!mz_zip_reader_init_file(&zipArchive, m_gdtfPath.c_str(), 0))
    {
        return false;
    }

    int fileIndex = mz_zip_reader_locate_file(&zipArchive, internalPath.c_str(), nullptr, 0);
    if (fileIndex < 0)
    {
        // Try to search case-insensitive if exact match fails
        int numFiles = static_cast<int>(mz_zip_reader_get_num_files(&zipArchive));

        for (int i = 0; i < numFiles; i++)
        {
            mz_zip_archive_file_stat fileStat;
            if (mz_zip_reader_file_stat(&zipArchive, i, &fileStat))
            {
                std::string fname = fileStat.m_filename;

                if (fname.length() == internalPath.length())
                {
                    bool match = std::equal(fname.begin(), fname.end(), internalPath.begin(),
                                            [](char a, char b)
                                            {
                                                return std::tolower(static_cast<unsigned char>(a)) ==
                                                       std::tolower(static_cast<unsigned char>(b));
                                            });

                    if (match)
                    {
                        fileIndex = i;
                        break;
                    }
                }
            }
        }
    }

    if (fileIndex < 0)
    {
        mz_zip_reader_end(&zipArchive);
        return false;
    }

    mz_zip_archive_file_stat fileStat;
    if (!mz_zip_reader_file_stat(&zipArchive, fileIndex, &fileStat))
    {
        mz_zip_reader_end(&zipArchive);
        return false;
    }

    outData.resize((size_t)fileStat.m_uncomp_size);
    if (!mz_zip_reader_extract_to_mem(&zipArchive, fileIndex, outData.data(), outData.size(), 0))
    {
        mz_zip_reader_end(&zipArchive);
        return false;
    }

    mz_zip_reader_end(&zipArchive);
    return true;
}

bool GDTFParser::ParseXML(const std::string &xmlContent)
{
    pugi::xml_parse_result result = m_doc.load_string(xmlContent.c_str());
    if (!result)
    {
        return false;
    }

    pugi::xml_node fixtureType = m_doc.child("GDTF").child("FixtureType");
    m_fixtureTypeName = fixtureType.attribute("Name").as_string();

    // Parse Models
    m_modelToFile.clear();
    pugi::xml_node models = fixtureType.child("Models");
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
    pugi::xml_node geometries = fixtureType.child("Geometries");
    for (pugi::xml_node child : geometries.children())
    {
        m_geometryRoot = ParseGeometry(child);
        if (m_geometryRoot)
            break;
    }

    // Parse Wheels
    m_goboWheels.clear();
    pugi::xml_node wheelsNode = fixtureType.child("Wheels");
    for (pugi::xml_node wheelNode : wheelsNode.children("Wheel"))
    {
        GoboWheel wheel;
        wheel.name = wheelNode.attribute("Name").as_string();

        for (pugi::xml_node slotNode : wheelNode.children("Slot"))
        {
            GoboSlot slot;
            slot.name = slotNode.attribute("Name").as_string();
            slot.media_file_name = slotNode.attribute("MediaFileName").as_string();

            wheel.slots.push_back(slot);
        }
        m_goboWheels.push_back(wheel);
    }

    // Parse DMX Modes (take the first one)
    pugi::xml_node dmxMode = fixtureType.child("DMXModes").child("DMXMode");
    if (dmxMode)
    {
        pugi::xml_node channels = dmxMode.child("DMXChannels");
        int currentOffset = 0;
        for (pugi::xml_node chan : channels.children("DMXChannel"))
        {
            DMXChannel dc;
            dc.name = chan.attribute("Geometry").as_string();
            if (dc.name.empty())
                dc.name = chan.attribute("Attribute").as_string();

            dc.offset = currentOffset;
            dc.byte_count = 0;
            for ([[maybe_unused]] pugi::xml_node logical : chan.children("LogicalChannel"))
            {
                dc.byte_count++;
            }
            if (dc.byte_count == 0)
                dc.byte_count = 1;

            dc.default_value = chan.attribute("Default").as_float();
            m_dmxChannels.push_back(dc);
            currentOffset += dc.byte_count;
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

        std::string matrixStr = node.attribute("Matrix").as_string();
        if (matrixStr.empty())
            matrixStr = node.attribute("Position").as_string();

        if (!matrixStr.empty())
        {
            std::string cleaned = matrixStr;
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
            DirectX::XMMATRIX gdtfMat = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&raw));

            // Apply translation inversion to match the physical model offsets
            DirectX::XMFLOAT4X4 finalMat;
            DirectX::XMStoreFloat4x4(&finalMat, gdtfMat);
            finalMat._41 = -finalMat._41;
            finalMat._42 = -finalMat._42;
            finalMat._43 = -finalMat._43;
            gn->matrix = finalMat;
        }
        for (pugi::xml_node child : node.children())
        {
            auto childNode = ParseGeometry(child);
            if (childNode)
            {
                gn->children.push_back(childNode);
            }
        }
        return gn;
    }
    return nullptr;
}

std::string GDTFParser::GetModelFile(const std::string &modelName) const
{
    auto it = m_modelToFile.find(modelName);
    if (it != m_modelToFile.end())
    {
        return it->second;
    }
    return modelName;
}

std::vector<std::vector<uint8_t>> GDTFParser::ExtractGoboImages()
{
    std::vector<std::vector<uint8_t>> images;

    // First slot is always "Open" (radial gradient - bright center, soft falloff)
    // Create a simple TGA in memory
    {
        const int size = 512;
        std::vector<uint8_t> circleData;
        // Simple uncompressed TGA format (easier than PNG)
        // TGA header (18 bytes)
        circleData.resize(18 + (static_cast<size_t>(size) * size * 4));
        circleData[2] = 2; // Uncompressed true-color
        circleData[12] = size & 0xFF;
        circleData[13] = (size >> 8) & 0xFF;
        circleData[14] = size & 0xFF;
        circleData[15] = (size >> 8) & 0xFF;
        circleData[16] = 32;   // 32 bits per pixel
        circleData[17] = 0x20; // Top-left origin
        // Radial gradient with hard edge cutoff (like real gobos)
        const float center = size / 2.0f;
        const float radius = center * 0.40f;      // Circle occupies 50% of diameter
        const float edgeSoftness = radius * 0.1f; // Soft edge zone
        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                float dx = static_cast<float>(x) - center;
                float dy = static_cast<float>(y) - center;
                float dist = std::sqrt((dx * dx) + (dy * dy));

                float brightness = 0.0f;
                if (dist < radius - edgeSoftness)
                {
                    // Inside: gradient from 100% center to 90% near edge
                    float t = dist / radius;
                    brightness = 1.0f - (t * t * 0.1f);
                }
                else if (dist < radius + edgeSoftness)
                {
                    // Soft edge transition
                    float t = (dist - (radius - edgeSoftness)) / (2.0f * edgeSoftness);
                    brightness = (1.0f - 0.1f) * (1.0f - t); // Fade from 90% to 0%
                }
                // else: black (brightness = 0)

                auto val = static_cast<unsigned char>(brightness * 255.0f);
                int idx = static_cast<int>(18 + (((static_cast<size_t>(y) * size) + x) * 4));
                circleData[static_cast<size_t>(idx)] = val;     // B
                circleData[static_cast<size_t>(idx) + 1] = val; // G
                circleData[static_cast<size_t>(idx) + 2] = val; // R
                circleData[static_cast<size_t>(idx) + 3] = 255; // A
            }
        }
        images.push_back(std::move(circleData));
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
            std::vector<std::string> pathsToTry = {"wheels/" + slot.media_file_name + ".png",
                                                   "wheels/" + slot.media_file_name + ".PNG",
                                                   "wheels/" + slot.media_file_name + ".jpg",
                                                   "wheels/" + slot.media_file_name + ".jpeg",
                                                   slot.media_file_name + ".png",
                                                   slot.media_file_name};

            std::vector<uint8_t> data;
            bool found = false;
            for (const auto &path : pathsToTry)
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