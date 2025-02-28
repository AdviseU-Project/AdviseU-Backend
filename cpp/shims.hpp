#ifndef SHIMS_HPP
#define SHIMS_HPP

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <variant>

#include "json.hpp"

namespace adviseu {
    namespace shims {
        using json = nlohmann::json;

        struct ScheduleRequest {
            std::string user_id;
            std::string degree_taking;
            std::vector<std::string> courses_taken;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ScheduleRequest, user_id, degree_taking, courses_taken);

        std::optional<ScheduleRequest> parse_schedule_request(std::string &request_body) {
            auto data = json::parse(request_body);
            
            try {
                return data.get<ScheduleRequest>();
            } catch(json::exception &e) {
                return {};
            }
        }
    }
}

#endif