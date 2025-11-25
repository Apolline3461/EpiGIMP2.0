#include <iostream>
#include "EpgFile.hpp"
#include "document.hpp"

int main() {
    Document doc(800, 600);

    doc.createLayer("Background", 800, 600);
    doc.createLayer("Brush Stroke", 800, 600);
    std::cout << "Document créé avec " << doc.layerCount() << " calques.\n";
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

    std::cout << "Document chargé ! Nombre de calques : " << res.document->layerCount() << "\n";
    return 0;
}
