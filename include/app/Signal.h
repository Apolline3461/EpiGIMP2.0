//
// Created by apolline on 29/01/2026.
//

#pragma once

namespace app
{
class Signal
{
   public:
    using Slot = std::function<void()>;
    void connect(Slot s)
    {
        slots_.push_back(std::move(s));
    }
    void emit()
    {
        for (const auto& s : slots_)
            s();
    }

   private:
    std::vector<Slot> slots_;
};
}  // namespace app