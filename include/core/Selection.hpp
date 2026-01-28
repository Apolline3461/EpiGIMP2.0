/*
** EPITECH PROJECT, 2026
** EpiGIMP2.0
** File description:
** Selection
*/

#pragma once

#include <cstdint>
#include <memory>

class ImageBuffer;

class Selection
{
   public:
    Selection() = default;
    explicit Selection(std::shared_ptr<ImageBuffer> mask) : mask_(std::move(mask)) {}

    [[nodiscard]] bool hasMask() const noexcept
    {
        return static_cast<bool>(mask_);
    }
    uint8_t t_at(int x, int y) const;

    // Use a small Rect type to avoid multiple adjacent primitive parameters
    struct Rect
    {
        int x{};
        int y{};
        int w{};
        int h{};
    };

    void addRect(
        const Rect& rect,
        const std::shared_ptr<ImageBuffer>& reference =
            std::shared_ptr<ImageBuffer>())  // NOLINT(bugprone-easily-swappable-parameters)
        ;
    void subtractRect(const Rect& rect)  // NOLINT(bugprone-easily-swappable-parameters)
        ;
    void clear() noexcept
    {
        mask_.reset();
    }

    void setMask(std::shared_ptr<ImageBuffer> mask)
    {
        mask_ = std::move(mask);
    }
    [[nodiscard]] const std::shared_ptr<ImageBuffer>& mask() const noexcept
    {
        return mask_;
    }

   private:
    std::shared_ptr<ImageBuffer> mask_;
};
