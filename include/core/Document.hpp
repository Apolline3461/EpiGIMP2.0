//
// Created by apolline on 09/12/2025.
//

#pragma once

#include <memory>
#include <vector>

class Layer;

class Document {
public:
    explicit Document(int width, int height, float dpi=72.f);

    int width() const noexcept;
    int height() const noexcept;
    float dpi() const noexcept;

    int addLayer(std::shared_ptr<Layer> layer, int idx = -1);
    void removeLayer(int idx);
    void reorderLayer(int from, int to);
    void mergeDown(int from);

    int layerCount() const noexcept;
    std::shared_ptr<Layer> layerAt(int index) const;


    //const Selection& selection() const noexcept;

private:
    int width_{};
    int height_{};
    float dpi_{};

    std::vector<std::shared_ptr<Layer>> layers_{};

    // Selection selection_{};

};

