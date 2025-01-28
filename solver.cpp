#include <iostream>
#include <memory>
#include <unordered_set>

#include <gecode/int.hh>

#include "solver.h"

using namespace Gecode;

typedef int Course;
typedef std::vector<Course> Option;
typedef std::vector<Option> Series;

// Idea for these struct: We have a process which syncs with the db.
// all request threads will request a mapping to these structs (fine 
// because they are read-only). We can read the degree/course constraints
// frm there as long as they are loaded into memory properly (lock on updates from
// the sync end, or do a pointer redirection for lock-free impl)

// Struct to hold information about course requirements
struct Requirement {
    int req_series;  // Number of required series of courses

    std::vector<Series> series;  // Each series is a list of options (groups of courses)
};

// Program struct holds program-level information, including requirements
struct Program {
    std::string name;          // Program name (e.g., "Computer Science")
    int code;                  // Program code (unique identifier)
    int credits;               // Credits for the program

    std::vector<Requirement> requirements; // List of requirements (series and options)
};

// Global variables to hold course and term mappings
std::unordered_map<Course, int> global_ids;  // Mapping from courses to global IDs
std::vector<std::pair<std::vector<std::vector<int>>, std::vector<std::vector<int>>>> course_reqs; // Course requirements: hard & soft constraints
std::unordered_map<int, std::unordered_set<int>> term_course_map;  // Mapping of terms to set of courses
std::unordered_map<std::string, int> term_map;  // Mapping of term names (e.g., "Fall") to numeric values

// The main class to handle course scheduling using Gecode's constraint solver
class CourseScheduler : public Space {
protected:
    std::shared_ptr<Program> program;         // Program associated with this scheduler
    IntVarArray course_term_map;             // Array of integer variables for course-term assignments (int corresponds to term)

public:
    // Constructor initializes the program and course-term variables
    CourseScheduler(std::shared_ptr<Program> &p) : program(p), course_term_map(*this, global_ids.size()) {
        // Array to hold satisfaction status for requirements
        BoolVarArray requirement_sats(*this, program->requirements.size());

        // Loop through each course and determine valid terms (constrain domains)
        for(int course_id = 0; course_id < global_ids.size(); course_id++) {
            std::vector<int> valid_terms;

            // Find terms in which the course can be scheduled
            for(const auto &term_course : term_course_map) {
                if(term_course.second.find(course_id) != term_course.second.end()) {
                    valid_terms.push_back(term_course.first);
                }
            }

            // Create an integer variable for the course with its valid terms as the domain
            course_id++;
            course_term_map[course_id] = IntVar(*this, IntSet(const_cast<const std::vector<int> &>(valid_terms)));
        }

        // Iterate through the program's requirements and add constraints
        for(const auto &requirement : program->requirements) {
            int idx = 0;
            BoolVarArray series_sats(*this, requirement.series.size()); // Satisfaction status for each series of options

            // Process each series of options in the requirement
            for(const auto &options : requirement.series) {
                // Satisfaction status for each option in the series
                BoolVarArray option_sats(*this, options.size());

                // For each option, create constraints for its courses
                for(const auto &option : options) {
                    for(const auto &course : option) {
                        // Reify creates a conditional relation between variables
                        Reify r(option_sats[course], RM_IMP);

                        // Constrain that the course must be scheduled within the valid terms
                        // for the option_sat to be true
                        rel(*this, course_term_map[global_ids[course]], IRT_LE, term_map.size(), r);
                    }
                }

                // Create OR constraint for series satisfaction (at least one option must be satisfied for the series)
                // sat to be satisfied (bi-implicated)
                rel(*this, BOT_OR, option_sats, series_sats[idx++]);
            }

            // Ensure the requirement has the specified number of satisfied series
            linear(*this, series_sats, IRT_GQ, requirement.req_series);
        }

        // Process hard and soft course group constraints
        for(int course_id = 0; course_id < global_ids.size(); course_id++) {
            // Hard constraints (e.g., must take certain courses before or after others)
            for(const auto &hard_group : course_reqs[course_id].first) {
                BoolVarArray group_sat(*this, hard_group.size());
                int group_idx = 0;

                // Loop over the courses in the group and create constraints
                for(const int &hard_course_id : hard_group) {
                    BoolVar cap, lt;

                    // For hard constraints either the course cannot be scheduled or its
                    // pre-requisites must have an earlier term
                    rel(*this, course_term_map[course_id], IRT_LE, course_term_map[hard_course_id], lt);
                    rel(*this, course_term_map[course_id], IRT_EQ, term_course_map.size() - 1, cap);
                    rel(*this, BOT_OR, {cap, lt}, group_sat[group_idx++]);
                }

                // Add the OR constraint that at least one of the courses in the group must be satisfied
                rel(*this, BOT_OR, group_sat, 1);
            }

            // Soft constraints (preferences, e.g., scheduling flexibility)
            for(const auto &soft_group : course_reqs[course_id].second) {
                BoolVarArray group_sat(*this, soft_group.size());
                int group_idx = 0;

                // Loop over the courses in the soft group and create "less than or equal to" constraints
                for(const int &soft_course_id : soft_group) {
                    rel(*this, course_term_map[course_id], IRT_LQ, course_term_map[soft_course_id], group_sat[group_idx++]);
                }
                
                // Add the OR constraint that at least one of the courses in the soft group must be scheduled
                rel(*this, BOT_OR, group_sat, 1);
            }
        }

        // Define the possible set of terms for courses
        IntSetArgs isa(term_course_map.size());
        std::vector<int> total;

        // Define all possible terms (0 through global_ids.size())
        for(int i = 0; i <= global_ids.size(); i++) {
            total.push_back(i);
        }

        // Set up valid term ranges for each term in the course-term mapping
        for(int i = 0; i < term_course_map.size() - 1; i++) {
            isa[i] = IntSet({0, 1, 2, 3, 4, 5, 6});
        }

        // Set the last term to include all possible terms
        isa[term_course_map.size() - 1] = IntSet(const_cast<const std::vector<int> &>(total));

        // Add a counting constraint on the course-term mapping
        count(*this, course_term_map, isa);

        // Branching strategy: Choose variable with minimum degree and maximum value
        branch(*this, course_term_map, INT_VAR_DEGREE_MIN(), INT_VAL_MAX());
    }

    // Copy constructor for cloning the CourseScheduler object
    CourseScheduler(CourseScheduler& s) : Space(s) {
        program = s.program;
        course_term_map.update(*this, s.course_term_map);
    } 

    // Create a deep copy of the current space (CourseScheduler)
    virtual Space* copy(void) {
        return new CourseScheduler(*this);
    }

    // TODO: After verifying, try using this
    // virtual void constrain(const Space &_b) {
    //     const CourseScheduler &b = static_cast<const CourseScheduler &>(_b);
    //     IntSetArgs isa(term_course_map.size());
    //     std::vector<int> total;
    //     int max_term = 0;

    //     for(IntVar &course_term : course_term_map) {
    //         max_term = std::max(max_term, std::min((int) term_course_map.size() - 2, course_term.max()));
    //     }

    //     for(int i = 0; i <= global_ids.size(); i++) {
    //         total.push_back(i);
    //     }

    //     for(int i = 0; i < term_course_map.size() - 1; i++) {
    //         if(i < max_term) {
    //             isa[i] = IntSet({0, 1, 2, 3, 4, 5, 6});
    //         } else {
    //             isa[i] = IntSet({0});
    //         }
    //     }

    //     isa[term_course_map.size() - 1] = IntSet(total);

    //     count(*this, course_term_map, isa);
    // }

    // Print the current solution (course-term assignments)
    void print(void) const {
        std::cout << course_term_map << std::endl;
    }
};

extern "C" {
    void invoke(void) {
        try {
            std::unique_ptr<CourseScheduler> m = std::make_unique<CourseScheduler>(); // Create a CourseScheduler object
            DFS<CourseScheduler> e(m.get());  // Depth-First Search solver to explore the solution space

            // Solve and print solutions as they are found
            while(std::unique_ptr<CourseScheduler> s{e.next()}) {
                s->print();
            }
        } catch (Exception e) {
            std::cerr <<"Gecode exception: " << e.what() << std::endl;
        }
    }
}