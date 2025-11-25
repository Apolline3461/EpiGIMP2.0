#pragma once
#include <vector>
#include <string>
#include <memory>
#include "layer.hpp"

// Fake Document class for testing purposes
class Document {
public:
    int width{800};
    int height{600};
    int dpi{72};

    std::vector<std::unique_ptr<Layer>> layers;

    Document() = default;
    Document(int w, int h) : width(w), height(h) {}

    Layer& createLayer(const std::string& name, int w, int h) {
        std::string id = generateLayerId();
        layers.push_back(std::make_unique<Layer>(id, name, w, h));
        return *layers.back();
    }

    size_t layerCount() const { return layers.size(); }

private:
    std::string generateLayerId() {
        int n = static_cast<int>(layers.size());
        char buf[16];
        snprintf(buf, sizeof(buf), "%04d", n + 1);
        return std::string(buf);
    }
};