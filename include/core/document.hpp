#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class ImageBuffer;
class Layer;

// Fournit des constructeurs et des méthodes utilitaires simples pour gérer
// la liste de calques (layers) sans exposer davantage d'implémentation.
struct Document
{
    // Taille du document en pixels
    int width{0};
    int height{0};

    // Résolution en DPI
    float dpi{72.0f};

    // Les calques sont conservés en `shared_ptr<Layer>` (core::Layer)
    std::vector<std::shared_ptr<Layer>> layers;

    // --- Constructeurs / opérateurs ---
    Document() = default;
    Document(int w, int h, float d = 72.0f) noexcept : width(w), height(h), dpi(d) {}
    Document(const Document&) = default;
    Document(Document&&) noexcept = default;
    Document& operator=(const Document&) = default;
    Document& operator=(Document&&) noexcept = default;
    ~Document() = default;

    // --- Méthodes utilitaires ---
    // Ajoute un calque en fin de liste. Accepte nullptr (ignoré).
    void addLayer(std::shared_ptr<Layer> layer)
    {
        if (layer)
            layers.emplace_back(std::move(layer));
    }

    // Retourne le calque à l'index donné, ou nullptr si hors limite.
    std::shared_ptr<Layer> layerAt(std::size_t index) const noexcept
    {
        if (index >= layers.size())
            return nullptr;
        return layers[index];
    }

    // Supprime et retourne le calque à l'index donné. Retourne nullptr si hors limite.
    std::shared_ptr<Layer> removeLayerAt(std::size_t index) noexcept
    {
        if (index >= layers.size())
            return nullptr;
        auto removed = layers[index];
        layers.erase(layers.begin() + static_cast<std::ptrdiff_t>(index));
        return removed;
    }

    // Nombre de calques
    std::size_t layerCount() const noexcept
    {
        return layers.size();
    }

    // Vide la liste des calques
    void clearLayers() noexcept
    {
        layers.clear();
    }

    // Indique si le document ne contient aucun calque
    bool isEmpty() const noexcept
    {
        return layers.empty();
    }
};
