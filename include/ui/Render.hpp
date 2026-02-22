//
// Created by apolline on 16/02/2026.
//

#pragma once
#include <QImage>

#include "core/Document.hpp"

class Renderer
{
   public:
    static QImage render(const Document& doc);
};
