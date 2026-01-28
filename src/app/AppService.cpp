//
// Created by apolline on 21/01/2026.
//
#include "app/AppService.hpp"

#include <stdexcept>

namespace app
{

AppService::AppService(std::unique_ptr<IStorage> storage)
    : storage_(std::move(storage)), history_(20), doc_(1, 1, 1.F)
{
}

void AppService::newDocument(Size /*size*/, float /*dpi*/)
{
    throw std::logic_error("TODO AppService::newDocument");
}

void AppService::open(const std::string& /*path*/)
{
    throw std::logic_error("TODO AppService::open");
}

void AppService::save(const std::string& /*path*/)
{
    throw std::logic_error("TODO AppService::save");
}

void AppService::exportPng(const std::string& /*path*/)
{
    throw std::logic_error("TODO AppService::exportPng");
}

const Document& AppService::document() const
{
    return doc_;
}
std::size_t AppService::activeLayer() const
{
    return activeLayer_;
}

void AppService::setActiveLayer(std::size_t /*idx*/)
{
    throw std::logic_error("TODO AppService::setActiveLayer");
}

void AppService::addLayer()
{
    throw std::logic_error("TODO AppService::addLayer");
}

void AppService::undo()
{
    throw std::logic_error("TODO AppService::undo");
}
void AppService::redo()
{
    throw std::logic_error("TODO AppService::redo");
}

bool AppService::canUndo() const noexcept
{
    return false;
}
bool AppService::canRedo() const noexcept
{
    return false;
}

}  // namespace app