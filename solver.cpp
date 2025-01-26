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

struct Requirement {
    int req_series;

    std::vector<Series> series;
};

struct Program {
    std::string name;
    int code;
    int credits;

    std::vector<Requirement> requirements;
};

std::unordered_map<Course, int> global_ids;
std::vector<std::pair<std::vector<std::vector<int>>, std::vector<std::vector<int>>>> course_reqs;
std::unordered_map<int, std::unordered_set<int>> term_course_map; 
std::unordered_map<std::string, int> term_map;

class CourseScheduler : public Space {
protected:
    std::shared_ptr<Program> program;
    IntVarArray course_term_map;

public:
    CourseScheduler(std::shared_ptr<Program> &p) : program(p), course_term_map(*this, global_ids.size()) {
        BoolVarArray requirement_sats(*this, program->requirements.size());

        for(int course_id = 0; course_id < global_ids.size(); course_id++) {
            std::vector<int> valid_terms;

            for(const auto &term_course : term_course_map) {
                if(term_course.second.find(course_id) != term_course.second.end()) {
                    valid_terms.push_back(term_course.first);
                }
            }

            course_id++;
            course_term_map[course_id] = IntVar(*this, IntSet(const_cast<const std::vector<int> &>(valid_terms)));
        }

        for(const auto &requirement : program->requirements) {
            int idx = 0;
            BoolVarArray series_sats(*this, requirement.series.size());

            for(const auto &options : requirement.series) {
                BoolVarArray option_sats(*this, options.size());

                for(const auto &option : options) {
                    for(const auto &course : option) {
                        Reify r(option_sats[course], RM_IMP);

                        rel(*this, course_term_map[global_ids[course]], IRT_LE, term_map.size(), r);
                    }
                }               

                rel(*this, BOT_OR, option_sats, series_sats[idx++]);
            }

            linear(*this, series_sats, IRT_GQ, requirement.req_series);
        }

        for(int course_id = 0; course_id < global_ids.size(); course_id++) {
            for(const auto &hard_group : course_reqs[course_id].first) {
                BoolVarArray group_sat(*this, hard_group.size());
                int group_idx = 0;

                for(const int &hard_course_id : hard_group) {
                    BoolVar cap, lt;

                    rel(*this, course_term_map[course_id], IRT_LE, course_term_map[hard_course_id], lt);
                    rel(*this, course_term_map[course_id], IRT_EQ, term_course_map.size() - 1, cap);
                    rel(*this, BOT_OR, {cap, lt}, group_sat[group_idx++]);
                }

                rel(*this, BOT_OR, group_sat, 1);
            }

            for(const auto &soft_group : course_reqs[course_id].second) {
                BoolVarArray group_sat(*this, soft_group.size());
                int group_idx = 0;

                for(const int &soft_course_id : soft_group) {
                    rel(*this, course_term_map[course_id], IRT_LQ, course_term_map[soft_course_id], group_sat[group_idx++]);
                }

                rel(*this, BOT_OR, group_sat, 1);
            }
        }

        IntSetArgs isa(term_course_map.size());
        std::vector<int> total;

        for(int i = 0; i <= global_ids.size(); i++) {
            total.push_back(i);
        }

        for(int i = 0; i < term_course_map.size() - 1; i++) {
            isa[i] = IntSet({0, 1, 2, 3, 4, 5, 6});
        }

        isa[term_course_map.size() - 1] = IntSet(const_cast<const std::vector<int> &>(total));

        count(*this, course_term_map, isa);

        branch(*this, course_term_map, INT_VAR_DEGREE_MIN(), INT_VAL_MAX());
    }

    CourseScheduler(CourseScheduler& s) : Space(s) {
        program = s.program;
        course_term_map.update(*this, s.course_term_map);
    } 

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

    void print(void) const {
        std::cout << course_term_map << std::endl;
    }
};

extern "C" {
    void invoke(void) {
        try {
            std::unique_ptr<CourseScheduler> m = std::make_unique<CourseScheduler>();
            DFS<CourseScheduler> e(m.get());

            while(std::unique_ptr<CourseScheduler> s{e.next()}) {
                s->print();
            }
        } catch (Exception e) {
            std::cerr <<"Gecode exception: " << e.what() << std::endl;
        }
    }
}