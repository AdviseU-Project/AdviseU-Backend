#ifndef SOLVER_HPP
#define SOLVER_HPP

#include <array>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "globals.hpp"

namespace adviseu {
    typedef int Course;
    typedef std::vector<Course> Option;
    typedef std::vector<Option> Series;

    // Struct to hold information about course requirements
    struct Requirement {
        int req_series;  // Number of required series of courses

        std::vector<Series> series;  // Each series is a list of options (groups of courses)
    };

    // Program struct holds program-level information, including requirements
    struct Program {
        std::string name;          // Program name (e.g., "Computer Science")
        int code;                  // Program code (unique identifier)

        std::vector<Requirement> requirements; // List of requirements (series and options)
    };

    std::optional<std::array<std::vector<std::string>, term_count>> plan_schedule(std::vector<Program>&, std::unordered_map<int, int>&);

    inline Program core;
}

#endif