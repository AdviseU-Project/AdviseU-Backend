#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <cstdlib>
#include <gtest/gtest.h>
#include <iostream>
#include <string>

#include "../src/shims.hpp"
#include "loader.hpp"

using namespace adviseu;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// Global variable to store sampling rate
int g_sampling_rate = 1; // Default: run all tests

// Structure to represent a degree combination for testing
struct DegreeCombination {
    std::string majorName;
    std::string optionName; // Empty string if testing just the major

    // For test case name generation
    std::string ToString() const
    {
        std::string result;
        if (optionName.empty()) {
            result = majorName;
        } else {
            result = majorName + "_with_" + optionName;
        }

        // Sanitize the test name - replace invalid characters with underscores
        for (char& c : result) {
            if (!std::isalnum(c) && c != '_') {
                c = '_';
            }
        }

        return result;
    }
};

// Custom printer for the test name - Overrides Google Test printing of test name
void PrintTo(const DegreeCombination& combo, std::ostream* os)
{
    *os << combo.ToString();
}

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

// Get backend service host from environment variable with fallback
std::string getBackendHost()
{
    const char* host_env = std::getenv("BACKEND_HOST");
    if (host_env && *host_env) {
        return host_env;
    }
    return "adviseu-backend"; // Default to the service name in docker-compose
}

// Get port from environment variable with fallback
unsigned short getBackendPort()
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

const auto uri = mongocxx::uri { getMongoURI() };
const auto api = mongocxx::options::server_api { mongocxx::options::server_api::version::k_version_1 };

// Function to make HTTP request to the bypass endpoint
bool makeBypassRequest(const std::string& userId, const std::string& planId)
{
    try {
        // Get the backend host and port
        std::string host = getBackendHost();
        unsigned short port = getBackendPort();

        std::cout << "Connecting to " << host << ":" << port << std::endl;

        // Set up the request
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Look up the domain name
        auto const results = resolver.resolve(host, std::to_string(port));

        // Make the connection on the IP address we get from a lookup
        stream.connect(results);

        // Set up an HTTP POST request message
        http::request<http::string_body> req { http::verb::post, "/api/bypass/generatePlan", 11 };
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");

        // Create the JSON payload
        std::string body = "{\"userId\":\"" + userId + "\",\"planId\":\"" + planId + "\"}";
        req.body() = body;
        req.prepare_payload();

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::string_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error { ec };

        // Check if we got a 200 OK response
        return res.result() == http::status::ok;
    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

// Return combinations of majors and their options
std::vector<DegreeCombination> getDegreeCombinations()
{
    mongocxx::options::client client_options;
    client_options.server_api_opts(api);
    mongocxx::client conn { uri, client_options };

    auto db = conn["adviseu_db"];
    auto collection = db["degrees"];

    // First collect all majors to determine sampling
    std::vector<std::string> allMajors;
    auto cursor = collection.find(make_document());
    for (auto&& doc : cursor) {
        std::string degreeName { doc["program_name"].get_string().value };
        if (degreeName.find("Major") != std::string::npos) {
            allMajors.push_back(degreeName);
        }
    }

    std::vector<DegreeCombination> combinations;

    // Process every Nth major based on sampling rate
    for (size_t i = 0; i < allMajors.size(); i++) {
        // Only process if it's at a sampled index (0, N, 2N, 3N, etc.)
        if (i % g_sampling_rate == 0) {
            std::string majorName = allMajors[i];

            // Need to query the database again to get the options
            auto majorDoc = collection.find_one(make_document(kvp("program_name", majorName)));
            if (!majorDoc) {
                continue;
            }

            // Add the major by itself
            combinations.push_back({ majorName, "" });

            // Check if it has options
            auto doc = majorDoc.value();
            if (doc.find("options") != doc.end() && doc["options"].type() == bsoncxx::type::k_array) {
                auto options = doc["options"].get_array().value;

                // Add combinations of major with each option
                for (auto&& option : options) {
                    std::string optionName { option.get_string().value };
                    combinations.push_back({ majorName, optionName + " Option" });
                }
            }
        }
    }

    std::cout << "Testing " << combinations.size() << " degree combinations "
              << "(sampling rate: " << g_sampling_rate << ")" << std::endl;

    return combinations;
}

class DegreePlanTestSuite : public testing::TestWithParam<DegreeCombination> {
public:
    void SetUp() override
    {
    }
};

TEST_P(DegreePlanTestSuite, CanCreate)
{
    const auto& param = GetParam();

    // For testing through the API, we're using the program names directly
    bool result = false;

    if (param.optionName.empty()) {
        // If testing just a major, send only the major name as userId
        result = makeBypassRequest(param.majorName, "");
    } else {
        // If testing a major with an option, send both
        result = makeBypassRequest(param.majorName, param.optionName);
    }

    if (!result) {
        if (param.optionName.empty()) {
            FAIL() << "HTTP request failed for major: " << param.majorName;
        } else {
            FAIL() << "HTTP request failed for major: " << param.majorName
                   << " with option: " << param.optionName;
        }
    }
}

INSTANTIATE_TEST_SUITE_P(DegreeInstantiation, DegreePlanTestSuite, testing::ValuesIn(getDegreeCombinations()));

int main(int argc, char** argv)
{
    // Check for environment variable
    const char* sampling_env = std::getenv("SAMPLING_RATE");
    if (sampling_env && *sampling_env) {
        try {
            g_sampling_rate = std::stoi(sampling_env);
            std::cout << "Using sampling rate from environment: " << g_sampling_rate << std::endl;

            // Validate the sampling rate
            if (g_sampling_rate <= 0) {
                std::cerr << "Sampling rate must be positive, using default (1)" << std::endl;
                g_sampling_rate = 1;
            }
        } catch (const std::exception& e) {
            std::cerr << "Invalid SAMPLING_RATE environment variable: " << sampling_env << std::endl;
            std::cerr << "Using default sampling rate (1)" << std::endl;
            g_sampling_rate = 1;
        }
    } else {
        std::cout << "No SAMPLING_RATE environment variable found, using default (1)" << std::endl;
    }

    std::cout << "Final sampling rate: " << g_sampling_rate << std::endl;

    testing::InitGoogleTest(&argc, argv);

    // Wait for the backend service to be ready
    std::cout << "Waiting for backend service to be ready..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return RUN_ALL_TESTS();
}
