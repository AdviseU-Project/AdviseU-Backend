#include <array>
#include <cstdlib>
#include <future>
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

#include "loader.hpp"
#include "shims.hpp"
#include "solver.hpp"

using namespace adviseu;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

const std::string path = "/api/generatePlan";
const std::string bypass = "/api/bypass/generatePlan";
const std::string health = "/health";

// Get port from environment variable with fallback to default
unsigned short getPort()
{
    const char* port_env = std::getenv("CONTAINER_PORT");
    if (port_env && *port_env) {
        try {
            return static_cast<unsigned short>(std::stoi(port_env));
        } catch (const std::exception& e) {
            std::cerr << "Invalid port in environment variable: " << e.what() << std::endl;
        }
    }
    return 8080; // Default port if environment variable is not set or invalid
}

// Get MongoDB URI from environment variable
std::string getMongoURI()
{
    const char* mongo_uri_env = std::getenv("MONGO_URI");
    if (mongo_uri_env && *mongo_uri_env) {
        std::cout << "Using MongoDB URI from environment variable" << std::endl;
        return mongo_uri_env;
    } else {
        std::cerr << "Error: MONGO_URI environment variable not set or empty" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

const auto uri = mongocxx::uri { getMongoURI() };
const auto api = mongocxx::options::server_api { mongocxx::options::server_api::version::k_version_1 };

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
        http::response<http::string_body> res { http::status::bad_request, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error = [&req](beast::string_view what) {
        http::response<http::string_body> res { http::status::internal_server_error, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // Health check endpoint - accept GET requests
    if (req.target() == health) {
        // Accept any HTTP method for the health endpoint
        http::response<http::string_body> res { http::status::ok, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = "OK";
        res.prepare_payload();
        return res;
    }

    // Make sure we can handle the method
    if (req.method() != http::verb::post) {
        return bad_request("Unknown HTTP-method");
    }

    // Request path must be absolute and not contain "..".
    if (req.target() != path && req.target() != bypass) {
        return bad_request("Illegal request-target");
    }

    auto body = shims::parse_schedule_request(req.body());

    if (!body) {
        return bad_request("Invalid request body");
    }

    mongocxx::options::client client_options;

    client_options.server_api_opts(api);

    mongocxx::client conn { uri, client_options };

    auto plan = [&req, &body, &conn]() {
        if (req.target() == bypass) {
            std::vector<Program> programs;

            std::cout << "Bypass: " << body->userId << " " << body->planId << std::endl;

            if (body->userId != "") {
                programs.push_back(*load_program_from_mongo(body->userId, conn));
            }

            if (body->planId != "") {
                programs.push_back(*load_program_from_mongo(body->planId, conn));
            }

            return std::optional { std::make_pair(programs, std::unordered_map<int, int>()) };
        } else {
            return load_user_plan_from_mongo(body->userId, body->planId, conn);
        }
    }();

    if (!plan) {
        return bad_request("Invalid plan/user combination");
    }

    std::cout << "Planning..." << std::endl;

    std::optional<std::array<std::vector<std::string>, term_count>> mapping = plan_schedule(plan->first, plan->second);

    if (!mapping) {
        return bad_request("Could not find valid plan");
    }

    if (req.target() == bypass) {
        // Respond to POST request
        http::response<http::string_body> res { http::status::ok, req.version() };
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/json");
        res.keep_alive(req.keep_alive());
        res.body() = "OK";
        res.prepare_payload();

        return res;
    }

    auto result = push_plan_to_mongo(body->userId, body->planId, *mapping, plan->second, conn);

    if (!result) {
        return server_error("Internal server error");
    }

    std::cout << "Sending..." << std::endl;

    // Respond to POST request
    http::response<http::string_body> res { http::status::ok, req.version() };
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/json");
    res.keep_alive(req.keep_alive());
    res.body() = "OK";
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

    // Set the timeout.
    stream.expires_after(std::chrono::seconds(60));

    // Read a request
    http::request<http::string_body> req;
    co_await http::async_read(stream, buffer, req);

    // Handle the request
    auto res = handle_request(std::move(req));

    // Send the response
    co_await beast::async_write(stream, std::move(res));

    // Send a TCP shutdown
    stream.socket().shutdown(net::ip::tcp::socket::shutdown_send);
}

// Accepts incoming connections and launches the sessions
net::awaitable<void>
do_listen(
    net::ip::tcp::endpoint endpoint)
{
    auto executor = co_await net::this_coro::executor;
    auto acceptor = net::ip::tcp::acceptor { executor, endpoint };

    for (;;) {
        net::co_spawn(
            executor,
            do_session(beast::tcp_stream { co_await acceptor.async_accept() }),
            [](std::exception_ptr e) {
                if (e) {
                    try {
                        std::rethrow_exception(e);
                    } catch (std::exception const& e) {
                        std::cerr << "Error in session: " << e.what() << "\n";
                    }
                }
            });
    }
}

int main(void)
{
    try {
        mongocxx::instance inst {};
        mongocxx::options::client client_options;

        client_options.server_api_opts(api);

        mongocxx::client conn { uri, client_options };

        ping_mongo(conn);
        load_courses_from_mongo(conn);

        auto bacc = load_program_from_mongo("Baccalaureate Core", conn);

        if (!bacc) {
            std::cout << "Could not load Bacc Core" << std::endl;
            return 0;
        }

        core = *bacc;

        std::cout << "Courses Loaded!" << std::endl;

        // The io_context is required for all I/O
        net::io_context ioc { std::thread::hardware_concurrency() / 2 };

        std::cout << "Concurrency: " << std::thread::hardware_concurrency() << std::endl;

        auto const address = net::ip::make_address("0.0.0.0");
        auto const port = getPort();

        // Spawn a listening port
        net::co_spawn(
            ioc,
            do_listen(net::ip::tcp::endpoint { address, port }),
            [](std::exception_ptr e) {
                if (e) {
                    try {
                        std::rethrow_exception(e);
                    } catch (std::exception const& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                    }
                }
            });

        std::cout << "Server started at 0.0.0.0:" << port << std::endl;
        std::cout << "Endpoints available:" << std::endl;
        std::cout << "  - " << path << " (main API endpoint)" << std::endl;
        std::cout << "  - " << bypass << " (testing bypass endpoint)" << std::endl;
        std::cout << "  - " << health << " (health check endpoint)" << std::endl;

        // Run the I/O service on the requested number of threads
        std::vector<std::thread> v;
        v.reserve(std::thread::hardware_concurrency() / 2 - 1);
        for(auto i = std::thread::hardware_concurrency() / 2 - 1; i > 0; --i) {
            v.emplace_back([&ioc] { ioc.run(); });
        }

        ioc.run();
    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
