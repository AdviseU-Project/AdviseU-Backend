#include <iostream>
#include <unordered_set>

#include <sys/resource.h>

#include "globals.hpp"
#include "loader.hpp"
#include "solver.hpp"

using namespace adviseu;

bool set_stack_limit_1gb() {
    constexpr rlim_t stack_size = 1025 * 1024 * 1024;  // 256 MB in bytes
    struct rlimit rl;
    
    // Get current limits
    if (getrlimit(RLIMIT_STACK, &rl) != 0) {
        std::cerr << "Error: Could not get current stack limit\n";
        return false;
    }
    
    // Set new limit
    rl.rlim_cur = stack_size;  // Set soft limit
    if (rl.rlim_max < stack_size) {
        rl.rlim_max = stack_size;  // Increase hard limit if necessary
    }
    
    if (setrlimit(RLIMIT_STACK, &rl) != 0) {
        std::cerr << "Error: Could not set stack limit to 256MB\n";
        return false;
    }
    
    return true;
}

int main(void) {  
    if (set_stack_limit_1gb()) {
        std::cout << "Successfully set stack limit to 1GB\n";
    } else {
        std::cout << "Failed to set stack limit\n";
        return 1;
    }

    mongocxx::instance inst{};
    mongocxx::options::client client_options;

    const auto uri = mongocxx::uri{"mongodb+srv://sebastianDev:IVKIJcpGOlIIpU0a@adviseu-dev-cluster.m3v7q.mongodb.net/?retryWrites=true&w=majority&appName=adviseu-dev-cluster"};
    const auto api = mongocxx::options::server_api{mongocxx::options::server_api::version::k_version_1};
    
    client_options.server_api_opts(api);

    mongocxx::client conn{uri, client_options};

    ping_mongo(conn);

    const std::string course_code = "810";

    load_courses_from_mongo(conn);
    Program program = load_program_from_mongo(course_code, conn, global_ids);

    for(const auto &global_id : global_ids) {
        global_names[global_id.second] = global_id.first;
    }

    for(int i = 0; i <= 18; i++) {
        for(const auto &global_id : global_ids) {
            term_course_map[i].insert(global_id.second);
        }

        term_map[std::to_string(i)] = i;
    }

    global_names[-1] = "N/A";

    int idc = 0;
    for(const auto &global_id : global_ids) {
        std::cout << global_id.first << " -> " << global_id.second << std::endl;

        if(++idc >= 10) {
            break;
        }
    }

    idc = 0;
    for(const auto &course_req : course_reqs) {
        if(global_names[idc].substr(0, 4) != "CS 1") {
            idc++;
            continue;
        }

        std::cout << "Course: " << idc << " -> " << global_names[idc] << std::endl;

        std::cout << "\tRequirements: ";
        for(const auto &req_group : course_req.first) {
            std::cout << "[";

            for(const auto &req : req_group) {
                std::cout << "(" << req << " " << global_names[req] << ") ";
            }

            std::cout << "] ";
        }

        std::cout << std::endl;
        std::cout << "\tCo-Requirements: ";
        for(const auto &req_group : course_req.second) {
            std::cout << "[";

            for(const auto &req : req_group) {
                std::cout << req << " ";
            }

            std::cout << "] ";
        }

        std::cout << std::endl;
        
        idc++;

        if(++idc >= 10) {
            break;
        }
    }

    std::cout << program.name << std::endl; 
    std::cout << program.code << std::endl; 

    for(const auto &requirement : program.requirements) {
        std::cout << "Requirement: " << requirement.req_series << std::endl;

        int series_idx = 0;
        for(const auto &series: requirement.series) {
            std::cout << "\tSeries: " << series_idx++ << std::endl;

            int option_idx = 0;
            for(const auto &option : series) {
                std::cout << "\t\tOption: " << option_idx++ << std::endl;

                int course_idx = 0;
                for(const auto &course : option) {
                    std::cout << "\t\t\tCourse: " << course_idx++ << " . " << global_names[course] << std::endl;
                }
            }
        }
    }

    // invoke();

    return 0;
}