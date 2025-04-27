#include <gtest/gtest.h>

#include "loader.hpp"
#include "solver.hpp"

using namespace adviseu;

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
                    combinations.push_back({ majorName, optionName });
                }
            }
        }
    }

    std::cout << "Testing " << combinations.size() << " degree combinations "
              << "(sampling rate: " << g_sampling_rate << ")" << std::endl;

    return combinations;
}

// Custom printer for the test name
struct DegreeCombinationPrinter {
    template <typename ParamType>
    std::string operator()(const testing::TestParamInfo<ParamType>& info) const
    {
        return info.param.ToString();
    }
};

class DegreePlanTestSuite : public testing::TestWithParam<DegreeCombination> {
public:
    std::vector<Program> programs;
    mongocxx::client conn;

    void SetUp() override
    {
        mongocxx::options::client client_options;
        client_options.server_api_opts(api);
        conn = mongocxx::client { uri, client_options };

        const auto& param = GetParam();

        // Always load the major program
        auto major_program = load_program_from_mongo(param.majorName, conn);
        if (!major_program) {
            FAIL() << "Major program not found: " << param.majorName;
        }
        programs.push_back(*major_program);

        // If we have an option, load that too
        if (!param.optionName.empty()) {
            auto option_program = load_program_from_mongo(param.optionName, conn);
            if (!option_program) {
                FAIL() << "Option program not found: " << param.optionName;
            }
            programs.push_back(*option_program);
        }
    }
};

TEST_P(DegreePlanTestSuite, CanCreate)
{
    std::unordered_map<int, int> taken_courses;

    auto result = plan_schedule(programs, taken_courses);

    if (!result) {
        const auto& param = GetParam();
        if (param.optionName.empty()) {
            FAIL() << "Could not create schedule for major: " << param.majorName;
        } else {
            FAIL() << "Could not create schedule for major: " << param.majorName
                   << " with option: " << param.optionName;
        }
    }
}

INSTANTIATE_TEST_SUITE_P(DegreeInstantiation, DegreePlanTestSuite, testing::ValuesIn(getDegreeCombinations()), DegreeCombinationPrinter {});

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

    mongocxx::instance inst {};
    mongocxx::options::client client_options;

    client_options.server_api_opts(api);

    mongocxx::client conn { uri, client_options };

    ping_mongo(conn);
    load_courses_from_mongo(conn);

    auto bacc = load_program_from_mongo("Baccalaureate Core", conn);

    if (!bacc) {
        std::cout << "Could not load Bacc Core" << std::endl;
        return 1;
    }

    core = *bacc;

    return RUN_ALL_TESTS();
}
