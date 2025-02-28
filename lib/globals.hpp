#ifndef ADVISEU_GLOBALS_H
#define ADVISEU_GLOBALS_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace adviseu {
    constexpr int term_count = 18;
    inline std::unordered_map<std::string, int> global_ids;  // Mapping from courses to global IDs
    inline std::unordered_map<int, std::string> global_names;
    inline std::vector<std::pair<std::vector<std::vector<int>>, std::vector<std::vector<int>>>> course_reqs; // Course requirements: hard & soft constraints
    inline std::array<std::unordered_set<int>, term_count> term_course_map;  // Mapping of terms to set of courses
    inline std::unordered_map<std::string, int> term_map;  // Mapping of term names (e.g., "Fall") to numeric values
};

#endif