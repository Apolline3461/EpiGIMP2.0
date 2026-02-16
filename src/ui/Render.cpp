//
// Created by apolline on 16/02/2026.
//

#include "ui/Render.hpp"

#include "core/Compositor.hpp"
#include "core/ImageBuffer.hpp"
#include "ui/ImageConversion.hpp"

QImage Renderer::render(const Document& doc)
{
    ImageBuffer out(doc.width(), doc.height());
    out.fill(0u);

    Compositor comp;
    comp.compose(doc, out);

    return ImageConversion::imageBufferToQImage(out, QImage::Format_ARGB32);
}
