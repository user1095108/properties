#ifndef PROPERTIES_HPP
# define PROPERTIES_HPP
# pragma once

#include <functional>

#include <initializer_list>

#include <memory>

#include <type_traits>

#include "json.hpp"

namespace nlm = nlohmann;

class properties
{
  template <typename U>
  using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<U>>;

  using serializor_t = std::function<nlm::json()>;
  using deserializor_t = std::function<void(nlm::json)>;

  struct property_info
  {
    std::string_view k{};
    serializor_t serializor{};
    deserializor_t deserializor{};

    property_info() noexcept = default;

    template <typename U,
      typename std::enable_if_t<
        std::is_lvalue_reference_v<U> &&
        !std::is_const_v<std::remove_reference_t<U>>,
        int
      > = 0
    >
    property_info(std::string_view key, U&& u):
      k(std::move(key)),
      serializor([&]()noexcept->decltype(auto){return u;}),
      deserializor([&](auto&& j)
        {
          if (!std::strcmp(nlm::json(u).type_name(), j.type_name()))
          {
            u = j.template get<remove_cvref_t<U>>();
          }
        }
      )
    {
    }

    template <typename U,
      typename std::enable_if_t<
        std::is_lvalue_reference_v<U> &&
        std::is_const_v<std::remove_reference_t<U>>,
        int
      > = 0
    >
    property_info(std::string_view key, U&& u):
      k(std::move(key)),
      serializor([&]()noexcept->decltype(auto){return u;})
    {
    }

    template <typename U,
      typename std::enable_if_t<
        std::is_invocable_v<U>,
        int
      > = 0
    >
    property_info(std::string_view key, U&& u):
      k(std::move(key)),
      serializor([=]()noexcept(noexcept(u()))->decltype(auto){return u();})
    {
    }

    template <typename U,
      typename V,
      typename std::enable_if_t<
        std::is_invocable_v<U> &&
        std::is_invocable_v<V, nlm::json>,
        int
      > = 0
    >
    property_info(std::string_view key, U&& u, V&& v):
      k(std::move(key)),
      serializor([=]()noexcept(noexcept(u()))->decltype(auto){return u();}),
      deserializor([=](auto&& j){v(std::forward<decltype(j)>(j));})
    {
    }
  };

  std::function<property_info const*(
    std::function<bool(property_info const&)>
  )> visitor_;

public:
  properties() = default;

  properties(properties const&) = delete;
  properties(properties&&) = delete;

  properties& operator=(properties const&) = delete;
  properties& operator=(properties&&) = delete;

  //
  nlm::json state() const;
  void state(nlm::json const&) const;

  //
  auto register_property(std::initializer_list<property_info> l)
  {
    //auto b(std::make_shared<property_info[]>(l.size()));

    std::shared_ptr<property_info[]> b(new property_info[l.size()]);
    std::copy(l.begin(), l.end(), b.get());

    visitor_ = [this, b(std::move(b)), c(std::move(visitor_)), sz(l.size())](
      auto const f) noexcept(noexcept(f(
      std::declval<property_info const&>()))) ->
      properties::property_info const*
      {
        for (auto i(b.get()), end(i + sz); end != i; ++i)
        {
          if (f(*i))
          {
            return i;
          }
        }

        return c ? c(std::move(f)) : nullptr;
      };
  }

  //
  nlm::json get(std::string_view const&) const;

  template <typename U>
  auto set(std::string_view const& k, U&& u) const
  {
    if (auto const pi(visitor_([&](auto& pi) noexcept
      {
        return pi.k == k;
      })); pi && pi->deserializor)
    {
      pi->deserializor(std::forward<U>(u));
    }

    return [&](auto&& ...a)
      {
        return set(std::forward<decltype(a)>(a)...);
      };
  }

  template <typename F>
  void visit(F const f) const
  {
    visitor_([&](auto& pi)
      {
        return f(pi.k, pi.serializor());
      }
    );
  }
};

//////////////////////////////////////////////////////////////////////////////
inline nlm::json properties::get(std::string_view const& k) const
{
  if (auto const pi(visitor_([&](auto& pi) noexcept
    {
      return pi.k == k;
    })); pi)
  {
    return pi->serializor();
  }
  else
  {
    return {};
  }
}

//////////////////////////////////////////////////////////////////////////////
inline nlm::json properties::state() const
{
  auto r(nlm::json::object());

  visitor_([&](auto& pi)
    {
      r.emplace(pi.k, pi.serializor());

      return false;
    }
  );

  return r;
}

//////////////////////////////////////////////////////////////////////////////
inline void properties::state(nlm::json const& e) const
{
  for (auto i(e.cbegin()), ecend(e.cend()); ecend != i; i = std::next(i))
  {
    auto& k(i.key());

    if (auto const pi(visitor_([&](auto& pi) noexcept
      {
        return pi.k == k;
      })); pi && pi->deserializor)
    {
      pi->deserializor(i.value());
    }
  }
}

#endif // PROPERTIES_HPP
