//
// Created by apolline on 17/11/2025.
//

#pragma once

#include <cstdint>
#include <vector>
#include <cassert>

class ImageBuffer
{
public:
  ImageBuffer(int width, int height);

  [[nodiscard]] int width() const noexcept;
  [[nodiscard]] int height() const noexcept;
  [[nodiscard]] int strideBytes() const noexcept;

  uint8_t* data() noexcept;
  [[nodiscard]] const uint8_t* data() const noexcept;

  void fill(uint32_t rgba);
  [[nodiscard]] uint32_t getPixel(int x, int y) const;
  void setPixel(int x, int y, uint32_t rgba);

private:
  int width_{};
  int height_{};
  int stride_{};

  std::vector<uint8_t> rgbaPixels_;
};

