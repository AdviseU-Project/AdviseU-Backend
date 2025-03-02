#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_set>

#include <gecode/int.hh>

#include "globals.hpp"
#include "solver.hpp"

using namespace adviseu;
using namespace Gecode;

// Bac Core gets dropped from data -> We merge this in later
// Elective -> merged into total block of credits
// Restricted/[Major Code] elective -> for now just make it major restricted to 3xx/4xx
// "/" containing options -> just create two options and select one
// Option -> drop in data, we will merge this when sending over

// Idea for these struct: We have a process which syncs with the db.
// all request threads will request a mapping to these structs (fine 
// because they are read-only). We can read the degree/course constraints
// frm there as long as they are loaded into memory properly (lock on updates from
// the sync end, or do a pointer redirection for lock-free impl)

// The main class to handle course scheduling using Gecode's constraint solver
class CourseScheduler : public Space {
protected:
    IntVarArray course_term_map;             // Array of integer variables for course-term assignments (int corresponds to term)
    std::unordered_set<int> touched_courses, mapped_courses;

    void fill(int course) {
        if(touched_courses.find(course) != touched_courses.end()) {
            return;
        }

        touched_courses.insert(course);

        for(const auto &req_groups : course_reqs[course].first) {
            for(const auto &req_course : req_groups) {
                fill(req_course);
            }
        }

        for(const auto &req_groups : course_reqs[course].second) {
            for(const auto &req_course : req_groups) {
                fill(req_course);
            }
        }
    }

public:
    // Constructor initializes the program and course-term variables
    CourseScheduler(Program& program, std::unordered_set<int> &taken_courses) : course_term_map(*this, global_ids.size()) {
        // Iterate through the program's requirements and add constraints
        for(const auto &requirement : program.requirements) {
            int series_idx = 0;
            BoolVarArray series_sats(*this, requirement.series.size(), 0, 1); // Satisfaction status for each series of options
            
            // Process each series of options in the requirement
            for(const auto &options : requirement.series) {
                // Satisfaction status for each option in the series
                int option_idx = 0;
                BoolVarArray option_sats(*this, options.size(), 0, 1);

                // For each option, create constraints for its courses
                for(const auto &option : options) {
                    for(const auto &course : option) {
                        assert(course >= 0);

                        // Reify creates a conditional relation between variables
                        Reify r(option_sats[option_idx], RM_IMP);

                        // Constrain that the course must be scheduled within the valid terms
                        // for the option_sat to be true
                        if(mapped_courses.find(course) == mapped_courses.end()) {
                            std::vector<int> valid_terms;

                            // Find terms in which the course can be scheduled
                            for(int term_idx = 0; term_idx < term_course_map.size(); term_idx++) {
                                if(term_course_map[term_idx].find(course) != term_course_map[term_idx].end()) {
                                    valid_terms.push_back(term_idx);
                                }
                            }

                            if(taken_courses.find(course) == taken_courses.end()) {
                                // Create an integer variable for the course with its valid terms as the domain
                                course_term_map[course] = IntVar(*this, IntSet(const_cast<const std::vector<int> &>(valid_terms)));
                            } else {
                                course_term_map[course] = IntVar(*this, -1, -1);
                            }
                            
                            mapped_courses.insert(course);

                            fill(course);
                        }

                        rel(*this, course_term_map[course], IRT_NQ, 0, r);
                    }

                    option_idx++;
                }

                // Create OR constraint for series satisfaction (at least one option must be satisfied for the series)
                // sat to be satisfied (bi-implicated)
                rel(*this, BOT_OR, option_sats, series_sats[series_idx++]);
            }

            // Ensure the requirement has the specified number of satisfied series
            linear(*this, series_sats, IRT_GQ, requirement.req_series);
        }

        for(int course_id = 0; course_id < global_ids.size(); course_id++) {
            if(mapped_courses.find(course_id) != mapped_courses.end()) {
                continue;
            }

            std::vector<int> valid_terms;

            // Find terms in which the course can be scheduled
            for(int term_idx = 0; term_idx < term_course_map.size(); term_idx++) {
                if(term_course_map[term_idx].find(course_id) != term_course_map[term_idx].end()) {
                    valid_terms.push_back(term_idx);
                }
            }

            // Create an integer variable for the course with its valid terms as the domain
            if(touched_courses.find(course_id) != touched_courses.end()) {
                if(taken_courses.find(course_id) == taken_courses.end()) {
                    // Create an integer variable for the course with its valid terms as the domain
                    course_term_map[course_id] = IntVar(*this, IntSet(const_cast<const std::vector<int> &>(valid_terms)));  
                } else {
                    course_term_map[course_id] = IntVar(*this, -1, -1);
                } 
            } else {
                course_term_map[course_id] = IntVar(*this, 0, 0);   
            }
        }

        for(const int &course : touched_courses) {
            if(taken_courses.find(course) != taken_courses.end()) {
                continue;
            }

            // Hard constraints (e.g., must take certain courses before or after others)
            for(const auto &hard_group : course_reqs[course].first) {
                if(hard_group.size() == 0) {
                    continue;
                }

                BoolVarArray group_sat(*this, hard_group.size() + 1, 0, 1);
                int group_idx = 0;

                // Loop over the courses in the group and create constraints
                for(const int &hard_course_id : hard_group) {
                    BoolVar on(*this, 0, 1), lt(*this, 0, 1);

                    // For hard constraints either the course cannot be scheduled or its
                    // pre-requisites must have an earlier term
                    rel(*this, course_term_map[course], IRT_GR, course_term_map[hard_course_id], lt);
                    rel(*this, course_term_map[hard_course_id], IRT_GR, 0, on);
                    rel(*this, on, BOT_AND, lt, group_sat[group_idx++]);
                }

                rel(*this, course_term_map[course], IRT_EQ, 0, group_sat[group_idx]);

                // Add the OR constraint that at least one of the courses in the group must be satisfied
                rel(*this, BOT_OR, group_sat, 1);
            }

            // Soft constraints (preferences, e.g., scheduling flexibility)
            for(const auto &soft_group : course_reqs[course].second) {
                if(soft_group.size() == 0) {
                    continue;
                }

                BoolVarArray group_sat(*this, soft_group.size() + 1, 0, 1);
                int group_idx = 0;

                // Loop over the courses in the group and create constraints
                for(const int &soft_course_id : soft_group) {
                    BoolVar on(*this, 0, 1), lt(*this, 0, 1);

                    // For soft constraints either the course cannot be scheduled or its
                    // pre-requisites must have an earlier term
                    rel(*this, course_term_map[course], IRT_GQ, course_term_map[soft_course_id], lt);
                    rel(*this, course_term_map[soft_course_id], IRT_GR, 0, on);
                    rel(*this, on, BOT_AND, lt, group_sat[group_idx++]);
                }

                rel(*this, course_term_map[course], IRT_EQ, 0, group_sat[group_idx]);

                // Add the OR constraint that at least one of the courses in the group must be satisfied
                rel(*this, BOT_OR, group_sat, 1);
            }
        }

        // Define the possible set of terms for courses
        IntSetArgs isa(term_course_map.size() + 1);
        IntArgs isv(term_course_map.size() + 1);
        std::vector<int> total;

        // Define all possible terms (0 through global_ids.size())
        for(int i = 0; i <= global_ids.size(); i++) {
            total.push_back(i);
        }

        // Set up valid term ranges for each term in the course-term mapping
        for(int i = 1; i < term_course_map.size(); i++) {
            isa[i] = IntSet({0, 1, 2, 3, 4, 5});
            isv[i] = i;
        }

        // Set the last term to include all possible terms
        isa[0] = IntSet(const_cast<const std::vector<int> &>(total));
        isv[0] = 0;

        // Set our "taken courses term placeholder"
        isa[term_course_map.size()] = IntSet(const_cast<const std::vector<int> &>(total));
        isv[term_course_map.size()] = -1;

        // // Add a counting constraint on the course-term mapping
        count(*this, course_term_map, isa, isv);

        // Branching strategy: Choose variable with minimum degree and maximum value
        branch(*this, course_term_map, INT_VAR_CHB_MAX(), INT_VAL_MIN());
    }

    // Copy constructor for cloning the CourseScheduler object
    CourseScheduler(CourseScheduler& s) : Space(s) {
        course_term_map.update(*this, s.course_term_map);
    } 

    // Create a deep copy of the current space (CourseScheduler)
    virtual Space* copy(void) {
        return new CourseScheduler(*this);
    }

    std::array<std::vector<std::string>, term_count> materialize(void) const {
        std::array<std::vector<std::string>, term_count> mapping;

        for(int i = 1; i < term_course_map.size(); i++) {
            std::cout << "Term " << i << ": ";

            int idx = 0;
            for(const auto &course : course_term_map) {
                if(course.val() == i) {
                    std::cout << global_names[idx] << " ";

                    mapping[i - 1].push_back(global_names[idx]);
                }

                idx++;
            }

            std::cout << std::endl;
        }

        return mapping;
    }
};

std::optional<std::array<std::vector<std::string>, term_count>> adviseu::plan_schedule(Program &program, std::unordered_set<int> &taken_courses) {
    try {
        program.requirements.insert(program.requirements.end(), core.requirements.begin(), core.requirements.end());

        std::unique_ptr<CourseScheduler> m = std::make_unique<CourseScheduler>(program, taken_courses); // Create a CourseScheduler object
        DFS<CourseScheduler> e(m.get());  // Depth-First Search solver to explore the solution space

        // Solve and print solutions as they are found
        while(std::unique_ptr<CourseScheduler> s{e.next()}) {
            return s->materialize();
        }
    } catch (Exception e) {
        std::cerr <<"Gecode exception: " << e.what() << std::endl;
    }

    return {};
}