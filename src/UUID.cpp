#include "UUID.h"

static std::random_device s_RandomDevice;
static std::mt19937 s_Engine(s_RandomDevice());
static std::uniform_int_distribution<uint32_t> s_UniformDistribution;

UUID::UUID()
    : m_UUID(s_UniformDistribution(s_Engine))
{
}

UUID::UUID(uint32_t uuid)
    : m_UUID(uuid)
{
}

std::string UUID::ToString() const
{
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << m_UUID;
    return ss.str();
}

UUID UUID::FromString(const std::string& str)
{
    uint32_t value;
    std::stringstream ss;
    ss << std::hex << str;
    ss >> value;
    return UUID(value);
}