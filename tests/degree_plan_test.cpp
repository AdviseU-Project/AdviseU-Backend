#include <gtest/gtest.h>

#include "loader.hpp"
#include "solver.hpp"

using namespace adviseu;

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

    std::vector<DegreeCombination> combinations;

    // Find all major degrees
    auto cursor = collection.find(make_document());
    for (auto&& doc : cursor) {
        std::string degreeName { doc["program_name"].get_string().value };

        // If it's a major degree
        if (degreeName.find("Major") != std::string::npos) {
            // Add the major by itself
            combinations.push_back({ degreeName, "" });

            // Check if it has options
            if (doc.find("options") != doc.end() && doc["options"].type() == bsoncxx::type::k_array) {
                auto options = doc["options"].get_array().value;

                // Add combinations of major with each option
                for (auto&& option : options) {
                    std::string optionName { option.get_string().value };
                    combinations.push_back({ degreeName, optionName });
                }
            }
        }
    }

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