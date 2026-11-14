#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

// Parses SMBIOS structures without the Windows RawSMBIOSData header.
// Empty for missing/unknown types or malformed firmware data.
std::optional<std::string> parseSmbiosMemoryType(const std::uint8_t* data, std::size_t size);
