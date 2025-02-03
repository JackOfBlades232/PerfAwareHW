#pragma once

#include <util.hpp>

namespace detail
{

template <typename TCallable>
class Defer {
public:
    Defer(TCallable &&callable) : m_callable(mv(callable)) {}
    ~Defer() { m_callable(); }

private:
    TCallable m_callable;
};

} // namespace detail

#define DEFER(func_) detail::Defer CAT(defer__, __LINE__)(func_)
