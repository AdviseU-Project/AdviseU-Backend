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
            std::string userId;
            std::string planId;
        };

        struct ScheduleResponse {
            std::array<std::vector<std::string>, term_count> term_course_mapping;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ScheduleRequest, userId, planId);
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