#ifndef PROPERTIES_HPP
# define PROPERTIES_HPP
# pragma once

#include <array>

#include <functional>

class properties
{
  using serializor_t = std::function<nlm::json()>;
  using deserializor_t = std::function<void(nlm::json)>;

  struct property_info
  {
    std::string_view k;
    serializor_t serializor;
    deserializor_t deserializor;
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
  template <std::size_t I = 0,
    typename A = std::array<property_info, 0>, typename U>
  auto register_property(std::string_view k, U&& u, A&& a = {})
  {
    std::array<property_info, I + 1> b;

    if constexpr (bool(I))
    {
      std::move(a.begin(), a.end(), b.begin());
    }

    if constexpr (std::is_invocable_v<U>)
    {
      b.back() = {
        std::move(k),
        [=]()noexcept(noexcept(u()))->decltype(auto){return u();},
        {}
      };
    }
    else if constexpr (std::is_lvalue_reference_v<U>)
    {
      if constexpr (std::is_const_v<std::remove_reference_t<U>>)
      {
        b.back() = {
          std::move(k),
          [&]()noexcept->decltype(auto){return u;},
          {}
        };
      }
      else
      {
        b.back() = {
          std::move(k),
          [&]()noexcept->decltype(auto){return u;},
          [&](auto&& j)
          {
            if (!std::strcmp(nlm::json(u).type_name(), j.type_name()))
            {
              u = j.template get<std::remove_cvref_t<U>>();
            }
          }
        };
      }
    }

    return [this, b(std::move(b))](auto&& ...a) mutable
      {
        if constexpr (bool(sizeof...(a)))
        {
          return register_property<I + 1>(std::forward<decltype(a)>(a)...,
            std::move(b));
        }
        else
        {
          visitor_ = [b(std::move(b)), c(std::move(visitor_))](auto f)
            noexcept(noexcept(f({})))
            {
              for (auto& i: b)
              {
                if (f(i))
                {
                  return &i;
                }
              }

              return c ? c(std::move(f)) : typename A::const_pointer{};
            };
        }
      };
  }

  template <std::size_t I = 0,
    typename A = std::array<property_info, 0>, typename U, typename V,
    std::enable_if_t<
      std::is_invocable_v<U> &&
      std::is_invocable_v<V, nlm::json>,
      int
    > = 0
  >
  auto register_property(std::string_view k, U&& u, V&& v, A&& a = {})
  {
    std::array<property_info, I + 1> b;

    if constexpr (bool(I))
    {
      std::move(a.begin(), a.end(), b.begin());
    }

    b.back() = {
      std::move(k),
      [=]()noexcept(noexcept(u()))->decltype(auto){return u();},
      [=](auto&& j){v(std::forward<decltype(j)>(j));}
    };

    return [this, b(std::move(b))](auto&& ...a) mutable
      {
        if constexpr (bool(sizeof...(a)))
        {
          return register_property<I + 1>(std::forward<decltype(a)>(a)...,
            std::move(b));
        }
        else
        {
          visitor_ = [b(std::move(b)), c(std::move(visitor_))](auto f)
            noexcept(noexcept(f({})))
            {
              for (auto& i: b)
              {
                if (f(i))
                {
                  return &i;
                }
              }

              return c ? c(std::move(f)) : typename A::const_pointer{};
            };
        }
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
