#pragma once
#include <memory>
#include <string>

// Forward declaration
class ImageBuffer;

// Fake Layer class for testing purposes
class Layer
{
   public:
    std::string id;
    std::string name;
    bool visible{true};
    float opacity{1.0f};

    std::unique_ptr<ImageBuffer> image;

    Layer();                                                // constructeur par défaut
    Layer(std::string ID, std::string Name, int w, int h);  // déclaration seulement
    ~Layer();

    // Layer contains a `std::unique_ptr`, so copying should be disabled.
    Layer(const Layer&) = delete;
    Layer& operator=(const Layer&) = delete;

    // Allow move semantics
    Layer(Layer&&) = default;
    Layer& operator=(Layer&&) = default;
};
