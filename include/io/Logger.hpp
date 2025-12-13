// Minimal header-only logger used by EpiGIMP
#pragma once

#include <iostream>
#include <string>

namespace epg
{
inline void log_info(const std::string& s)
{
    std::cout << "[info] " << s << std::endl;
}

inline void log_warn(const std::string& s)
{
    std::cerr << "[warn] " << s << std::endl;
}

inline void log_error(const std::string& s)
{
    std::cerr << "[error] " << s << std::endl;
}
}  // namespace epg
