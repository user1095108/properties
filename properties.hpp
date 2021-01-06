#ifndef PROPERTIES_HPP
# define PROPERTIES_HPP
# pragma once

#include <array>

#include <functional>

#include <type_traits>

#include <utility>

#include "json.hpp"

class properties
{
  using json = nlohmann::json;

  template <typename U>
  using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<U>>;

  template <class T, std::size_t N, std::size_t ...I>
  constexpr auto to_array_impl(T (&&a)[N], std::index_sequence<I...>)
  {
    return std::array<std::remove_cv_t<T>, N>{{std::move(a[I])...}};
  }
 
  template <class T, std::size_t N>
  constexpr auto to_array(T (&&a)[N])
  {
    return to_array_impl(std::move(a), std::make_index_sequence<N>{});
  }

  using serializor_t = std::function<json()>;
  using deserializor_t = std::function<void(json)>;

  struct property_info
  {
    std::string_view k;
    serializor_t serializor;
    deserializor_t deserializor;

    template <std::size_t N, typename U,
      typename std::enable_if_t<
        std::is_lvalue_reference_v<U> &&
        !std::is_const_v<std::remove_reference_t<U>>,
        int
      > = 0
    >
    property_info(char const (&key)[N], U&& u):
      k(key, N - 1),
      serializor([&]()noexcept->decltype(auto){return u;}),
      deserializor([&](auto&& j)
        {
          if (!std::strcmp(json(u).type_name(), j.type_name()))
          {
            u = j.template get<remove_cvref_t<U>>();
          }
        }
      )
    {
      static_assert(N > 1);
    }

    template <std::size_t N, typename U,
      typename std::enable_if_t<
        std::is_lvalue_reference_v<U> &&
        std::is_const_v<std::remove_reference_t<U>>,
        int
      > = 0
    >
    property_info(char const (&key)[N], U&& u):
      k(key, N - 1),
      serializor([&]()noexcept->decltype(auto){return u;})
    {
      static_assert(N > 1);
    }

    template <std::size_t N, typename U,
      typename std::enable_if_t<
        std::is_invocable_v<U>,
        int
      > = 0
    >
    property_info(char const (&key)[N], U&& u):
      k(key, N - 1),
      serializor([=]()noexcept(noexcept(u()))->decltype(auto){return u();})
    {
      static_assert(N > 1);
    }

    template <std::size_t N, typename U, typename V,
      typename std::enable_if_t<
        std::is_invocable_v<U> &&
        std::is_invocable_v<V, json>,
        int
      > = 0
    >
    property_info(char const (&key)[N], U&& u, V&& v):
      k(key, N - 1),
      serializor([=]()noexcept(noexcept(u()))->decltype(auto){return u();}),
      deserializor([=](auto&& j){v(std::forward<decltype(j)>(j));})
    {
      static_assert(N > 1);
    }
  };

  std::function<property_info const*(
    std::function<bool(property_info const&)>
  )> visitor_;

public:
  properties() = default;
  virtual ~properties() = default;

  properties(properties const&) = delete;
  properties(properties&&) = delete;

  properties& operator=(properties const&) = delete;
  properties& operator=(properties&&) = delete;

  //
  auto state() const;
  void state(json const&) const;

  //
  template <std::size_t N>
  auto register_properties(property_info (&&l)[N])
  {
    visitor_ = [b(to_array(std::move(l))), c(std::move(visitor_))](auto f)
      noexcept(noexcept(f(std::declval<property_info const&>()))) ->
      auto const*
      {
        for (auto& i: b)
        {
          if (f(i))
          {
            return &i;
          }
        }

        return c ? c(std::move(f)) : nullptr;
      };
  }

  //
  bool exists(std::string_view const& k) const
  {
    return !get(k).is_null();
  }

  template <std::size_t N>
  auto exists(char const (&k)[N]) const
  {
    return exists({k, N - 1});
  }

  json get(std::string_view const&) const;

  template <std::size_t N>
  auto get(char const (&k)[N]) const
  {
    return get({k, N - 1});
  }

  template <typename U>
  auto set(std::string_view const& k, U&& u) const
  {
    if (auto const pi(visitor_([&](auto& pi) noexcept
      {
        return pi.k == k;
      })); pi && pi->deserializor)
    {
      pi->deserializor(std::forward<U>(u));

      property_changed(k);
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

  virtual void property_changed(std::string_view const&) const;
};

//////////////////////////////////////////////////////////////////////////////
inline properties::json properties::get(std::string_view const& k) const
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
    return json{};
  }
}

//////////////////////////////////////////////////////////////////////////////
inline auto properties::state() const
{
  auto r(json::object());

  visitor_([&](auto& pi)
    {
      if ('_' != pi.k.front())
      {
        r.emplace(pi.k, pi.serializor());
      }

      return false;
    }
  );

  return r;
}

//////////////////////////////////////////////////////////////////////////////
inline void properties::state(json const& e) const
{
  for (auto& i: e.items())
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

//////////////////////////////////////////////////////////////////////////////
inline void properties::property_changed(std::string_view const&) const
{
}

#endif // PROPERTIES_HPP
