//
// Created by apolline on 25/11/2025.
//

#pragma once

#include <cstdint>
#include <memory>
#include <string>

class ImageBuffer;

class Layer
{
   public:
    Layer(uint64_t id, std::string name, std::shared_ptr<ImageBuffer> image, bool visible = true,
          bool locked = false, float opacity = 1.0f);

    [[nodiscard]] std::uint64_t id() const noexcept;

    [[nodiscard]] const std::string& name() const noexcept;
    void setName(std::string name);

    [[nodiscard]] bool visible() const noexcept;
    void setVisible(bool visible);

    [[nodiscard]] bool locked() const noexcept;
    void setLocked(bool locked);

    [[nodiscard]] bool isEditable() const noexcept;

    [[nodiscard]] float opacity() const noexcept;
    void setOpacity(float opacity);

    [[nodiscard]] const std::shared_ptr<ImageBuffer>& image() const noexcept;
    void setImageBuffer(std::shared_ptr<ImageBuffer> image);

   private:
    const uint64_t id_;
    std::string name_;
    bool visible_{true};
    bool locked_{false};
    float opacity_{1.0f};
    std::shared_ptr<ImageBuffer> image_;
};