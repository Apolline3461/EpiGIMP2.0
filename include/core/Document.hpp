//
// Created by apolline on 09/12/2025.
//

#pragma once

#include <memory>
#include <vector>

#include "Selection.hpp"

class Layer;

class Document
{
   public:
    explicit Document(int width, int height, float dpi = 72.f);

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] float dpi() const noexcept;

    size_t addLayer(std::shared_ptr<Layer> layer);
    size_t addLayer(std::shared_ptr<Layer> layer, size_t idx);
    void removeLayer(size_t idx);
    void reorderLayer(size_t from, size_t to);
    void mergeDown(size_t from);

    [[nodiscard]] size_t layerCount() const noexcept;
    [[nodiscard]] std::shared_ptr<Layer> layerAt(size_t index) const;

    Selection& selection() noexcept
    {
        return selection_;
    }
    const Selection& selection() const noexcept
    {
        return selection_;
    };

   private:
    int width_{};
    int height_{};
    float dpi_{};

    std::vector<std::shared_ptr<Layer>> layers_{};

    Selection selection_{};
};
