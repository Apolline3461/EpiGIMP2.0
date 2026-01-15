//
// Created by apolline on 09/01/2026.
//
#pragma once

class Document;
class ImageBuffer;

class Compositor
{
   public:
    void compose(const Document& doc, ImageBuffer& out) const;
    void composeROI(const Document& doc, int x, int y, int w, int h, ImageBuffer& out) const;
};
