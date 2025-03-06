#ifndef SHIMS_HPP
#define SHIMS_HPP

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include "globals.hpp"
#include "json.hpp"

namespace adviseu {
    namespace shims {
        using json = nlohmann::json;

        struct ScheduleRequest {
            std::string user_id;
            std::string degree_taking;
            std::unordered_set<int> courses_taken;
        };

        struct ScheduleResponse {
            std::array<std::vector<std::string>, term_count> term_course_mapping;
        };

        inline void from_json(const nlohmann::json& nlohmann_json_j, ScheduleRequest& nlohmann_json_t) { 
            nlohmann_json_j.at("user_id").get_to(nlohmann_json_t.user_id); 
            nlohmann_json_j.at("degree_taking").get_to(nlohmann_json_t.degree_taking); 

            auto courses = nlohmann_json_j.at("courses_taken").get<std::vector<std::string>>();

            std::transform(
                courses.begin(), courses.end(), 
                std::inserter(nlohmann_json_t.courses_taken, nlohmann_json_t.courses_taken.begin()), 
                [](const auto &v) { return global_ids[v]; });
        }

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ScheduleResponse, term_course_mapping);

        std::optional<ScheduleRequest> parse_schedule_request(std::string &request_body) {
            auto data = json::parse(request_body);
            
            try {
                return data.get<ScheduleRequest>();
            } catch(json::exception &e) {
                return {};
            }
        }

        std::optional<std::string> serialize_schedule_response(ScheduleResponse &&response) {
            json data = response;
            
            try {
                return data.dump();
            } catch(json::exception &e) {
                return {};
            }
        }
    }
}

#endif