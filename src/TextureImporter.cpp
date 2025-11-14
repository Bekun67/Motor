#include "TextureImporter.h"
#include <IL/il.h>
#include <IL/ilu.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <limits>

namespace fs = std::filesystem;

namespace TextureImporter {

    // Import texture from file using DevIL
    CustomTexture ImportTexture(const std::string& texturePath) {
        CustomTexture texture;

        auto startTime = std::chrono::high_resolution_clock::now();

        // Initialize DevIL if not already done
        static bool devilInitialized = false;
        if (!devilInitialized) {
            ilInit();
            iluInit();
            ilEnable(IL_ORIGIN_SET);
            devilInitialized = true;
        }

        ILuint imageID;
        ilGenImages(1, &imageID);
        ilBindImage(imageID);

        if (!ilLoadImage((const ILstring)texturePath.c_str())) {
            std::cerr << "[TextureImporter] Failed to load texture: " << texturePath << std::endl;
            ilDeleteImages(1, &imageID);
            return texture;
        }

        iluFlipImage();

        // Convert to RGBA
        ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

        texture.width = ilGetInteger(IL_IMAGE_WIDTH);
        texture.height = ilGetInteger(IL_IMAGE_HEIGHT);
        texture.channels = 4; // RGBA

        unsigned char* imageData = ilGetData();
        size_t dataSize = texture.width * texture.height * texture.channels;
        texture.data.resize(dataSize);
        memcpy(texture.data.data(), imageData, dataSize);

        ilDeleteImages(1, &imageID);

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        std::cout << "[TextureImporter] Texture loaded in " << duration.count() << " ms" << std::endl;
        std::cout << "[TextureImporter] " << texture.width << "x" << texture.height
            << " (" << (int)texture.channels << " channels)" << std::endl;

        return texture;
    }

    // Save custom texture to binary format
    bool SaveTexture(const CustomTexture& texture, const std::string& outputPath) {
        std::ofstream file(outputPath, std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "[TextureImporter] Failed to open file for writing: " << outputPath << std::endl;
            return false;
        }

        // Write header
        file.write(reinterpret_cast<const char*>(&texture.width), sizeof(texture.width));
        file.write(reinterpret_cast<const char*>(&texture.height), sizeof(texture.height));
        file.write(reinterpret_cast<const char*>(&texture.channels), sizeof(texture.channels));

        // Write image data
        file.write(reinterpret_cast<const char*>(texture.data.data()), texture.data.size());

        file.close();

        std::cout << "[TextureImporter] Saved texture to: " << outputPath << std::endl;
        return true;
    }

    // Load custom texture from binary format
    bool LoadTexture(CustomTexture& texture, const std::string& inputPath) {
        auto startTime = std::chrono::high_resolution_clock::now();

        std::ifstream file(inputPath, std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "[TextureImporter] Failed to open file for reading: " << inputPath << std::endl;
            return false;
        }

        // Read header
        file.read(reinterpret_cast<char*>(&texture.width), sizeof(texture.width));
        file.read(reinterpret_cast<char*>(&texture.height), sizeof(texture.height));
        file.read(reinterpret_cast<char*>(&texture.channels), sizeof(texture.channels));

        // Read image data
        size_t dataSize = texture.width * texture.height * texture.channels;
        texture.data.resize(dataSize);
        file.read(reinterpret_cast<char*>(texture.data.data()), dataSize);

        file.close();

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        std::cout << "[TextureImporter] Custom texture loaded in " << duration.count() << " ms" << std::endl;

        return true;
    }

    // Get the custom texture file path
    std::string GetCustomTexturePath(const std::string& originalPath) {
        fs::path p(originalPath);
        std::string filename = p.stem().string();

        return "Library/Materials/" + filename + ".iltex";
    }
}