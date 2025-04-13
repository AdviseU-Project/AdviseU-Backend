#ifndef LOADER_HPP
#define LOADER_HPP

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
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
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::finalize;

using json = nlohmann::json;

const int TAKEN = -100;
const int term_offset = 1;
const int base_year = 2025;
const std::vector<std::string> term_rotation{"Spring ", "Fall ", "Winter "};

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

    // std::sort(course_jsons.begin(), course_jsons.end(), [](const auto &a_json, const auto &b_json) {
    //     json a(json::parse(a_json));
    //     json b(json::parse(b_json));

    //     std::string a_code = a["course_number"].get<std::string>();
    //     std::string b_code = b["course_number"].get<std::string>();
        
    //     return a_code.substr(a_code.find_first_of(" ")) < b_code.substr(b_code.find_first_of(" "));
    // });

    for(const auto &course_json : course_jsons) {
        json course(json::parse(course_json));

        global_ids[course["course_number"].get<std::string>()] = global_id++;
    }

    for(const auto &global_id : global_ids) {
        global_names[global_id.second] = global_id.first;
        term_course_map[0].insert(global_id.second);
    }

    global_names[-1] = "N/A";

    std::map<std::string, std::unordered_set<int>> term_str_map;

    for(const auto &course_json : course_jsons) {
        json course(json::parse(course_json));
        course_reqs.push_back({std::vector<std::vector<int>>(), std::vector<std::vector<int>>()});

        auto pre_reqs = course["prerequisites"];

        for(const auto &pre_req : pre_reqs) {
            std::vector<std::string> req_groups = pre_req.get<std::vector<std::string>>();

            course_reqs.back().first.push_back(Option());

            std::transform(req_groups.begin(), req_groups.end(), std::back_inserter(course_reqs.back().first.back()), 
                [&course](const auto &a) { 
                    const auto ptr = global_ids.find(a);
                    const auto id = global_ids[course["course_number"].get<std::string>()];

                    if(ptr != global_ids.end()) {
                        if(ptr->second < id) {
                            for(auto &pcr : course_reqs[ptr->second].first) {
                                auto eptr = std::remove(pcr.begin(), pcr.end(), id);

                                if(eptr != pcr.end()) {
                                    std::cout << "Loop: " << global_names[ptr->second] << " " << ptr->second << " - " << global_names[id] << " " << id << std::endl;

                                    pcr.erase(eptr, pcr.end());

                                    return -1;
                                }
                            }
                        }

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
                [&course](const auto &a) { 
                    const auto ptr = global_ids.find(a);
                    const auto id = global_ids[course["course_number"].get<std::string>()];

                    if(ptr != global_ids.end()) {
                        if(ptr->second < id) {
                            for(auto &pcr : course_reqs[ptr->second].second) {
                                auto eptr = std::remove(pcr.begin(), pcr.end(), id);

                                if(eptr != pcr.end()) {
                                    std::cout << "Loop: " << global_names[ptr->second] << " " << ptr->second << " - " << global_names[id] << " " << id << std::endl;

                                    pcr.erase(eptr, pcr.end());

                                    return -1;
                                }
                            }
                        }

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
                // assert(course["course_number"].get<std::string>() != "ENGR 103");
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

    int elec_creds = 0;
    std::unordered_map<std::string, int> code_cnts;

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

                if(courses[0].find("Option") != courses[0].npos || courses[0].find("Elective") != courses[0].npos) {
                    elec_creds += std::stoi(courses[1]);
                    continue;
                } else if(global_ids.find(courses[0]) == global_ids.end()) {
                    continue;
                }

                for(const auto &course : courses) {
                    if(
                        term_course_map[1].find(global_ids[course]) == term_course_map[1].end() &&
                        term_course_map[2].find(global_ids[course]) == term_course_map[2].end() &&
                        term_course_map[3].find(global_ids[course]) == term_course_map[3].end()) {
                            for(int i = 1; i < term_course_map.size(); i++) {
                                term_course_map[i].insert(global_ids[course]);
                            }
                    }
                }

                code_cnts[courses[0].substr(0, courses[0].find(" ") + 1)]++;

                program.requirements.back().series.back().push_back(Option());

                for(const auto &course : courses) {
                    const auto ptr = global_ids.find(course);

                    if(ptr != global_ids.end()) {
                        program.requirements.back().series.back().back().push_back(ptr->second);
                    }
                }

                if(program.requirements.back().series.back().back().size() == 0) {
                    program.requirements.back().series.back().pop_back();
                }
            }

            if(program.requirements.back().series.back().size() == 0) {
                program.requirements.back().series.pop_back();
            }
        }

        if(program.requirements.back().series.size() == 0) {
            program.requirements.pop_back();
        }
    }

    if(elec_creds) {
        std::string best = "";

        for(const auto &c : code_cnts) {
            if(best == "" || c.second > code_cnts[best]) {
                best = c.first;
            }
        }

        std::cout << "Best: " << best << std::endl;

        program.requirements.push_back(Requirement{
            elec_creds / 4,
            std::vector<Series>()
        });

        for(const auto &c : global_ids) {
            if(c.first.starts_with(best) && (program.code != -1 || (c.first[c.first.find(" ") + 1] - '0') < 4) && (c.first[c.first.find(" ") + 1] - '0') < 5 && course_reqs[c.second].first.size()) {
                program.requirements.back().series.push_back({{c.second}});
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
                    
                    if(term_course_map[0].find(course) != term_course_map[0].end()) {
                        std::cout << "\t\t\t\tTerm: 0" << std::endl;
                    }
                    if(term_course_map[1].find(course) != term_course_map[2].end()) {
                        std::cout << "\t\t\t\tTerm: 1" << std::endl;
                    }
                    if(term_course_map[2].find(course) != term_course_map[3].end()) {
                        std::cout << "\t\t\t\tTerm: 2" << std::endl;
                    }
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

    auto result = collection.find_one(make_document(kvp("program_name", code)));

    if(!result) {
        return {};
    }

    return load_program_from_string(bsoncxx::to_json(result.value()));
}

std::optional<std::pair<std::vector<Program>, std::unordered_map<int, int>>>
load_user_plan_from_mongo(
    std::string user_id,
    std::string plan_id, 
    mongocxx::client &conn)
{
    auto db = conn["test"];
    auto users_collection = db["users"];

    // First get the user to retrieve extension_id
    auto user_result = users_collection.find_one(make_document(kvp("_id", bsoncxx::oid(user_id))));

    if(!user_result) {
        std::cout << "User not found" << std::endl;
        return {};
    }

    // Get the extension_id from the user document
    auto extension_id_value = user_result->view()["extension_id"].get_oid().value;
    std::string extension_id = extension_id_value.to_string();
    
    std::cout << "Found extension_id: " << extension_id << std::endl;
    
    // Now query the extensions collection with the extension_id
    auto extensions_collection = db["extensions"];
    auto extension_result = extensions_collection.find_one(make_document(kvp("_id", extension_id_value)));

    if(!extension_result) {
        std::cout << "Extension not found" << std::endl;
        return {};
    }
    
    json data(json::parse(bsoncxx::to_json(extension_result.value())));
    std::vector<Program> programs;

    // Now access fields from the extension document
    for(auto p_list = data["degrees"].get<std::vector<json>>(); const auto &pl_obj : p_list) {
        std::string p_name = pl_obj["program_name"].get<std::string>();

        std::cout << "Major: " << p_name << std::endl;

        auto p_obj = load_program_from_mongo(p_name, conn);

        if(p_obj) {
            programs.push_back(*p_obj);
        }
    }

    std::unordered_map<int, int> taken_courses;

    for(auto c_list = data["courses_taken"].get<std::vector<json>>(); const auto &c_obj : c_list) {
        std::string c_code = c_obj["course_number"].get<std::string>();

        std::cout << "Taken Course: " << c_code << std::endl;

        if(global_ids[c_code] != -1) {
            taken_courses[global_ids[c_code]] = TAKEN;
        }
    }

    bool found = false;
    for(auto c_list = extension_result.value()["plans"].get_array().value; const auto &c_obj : c_list) {
        std::string c_code = c_obj["_id"].get_oid().value.to_string();

        std::cout << "OID: " << c_code << std::endl;

        if(c_code == bsoncxx::oid(plan_id).to_string()) {
            found = true;

            for(auto terms = c_obj["terms"].get_array().value; const auto &term : terms) {
                std::string t_name{term["name"].get_string().value};
                int y_off = std::stoi(t_name.substr(t_name.find_first_of(" "))) - base_year;
                int t_off = std::find(term_rotation.begin(), term_rotation.end(), t_name.substr(0, t_name.find_first_of(" ") + 1)) - term_rotation.begin();

                t_off += term_offset;
                t_off %= term_rotation.size();
                t_off -= term_offset;

                if(y_off * 3 + t_off >= term_count) {
                    std::cout << "Issue" << std::endl;
                    return {};
                }

                for(auto courses = term["courses"].get_array().value; const auto &course : courses) {
                    std::string c_code{course["course_number"].get_string().value};

                    taken_courses[global_ids[c_code]] = y_off * 3 + t_off + (y_off * 3 + t_off >= 0);

                    std::cout << "Taken Course: " << c_code << " @ " << y_off * 3 + t_off + (y_off * 3 + t_off >= 0) << std::endl;
                }
            }

            break;
        }
    }

    if(!found) {
        return {};
    }

    return make_pair(programs, taken_courses);
}

bool
push_plan_to_mongo(
    std::string user_id,
    std::string plan_id,
    std::array<std::vector<std::string>, term_count> plan,
    std::unordered_map<int, int> taken_courses,
    mongocxx::client &conn
) {
    auto db = conn["adviseu_db"];
    auto collection = db["courses"];

    auto term_list = bsoncxx::builder::basic::array{};
    
    std::map<int, std::vector<std::string>> taken_map;
    for(const auto &kv : taken_courses) {
        if(kv.second > 0 || kv.second == TAKEN)
            continue;

        taken_map[kv.second].push_back(global_names[kv.first]);
    }

    for(const auto &kv : taken_map) {
        int term_idx = kv.first;
        std::vector<std::string> course_list = kv.second;
        auto course_list_obj = bsoncxx::builder::basic::document{};
        auto course_list_array = bsoncxx::builder::basic::array{};
        
        course_list_obj.append(kvp("_id", bsoncxx::oid{}));
        course_list_obj.append(kvp("name", term_rotation[(term_rotation.size() - (term_idx % term_rotation.size())) % term_rotation.size()] + std::to_string(base_year + (term_idx - term_offset) / term_rotation.size())));
        term_idx += 1;

        for(const auto &course : course_list) {
            auto result = collection.find_one(make_document(kvp("course_number", course)));

            // std::cout << "Pasting: " << course << std::endl;

            if(!result) {
                return false;
            }

            course_list_array.append(*result);
        }

        course_list_obj.append(kvp("courses", course_list_array));
        term_list.append(course_list_obj);
    }

    int term_idx = 0;
    for(const auto &course_list : plan) {
        if(!course_list.size()) {
            continue;
        }

        auto course_list_obj = bsoncxx::builder::basic::document{};
        auto course_list_array = bsoncxx::builder::basic::array{};
        
        course_list_obj.append(kvp("_id", bsoncxx::oid{}));
        course_list_obj.append(kvp("name", term_rotation[term_idx % term_rotation.size()] + std::to_string(base_year + (term_offset + term_idx) / term_rotation.size())));
        term_idx += 1;

        for(const auto &course : course_list) {
            auto result = collection.find_one(make_document(kvp("course_number", course)));

            // std::cout << "Pasting: " << course << std::endl;

            if(!result) {
                return false;
            }

            course_list_array.append(*result);
        }

        course_list_obj.append(kvp("courses", course_list_array));
        term_list.append(course_list_obj);
    }

    // std::cout << bsoncxx::to_json(term_obj) << std::endl;

    // First get the user document to retrieve the extension_id
    db = conn["test"];
    auto users_collection = db["users"];
    auto user_result = users_collection.find_one(make_document(kvp("_id", bsoncxx::oid(user_id))));

    if(!user_result) {
        std::cout << "User not found" << std::endl;
        return false;
    }

    // Get the extension_id from the user document
    auto extension_id_value = user_result->view()["extension_id"].get_oid().value;

    // Now query the extensions collection with the extension_id
    auto extensions_collection = db["extensions"];
    auto extension_result = extensions_collection.find_one(make_document(kvp("_id", extension_id_value)));

    if(!extension_result) {
        std::cout << "Extension not found" << std::endl;
        return false;
    }

    int idx = -1, tidx = 0;
    for(auto c_list = extension_result.value()["plans"].get_array().value; const auto &c_obj : c_list) {
        std::string c_code = c_obj["_id"].get_oid().value.to_string();

        if(c_code == bsoncxx::oid(plan_id).to_string()) {
            idx = tidx;
            break;
        }

        tidx++;
    }

    if(idx == -1) {
        return false;
    }

    // Now update the plan in the extensions collection
    auto result = extensions_collection.update_one(
        make_document(kvp("_id", extension_id_value)), 
        make_document(kvp("$set", make_document(kvp("plans." + std::to_string(idx) + ".terms", term_list))))
    );

    return result->modified_count() == 1;
}

#endif