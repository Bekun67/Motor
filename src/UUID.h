#pragma once
#include <cstdint>
#include <random>
#include <string>
#include <sstream>
#include <iomanip>

class EngineUUID
{
public:
    EngineUUID();
    EngineUUID(uint32_t uuid);

    operator uint32_t() const { return m_UUID; }

    std::string ToString() const;
    static EngineUUID FromString(const std::string& str);

private:
    uint32_t m_UUID;
};

namespace std {
    template<>
    struct hash<EngineUUID>
    {
        size_t operator()(const EngineUUID& uuid) const
        {
            return hash<uint32_t>()((uint32_t)uuid);
        }
    };
}