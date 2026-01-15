#pragma once

#include <zip.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/document.hpp"
#include "core/ImageBuffer.hpp"

// PNG signature constant
inline constexpr unsigned char kPngSignature[8] = {137, 80, 78, 71, 13, 10, 26, 10};

namespace io::epg
{

class ZipHandle
{
   public:
    explicit ZipHandle(zip_t* z = nullptr) noexcept : z_(z) {}
    ~ZipHandle() noexcept
    {
        if (z_)
            zip_close(z_);
    }

    ZipHandle(ZipHandle&& o) noexcept : z_(o.z_)
    {
        o.z_ = nullptr;
    }
    ZipHandle& operator=(ZipHandle&& o) noexcept
    {
        if (this != &o)
        {
            if (z_)
                zip_close(z_);
            z_ = o.z_;
            o.z_ = nullptr;
        }
        return *this;
    }

    ZipHandle(const ZipHandle&) = delete;
    ZipHandle& operator=(const ZipHandle&) = delete;

    zip_t* get() const noexcept
    {
        return z_;
    }
    operator zip_t*() const noexcept
    {
        return z_;
    }
    zip_t* release() noexcept
    {
        zip_t* t = z_;
        z_ = nullptr;
        return t;
    }

   private:
    zip_t* z_{nullptr};
};

struct OpenResult
{
    bool success{false};
    std::string errorMessage;
    std::unique_ptr<Document> document;
};

}  // namespace io::epg
