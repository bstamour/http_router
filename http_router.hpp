#ifndef BST_HTTP_ROUTER_HPP_
#define BST_HTTP_ROUTER_HPP_

//------------------------------------------------------------------------------
//
// A route table to go along with http_listener objects. Given a
// reference to an http_listener, the route table injects its own
// methods into the listener that dispatch to functions based on the
// URL/HTTP method.
//
// Register your custom route functions using the register_routes()
// function.
//
// This library is designed to work in conjunction with Microsoft's
// Casablanca HTTP library.
//
//------------------------------------------------------------------------------

#include <cpprest/http_listener.h>

#include <string>
#include <utility>
#include <functional>
#include <tuple>
#include <regex>
#include <vector>

namespace bst {

namespace lst = web::http::experimental::listener;
namespace ht  = web::http;

//------------------------------------------------------------------------------

template <typename CharT> class http_router final {
public:
  using value_type  = CharT;
  using regex_type  = std::basic_regex<CharT>;
  using string_type = typename regex_type::string_type;
  using match_type  =
    std::match_results<typename string_type::const_iterator>;

  explicit http_router(lst::http_listener&);

  http_router()                                = delete;
  http_router(http_router const&)              = delete;
  http_router& operator = (http_router const&) = delete;

  http_router(http_router&&)                   = default;
  http_router& operator = (http_router&&)      = default;

  template <typename Str, typename Func, typename... Ts>
  void register_routes(Str&&, ht::method, Func&&, Ts&&...);
  void register_routes() { /* base case */ }

private:
  using callback   = std::function<void (ht::http_request)>;
  using route_item = std::tuple<regex_type, ht::method, callback>;

  lst::http_listener& l;
  std::vector<route_item> routing_table;

  template <typename Request>
  void handle_request(Request&&, ht::method);
};

//------------------------------------------------------------------------------

template <typename CharT>
http_router<CharT>::http_router(lst::http_listener& listener) : l(listener)
{
  using std::forward;
  using namespace ht;

  l.support(methods::GET,  [this](auto&& r) {
      this->handle_request(forward<decltype(r)>(r), methods::GET);  });
  l.support(methods::POST, [this](auto&& r) {
      this->handle_request(forward<decltype(r)>(r), methods::POST); });
  l.support(methods::PUT,  [this](auto&& r) {
      this->handle_request(forward<decltype(r)>(r), methods::PUT);  });
  l.support(methods::DEL,  [this](auto&& r) {
      this->handle_request(forward<decltype(r)>(r), methods::DEL);  });
}

template <typename CharT>
template <typename Str, typename Func, typename... Ts>
void http_router<CharT>::register_routes(Str&& route, ht::method method,
					 Func&& func, Ts&&... params)
{
  using std::forward;

  static_assert(
    sizeof... (Ts) % 3 == 0,
    "Number of parameters must be a multiple of three");

  this->routing_table.emplace_back(
    forward<Str>(route),
    method,
    forward<Func>(func));

  register_routes(forward<Ts>(params)...);
}

template <typename CharT>
template <typename Request>
void http_router<CharT>::handle_request(Request&& request, ht::method method)
{
  using std::forward;
  using ht::status_codes;

  for (auto const& p : routing_table) {
    auto const& r   = get<0>(p);
    auto const& m   = get<1>(p);
    auto const& f   = get<2>(p);
    auto const& uri = request.request_uri().to_string();

    if (m != method) continue;

    if (std::regex_match(uri, r))
      return f(forward<Request>(request));
  }

  forward<Request>(request).reply(
    status_codes::OK,
    "Could not find route in table.");
}

}

#endif
