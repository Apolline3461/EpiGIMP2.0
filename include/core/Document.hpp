//
// Created by apolline on 09/12/2025.
//

#pragma once

using namespace std;

class Layer;

class Document {
public:
    explicit Document(int widht, int height, float dpi=72.f);

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
    int widht_{};
    int height_{};
    float dpi_{};

    vector<shared_ptr<Layer>> layers_{};

    // Selection selection_{};

};

