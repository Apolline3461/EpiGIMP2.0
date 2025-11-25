#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>


// Fake ImageBuffer class for testing purposes
class ImageBuffer {
public:
    int width{0};
    int height{0};
    std::vector<uint8_t> pixels; // RGBA8

    ImageBuffer() = default;
    ~ImageBuffer();

    ImageBuffer(int w, int h)
        : width(w), height(h), pixels(static_cast<size_t>(w) * h * 4, 255) // blanc opaque
    {}

    size_t size() const { return pixels.size(); }

    uint8_t* data() { return pixels.data(); }
    const uint8_t* data() const { return pixels.data(); }

    bool isValid() const { return width > 0 && height > 0 && pixels.size() == static_cast<size_t>(width) * height * 4; }
};

// define destructor out-of-line to ensure the type is complete at the point of definition
inline ImageBuffer::~ImageBuffer() = default;
