//
// Created by apolline on 09/01/2026.
//
#pragma once

class Document;
class ImageBuffer;

class Compositor
{
   public:
    static void compose(const Document& doc, ImageBuffer& out);
    static void composeROI(const Document& doc, int x, int y, int w, int h, ImageBuffer& out);
};
