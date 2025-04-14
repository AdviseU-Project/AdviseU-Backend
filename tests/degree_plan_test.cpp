#include <gtest/gtest.h>

#include "loader.hpp"
#include "solver.hpp"

using namespace adviseu;

std::string getMongoURI() {
    const char* mongo_uri_env = std::getenv("MONGO_URI");
    if (mongo_uri_env && *mongo_uri_env) {
        std::cout << "Using MongoDB URI from environment variable" << std::endl;
        return mongo_uri_env;
    } else {
        std::cerr << "Error: MONGO_URI environment variable not set or empty" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

const auto uri = mongocxx::uri{getMongoURI()};
const auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};

std::vector<std::string> getMajorDegrees() {
    mongocxx::options::client client_options;

    client_options.server_api_opts(api);

    mongocxx::client conn{uri, client_options};

    auto db = conn["adviseu_db"];
    auto collection = db["degrees"];

    auto cursor = collection.find(make_document());
    std::vector<std::string> degrees;

    for(auto&& doc : cursor) {
        degrees.push_back(std::string{doc["program_name"].get_string().value});
    }
    
    std::erase_if(degrees, [](const auto &degree) { 
        return degree.find("Major") == std::string::npos; 
    });

    return degrees;
};  

class DegreePlanTestSuite : public testing::TestWithParam<std::string> {
public:
    Program program;
    mongocxx::client conn;
    
    void SetUp() override {
        mongocxx::options::client client_options;

        client_options.server_api_opts(api);

        conn = mongocxx::client{uri, client_options};

        auto program_q = load_program_from_mongo(GetParam(), conn);

        if(!program_q) {
            FAIL() << "Program not found";
        }

        program = *program_q;
    }
};

TEST_P(DegreePlanTestSuite, CanCreate) {
    std::vector<Program> programs{program};
    std::unordered_map<int, int> taken_courses;

    auto result = plan_schedule(programs, taken_courses);

    if(!result) {
        FAIL() << "Could not create schedule for this major";
    }
}

INSTANTIATE_TEST_SUITE_P(DegreeInstantiation, DegreePlanTestSuite, testing::ValuesIn(getMajorDegrees()));

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);

    mongocxx::instance inst{};
    mongocxx::options::client client_options;

    client_options.server_api_opts(api);

    mongocxx::client conn{uri, client_options};

    ping_mongo(conn);
    load_courses_from_mongo(conn);

    auto bacc = load_program_from_mongo("Baccalaureate Core", conn);

    if(!bacc) {
        std::cout << "Could not load Bacc Core" << std::endl;
        return 1;
    }

    core = *bacc;

    return RUN_ALL_TESTS();
}