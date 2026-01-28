#pragma once

#include <memory>
#include <string>

#include "core/Document.hpp"
#include "io/EpgTypes.hpp"

// Interface abstract for storage backends
class IStorage
{
   public:
    virtual ~IStorage() = default;
    virtual io::epg::OpenResult open(const std::string& path) = 0;
    virtual void save(const Document& doc, const std::string& path) = 0;
    virtual void exportPng(const Document& doc, const std::string& path) = 0;
};
