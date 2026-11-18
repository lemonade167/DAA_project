# Smart Parking Allocation and Reservation Platform (C17)
Build and run:
    make            # builds ./parking and ./tests
    make test       # 139 automated checks
    ./parking       # interactive menu
    ./parking --workbench   # runs every syllabus module on parking data
    ./parking --bench       # benchmark tables (time / operations / nodes)
Data is saved to parking_lot.csv in the current directory.
Files: parking.h (shared types and API), core.c, algo_*.c (algorithm library), domain.c (parking logic + persistence), main.c (CLI, workbench, benchmarks), tests.c, REPORT.md (analysis).
