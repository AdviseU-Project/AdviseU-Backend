
# AdviseU-Backend Algorithm Documentation

This document provides an overview of the scheduling algorithm for future modification.

## Data Model

### Term

Terms are represented by integers, where 0 is the current term, 1 is the next, and so on. Negative terms just mean the course has already been taken.

### Taken Courses

This is a map of all courses that are currently locked in the schedule (locked, meaning they don't move/change).
You build a schedule in stages by adding to this map after each stage completes.

taken_courses[global_course_id] = term (ranges from 0 to 18)

### Program

A list of Program structs are passed to the initial scheduler. These are combined into a single program struct to represent the total requirements of the selected degree combination.
Programs contain a list of requirements that each need to be satisfied. Each requirement is composed of a list of options that can satisfy it. One of these options must be satisfied to 
complete the requirement. Each option is composed of a list of course groups that must be taken to satisfy it. Finally, each course group is a list of courses that can satisfy that group's
requirement (only one needs to be taken to satisfy the group). Courses are represented by their global id so the scheduler can reference and interact with them.

```
Program -> All of [
  Requirement -> Any of [
    Track Option -> All of [
      Course Group -> Any of [
        Course ID
      ]
    ]
  ]
]
```

### Term Map

If a given scheduler stage works correctly, it will output a term map. This is an array of 18 course id lists. 
Each list represents the courses scheduled for the term given by the array index. This can be used to modify the 
taken_courses map for the next stage of scheduling.

## Current Algorithm Workflow

The current algorithm workflow is as follows:

1. **Schedule the Major-Specific Courses**
This stage works by collecting all inputted programs and passing them into a scheduler that tries to pack courses to the earlier terms, but with a 
per-term limit that prevents them from completely blocking out baccore classes for earlier terms (relaxing this restriction for later terms when less
baccore is taken). This term mapping is added to the taken_courses map to lock the current course placements into the later schedule stages.

2. **Schedule Baccore Courses**
This stage applies a different algorithm with only the baccore courses, to complete the degree requirements. It aims for quick insertion/satisfaction
into earlier terms to finish the degree as early as possible. This output is the final output of the algorithm.

## Creating Scheduler Stages

Creating a new scheduler stage requires two steps:

1. Preparing the program and taken_courses objects to correctly represent and schedule the current stage
2. A new Gecode Space & DFS engine invocation to perform the sheduling. These spaces set the constraints by
which the new courses can be added to the schedule. As seen in the current source code, both Spaces set up boolean equations
to model the program requirements (identically, so this can be moved into some external function for reuse) and then use counting
constraints to constrain how valid courses from the program can be placed into the term map. There is also code to distnguish which 
courses are already locked into the shedule, and appropriately handle their relationship with other courses without allowing them to move.
This code can all be reused from the existing two algorithm spae objects, and it's highly reccomended to reference [this](https://www.gecode.org/doc-latest/MPG.pdf)
when developing/modifying Space stages.
