#include <iostream>
#include "EpgFile.hpp"
#include "document.hpp"

int main() {
    Document doc;
    ImageBuffer img;
    img.width = 100;
    img.height = 100;
    img.stride = img.width * 4;
    img.rgba.resize(img.stride * img.height, 255); // blanc opaque
    ImageBuffer img2;
    img2.width = 100;
    img2.height = 100;
    img2.stride = img2.width * 4;
    img2.rgba.resize(img2.stride * img2.height);
    for (size_t y = 0; y < img2.height; ++y) {
        for (size_t x = 0; x < img2.width; ++x) {
            size_t i = y * img2.stride + x * 4;
            img2.rgba[i + 0] = 255; // R
            img2.rgba[i + 1] = 0;   // G
            img2.rgba[i + 2] = 0;   // B
            img2.rgba[i + 3] = 255; // A
        }
    }
    
    Layer layer1(0);
    Layer layer2(1);
    layer1.name = "Background";
    layer2.name = "Foreground";
    layer1.pixels = std::make_shared<ImageBuffer>(img);
    layer2.pixels = std::make_shared<ImageBuffer>(img2);
    doc.layers.push_back(std::make_shared<Layer>(layer1));
    doc.layers.push_back(std::make_shared<Layer>(layer2));
    ZipEpgStorage storage;

    try {
        storage.save(doc, "test.epg");
        std::cout << "ZIP sauvegardé !\n";
    } catch (const std::exception& e) {
        std::cout << "Erreur SAVE: " << e.what() << "\n";
    }

    OpenResult res = storage.open("test.epg");
    if (!res.success) {
        std::cout << "Erreur OPEN: " << res.errorMessage << "\n";
        return 1;
    }

    return 0;
}
