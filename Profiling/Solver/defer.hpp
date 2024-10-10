#pragma once

namespace detail
{

template <typename TCallable>
class Defer {
public:
    Defer(TCallable &&callable) : m_callable(callable) {}
    ~Defer() { m_callable(); }

private:
    TCallable m_callable;
};

} // namespace detail

#define DEFER_FUNC(func_)      detail::Defer defer__##__COUNTER__(func_)
#define DEFER(stmt_)           DEFER_FUNC([&]() { stmt_; })
#define DEFER_IN_METHOD(stmt_) DEFER_FUNC([&, this]() { stmt_; })
