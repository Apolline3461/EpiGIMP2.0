//
// Created by apolline on 09/12/2025.
//

#pragma once

#include <memory>
#include <vector>

class Layer;

class Document
{
   public:
    explicit Document(int width, int height,
                      float dpi = 72.f)  // NOLINT(bugprone-easily-swappable-parameters)
        ;

    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] float dpi() const noexcept;

    int addLayer(std::shared_ptr<Layer> layer);
    int addLayer(std::shared_ptr<Layer> layer, int idx);
    void removeLayer(int idx);
    void reorderLayer(int from, int to);
    void setLayers(std::vector<std::shared_ptr<Layer>> layers);
    void mergeDown(int from);

    [[nodiscard]] int layerCount() const noexcept;
    [[nodiscard]] std::shared_ptr<Layer> layerAt(int index) const;

    // const Selection& selection() const noexcept;
    //[[nodiscard]] Selection& selection() noexcept { return selection_; }
    //[[nodiscard]] const Selection& selection() const noexcept { return selection_; }

   private:
    int width_{};
    int height_{};
    float dpi_{};

    std::vector<std::shared_ptr<Layer>> layers_{};

    // Selection selection_{};
};
