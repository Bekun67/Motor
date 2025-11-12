#pragma once
#include <string>
#include <vector>

// Custom texture structure (no DevIL dependency in header)
struct CustomTexture {
    unsigned short width;
    unsigned short height;
    unsigned char channels;
    std::vector<unsigned char> data;

    CustomTexture() : width(0), height(0), channels(0) {}
};

namespace TextureImporter {
    // Import texture from file using DevIL
    CustomTexture ImportTexture(const std::string& texturePath);

    // Save custom texture to binary format
    bool SaveTexture(const CustomTexture& texture, const std::string& outputPath);

    // Load custom texture from binary format
    bool LoadTexture(CustomTexture& texture, const std::string& inputPath);

    // Get the custom texture file path
    std::string GetCustomTexturePath(const std::string& originalPath);
}