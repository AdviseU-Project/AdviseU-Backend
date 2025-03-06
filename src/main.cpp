#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <sys/resource.h>

#include "globals.hpp"
#include "loader.hpp"
#include "shims.hpp"
#include "solver.hpp"

using namespace adviseu;

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;

const std::string path = "/api/generatePlan";
const auto uri = mongocxx::uri{""};
const auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};

// Return a response for the given request.
//
// The concrete type of the response message (which depends on the
// request), is type-erased in message_generator.
template <class Body, class Allocator>
http::message_generator
handle_request(
    http::request<Body, http::basic_fields<Allocator>>&& req)
{
    // Returns a bad request response
    auto const bad_request = [&req](beast::string_view why) {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        
        return res;
    };

    // Returns a server error response
    auto const server_error = [&req](beast::string_view what) {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if(req.method() != http::verb::post) {
        return bad_request("Unknown HTTP-method");
    }

    // Request path must be absolute and not contain "..".
    if(req.target() != path) {
        return bad_request("Illegal request-target");
    }

    auto body = shims::parse_schedule_request(req.body());

    if(!body) {
        return bad_request("Invalid request body");
    }

    mongocxx::options::client client_options;

    client_options.server_api_opts(api);

    mongocxx::client conn{uri, client_options};

    auto program = load_program_from_mongo(body->degree_taking, conn);

    if(!program) {
        return bad_request("Invalid degree option");
    }

    std::optional<std::array<std::vector<std::string>, term_count>> mapping = plan_schedule(*program, body->courses_taken);

    if(!mapping) {
        return bad_request("Could not find valid plan");
    }

    auto response = shims::serialize_schedule_response(shims::ScheduleResponse{*mapping});

    if(!response) {
        return server_error("Internal server error");
    }

    std::cout << "Sending..." << std::endl;
    
    // Respond to POST request
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/json");
    res.keep_alive(req.keep_alive());
    res.body() = *response;
    res.prepare_payload();

    return res;
}

// Handles an HTTP server connection
net::awaitable<void>
do_session(
    beast::tcp_stream stream)
{
    // This buffer is required to persist across reads
    beast::flat_buffer buffer;

    for(;;) {
        // Set the timeout.
        stream.expires_after(std::chrono::seconds(15));

        // Read a request
        http::request<http::string_body> req;
        co_await http::async_read(stream, buffer, req);

        // Handle the request
        http::message_generator msg = handle_request(std::move(req));

        // Determine if we should close the connection
        bool keep_alive = msg.keep_alive();

        // Send the response
        co_await beast::async_write(stream, std::move(msg));

        if(!keep_alive) {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            break;
        }
    }

    // Send a TCP shutdown
    stream.socket().shutdown(net::ip::tcp::socket::shutdown_send);
}

// Accepts incoming connections and launches the sessions
net::awaitable<void>
do_listen(
    net::ip::tcp::endpoint endpoint)
{
    auto executor = co_await net::this_coro::executor;
    auto acceptor = net::ip::tcp::acceptor{executor, endpoint};

    for(;;) {
        net::co_spawn(
            executor,
            do_session(
                beast::tcp_stream{ co_await acceptor.async_accept() }),
            [](std::exception_ptr e) {
                if(e) {
                    try {
                        std::rethrow_exception(e);
                    } catch(std::exception const& e) {
                        std::cerr << "Error in session: " << e.what() << "\n";
                    }
                }
            });
    }
}

int main(void) {
    try {
        mongocxx::instance inst{};
        mongocxx::options::client client_options;

        client_options.server_api_opts(api);

        mongocxx::client conn{uri, client_options};

        ping_mongo(conn);
        load_courses_from_mongo(conn);

        auto bacc = load_program_from_mongo("-1", conn);

        if(!bacc) {
            std::cout << "Could not load Bacc Core" << std::endl;
            return 0;
        }

        core = *bacc;

        std::cout << "Courses Loaded!" << std::endl;

        // The io_context is required for all I/O
        net::io_context ioc{ 1 };
        
        auto const address = net::ip::make_address("127.0.0.1");
        auto const port = static_cast<unsigned short>(std::atoi("8080"));

        // Spawn a listening port
        net::co_spawn(
            ioc,
            do_listen(net::ip::tcp::endpoint{address, port}),
            [](std::exception_ptr e) {
                if(e) {
                    try {
                        std::rethrow_exception(e);
                    } catch(std::exception const& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                    }
                }
            });

        ioc.run();
    } catch(std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
