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
    
    Layer layer1(0);
    Layer layer2(1);
    layer1.name = "Background";
    layer2.name = "Foreground";
    layer1.pixels = std::make_shared<ImageBuffer>(img);
    layer2.pixels = std::make_shared<ImageBuffer>(img);
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
