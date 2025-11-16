#pragma once
#include <cstdint>
#include <random>
#include <string>
#include <sstream>
#include <iomanip>

class UUID
{
public:
    UUID();
    UUID(uint32_t uuid);

    operator uint32_t() const { return m_UUID; }

    std::string ToString() const;
    static UUID FromString(const std::string& str);

private:
    uint32_t m_UUID;
};

namespace std {
    template<>
    struct hash<UUID>
    {
        size_t operator()(const UUID& uuid) const
        {
            return hash<uint32_t>()((uint32_t)uuid);
        }
    };
}