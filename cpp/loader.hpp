#ifndef LOADER_HPP
#define LOADER_HPP

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "globals.hpp"
#include "json.hpp"
#include "solver.hpp"

using namespace adviseu;

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

using json = nlohmann::json;

void 
ping_mongo(
    mongocxx::client &conn) 
{
    try {
        mongocxx::database db = conn["admin"];

        // Ping the database.
        const auto ping_cmd = bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("ping", 1));
        db.run_command(ping_cmd.view());

        std::cout << "Pinged your deployment. You successfully connected to MongoDB!" << std::endl;
    } catch (const std::exception& e) {
        std::cout<< "Mongo Exception: " << e.what() << std::endl;
    }
}

void 
load_course_requirements_from_vector(
    std::vector<std::string> &course_jsons) 
{
    int global_id = 0;

    for(const auto &course_json : course_jsons) {
        json course(json::parse(course_json));

        global_ids[course["course_number"].get<std::string>()] = global_id++;
    }

    std::map<std::string, std::unordered_set<int>> term_str_map;

    for(const auto &course_json : course_jsons) {
        json course(json::parse(course_json));
        course_reqs.push_back({std::vector<std::vector<int>>(), std::vector<std::vector<int>>()});

        auto pre_reqs = course["prerequisites"];

        for(const auto &pre_req : pre_reqs) {
            std::vector<std::string> req_groups = pre_req.get<std::vector<std::string>>();

            course_reqs.back().first.push_back(Option());

            std::transform(req_groups.begin(), req_groups.end(), std::back_inserter(course_reqs.back().first.back()), 
                [](const auto &a) { 
                    const auto ptr = global_ids.find(a);

                    if(ptr != global_ids.end()) {
                        return ptr->second;
                    } else {
                        return -1;
                    }
                });

            course_reqs.back().first.back().erase(
                std::remove(
                    course_reqs.back().first.back().begin(), 
                    course_reqs.back().first.back().end(), 
                    -1
                ), 
                course_reqs.back().first.back().end()
            );
        }

        auto co_reqs = course["corequisites"];

        for(const auto &co_req : co_reqs) {
            std::vector<std::string> req_groups = co_req.get<std::vector<std::string>>();

            course_reqs.back().second.push_back(Option());

            std::transform(req_groups.begin(), req_groups.end(), std::back_inserter(course_reqs.back().second.back()), 
                [](const auto &a) { 
                    const auto ptr = global_ids.find(a);

                    if(ptr != global_ids.end()) {
                        return ptr->second;
                    } else {
                        return -1;
                    }
                });
            
            course_reqs.back().second.back().erase(
                std::remove(
                    course_reqs.back().second.back().begin(), 
                    course_reqs.back().second.back().end(), 
                    -1
                ), 
                course_reqs.back().second.back().end()
            );
        }

        auto offers = course["offerings"];

        for(const auto &offer : offers) {
            int count = offer["count"].get<int>();
            std::string term = offer["srcdb"].get<std::string>();

            if(count) {
                term_str_map[term].insert(global_ids[course["course_number"].get<std::string>()]);
            }
        }
    }

    int term_idx = 1;
    for(const auto& term : term_str_map) {
        term_course_map[term_idx++].insert(term.second.begin(), term.second.end());
    }

    for(; term_idx < term_course_map.size(); term_idx++) {
        term_course_map[term_idx].insert(term_course_map[term_idx - 3].begin(), term_course_map[term_idx - 3].end());
    }
}

void 
load_courses_from_mongo(
    mongocxx::client &conn)
{
    auto db = conn["adviseu_db"];
    auto collection = db["courses"];

    auto cursor = collection.find(make_document());
    std::vector<std::string> courses;

    for(auto&& doc : cursor) {
        courses.push_back(bsoncxx::to_json(doc));
    }

    load_course_requirements_from_vector(courses);

    for(const auto &global_id : global_ids) {
        global_names[global_id.second] = global_id.first;
        term_course_map[0].insert(global_id.second);
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

        idc++;

        std::cout << std::endl;
    }
}

Program 
load_program_from_string(
    std::string content)
{
    json data(json::parse(content));
    Program program;

    program.name = data["program_name"];
    program.code = std::stoi(data["program_code"].get<std::string>());
    
    auto reqs = data["requirements"];

    for(const auto &req : reqs) {
        program.requirements.push_back(Requirement{
            std::stoi(req["amount"].get<std::string>()),
            std::vector<Series>()
        });

        auto seriess = req["series"];

        for(const auto &series : seriess) {
            auto options = series["options"];

            program.requirements.back().series.push_back(Series());

            for(const auto &option : options) {
                auto courses = option["courses"].get<std::vector<std::string>>();

                program.requirements.back().series.back().push_back(Option());

                std::transform(courses.begin(), courses.end(), 
                    std::back_inserter(program.requirements.back().series.back().back())
                    , [](const auto &a) { 
                        const auto ptr = global_ids.find(a);

                        if(ptr != global_ids.end()) {
                            return ptr->second;
                        } else {
                            return -1;
                        }
                    });
            }
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

    return program;
}

std::optional<Program>
load_program_from_mongo(
    std::string code, 
    mongocxx::client &conn)
{
    auto db = conn["adviseu_db"];
    auto collection = db["degrees"];

    auto result = collection.find_one(make_document(kvp("program_code", code)));

    if(!result) {
        return {};
    } else {
        return load_program_from_string(bsoncxx::to_json(result.value()));
    }
}

#endif