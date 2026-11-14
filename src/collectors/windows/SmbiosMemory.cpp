#include "collectors/windows/SmbiosMemory.hpp"
#include <set>

namespace {
const char* memoryType(std::uint8_t type) {
    // DMTF SMBIOS Type 17 Memory Type enumeration.
    switch (type) {
        case 0x03: return "DRAM";
        case 0x0F: return "SDRAM";
        case 0x12: return "RDRAM";
        case 0x13: return "DDR";
        case 0x14: return "DDR2";
        case 0x15: return "DDR2 FB-DIMM";
        case 0x18: return "DDR3";
        case 0x1A: return "DDR4";
        case 0x1B: return "LPDDR";
        case 0x1C: return "LPDDR2";
        case 0x1D: return "LPDDR3";
        case 0x1E: return "LPDDR4";
        case 0x20: return "HBM";
        case 0x21: return "HBM2";
        case 0x22: return "DDR5";
        case 0x23: return "LPDDR5";
        case 0x24: return "HBM3";
        default: return nullptr;
    }
}
}

std::optional<std::string> parseSmbiosMemoryType(const std::uint8_t* data, std::size_t size) {
    if (!data) return std::nullopt;
    std::set<std::string> types;
    std::size_t offset = 0;
    while (offset < size) {
        if (size - offset < 4) return std::nullopt;
        const auto type = data[offset];
        const auto length = data[offset + 1];
        if (length < 4 || length > size - offset) return std::nullopt;
        std::size_t end = offset + length;
        while (end + 1 < size && (data[end] != 0 || data[end + 1] != 0)) ++end;
        if (end + 1 >= size) return std::nullopt;
        if (type == 17) {
            if (length < 0x15) return std::nullopt;
            // Size == 0 means an empty slot; 0xFFFF means unknown size.
            if (data[offset + 0x0C] != 0 || data[offset + 0x0D] != 0) {
                const char* name = memoryType(data[offset + 0x12]);
                if (!name) return std::nullopt;
                types.insert(name);
            }
        }
        offset = end + 2;
        if (type == 127) break;
    }
    if (types.empty()) return std::nullopt;
    std::string result;
    for (const auto& type : types) {
        if (!result.empty()) result += " / ";
        result += type;
    }
    return result;
}
