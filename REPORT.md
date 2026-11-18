# Smart Parking Allocation and Reservation Platform: Design and Complexity Report
DAA CSEG3053, Project 46. Language: C17. Build: `make`. Tests: `make test`. Benchmarks: `./parking --bench`.

## 1. Problem model
| Entity | Representation |
|---|---|
| Parking space | `Space` record: id, road distance from gate, price per hour, occupancy |
| Road network | `Graph` adjacency matrix (node 0 = gate, node i = space i) |
| Reservation | `Reservation` record: plate, start hour, end hour (half-open), value, space |
| Lot | `Lot`: arrays of spaces and reservations, road graph, revenue |

**Objective:** match drivers to spaces, minimise search distance, avoid reservation conflicts.
**Constraints:** one vehicle per space; reservations on a space may not overlap (`[start,end)`, 0 <= start < end <= 24); a reserved space is protected from other drivers during its window; billing is per hour rounded up, minimum 1 hour.
**Edge cases handled:** duplicate plate, full lot, unknown plate on exit, invalid times, disconnected graph, empty pattern or text, single-symbol Huffman input.
**Invariants:** the free-space list is sorted by (distance, index) before allocation; the `activity_selection` output set is pairwise non-overlapping; `interval_partition` never puts overlapping bookings on one space.

## 2. Module map (syllabus -> file -> parking use)
| Syllabus area | Algorithms (file) | Parking mapping |
|---|---|---|
| Foundations | `g_ops`, `now_ms`, `talloc` (core.c) | operation count, time and memory in every benchmark |
| Divide & conquer | merge/quick sort, binary search (iter/rec), max subarray, fast power, celebrity, classical vs Strassen (algo_dc.c) | sort spaces by distance; find a distance; busiest hours of net arrivals; modular hashing; most-preferred zone; zone-transfer matrices |
| Greedy | activity selection, interval partitioning, fractional knapsack, Huffman, Dijkstra, Prim, Kruskal (algo_greedy.c) | max bookings on one space; min spaces for all bookings; EV charger power; event-log compression; driver routing; sensor cabling |
| Dynamic programming | 0/1 knapsack, matrix chain, LCS, optimal BST, Bellman-Ford, Floyd-Warshall, TSP Held-Karp (algo_dp.c) | choose bookings under space-hours; plate similarity; space lookup tree; patrol tour |
| Backtracking / B&B | N-Queens, subset sum, knapsack backtracking, FIFO/LIFO/least-cost B&B (algo_bt.c) | constraint validation; exact booking decision |
| Flow / matching / CRT | Ford-Fulkerson, bipartite matching via flow, CRT (algo_flow.c) | gate lane capacity; driver-to-space assignment; ticket-counter reconstruction |
| String matching | naive, Rabin-Karp, finite automaton, KMP (algo_str.c) | plate and event search in logs |
| Domain / persistence | domain.c (CSV save and load) | parking, reservation, billing |
| UI / experiments | main.c (menu, `--workbench`, `--bench`) | CLI |

## 3. Recurrences
| Module | Recurrence | Solution | Method |
|---|---|---|---|
| Merge sort | T(n) = 2T(n/2) + Θ(n) | Θ(n log n) | Master, case 2 |
| Quick sort, best/average | T(n) = 2T(n/2) + Θ(n) | Θ(n log n) | Master, case 2 |
| Quick sort, worst (sorted input) | T(n) = T(n-1) + Θ(n) | Θ(n²) | iteration; Master NOT applicable (not aT(n/b)) |
| Binary search | T(n) = T(n/2) + Θ(1) | Θ(log n) | Master, case 2 |
| Max subarray | T(n) = 2T(n/2) + Θ(n) | Θ(n log n) | recursion tree |
| Fast power | T(e) = T(e/2) + Θ(1) | Θ(log e) | substitution |
| Celebrity (D&C candidate) | T(n) = 2T(n/2) + Θ(1), then Θ(n) verify | Θ(n) | Master, case 1 |
| Classical matrix multiply | T(n) = 8T(n/2) + Θ(n²) | Θ(n³) | Master, case 1 |
| Strassen | T(n) = 7T(n/2) + Θ(n²) | Θ(n^log2 7) ≈ Θ(n^2.81) | Master, case 1 |
| Subset sum / N-Queens | T(n) = 2T(n-1) + Θ(1) (worst) | O(2^n) | iteration; Master NOT applicable (subtract-and-conquer) |

## 4. Asymptotic summary
| Algorithm | Best | Average | Worst | Space |
|---|---|---|---|---|
| Merge sort | Θ(n log n) | Θ(n log n) | Θ(n log n) | Θ(n) |
| Quick sort | Ω(n log n) | Θ(n log n) | O(n²) | O(log n) to O(n) stack |
| Dijkstra (matrix) | Θ(V²) | Θ(V²) | Θ(V²) | Θ(V) |
| Bellman-Ford | Ω(E) | O(VE) | O(VE) | Θ(V) |
| Floyd-Warshall | Θ(V³) | Θ(V³) | Θ(V³) | Θ(V²) |
| Prim (matrix) / Kruskal | Θ(V²) / Θ(E log E)* | same | same | Θ(V) / Θ(E) |
| 0/1 knapsack DP | Θ(nW) | Θ(nW) | Θ(nW) | Θ(nW) |
| Knapsack backtracking / B&B | pruning-dependent | pruning-dependent | O(2^n) | O(n) / pool |
| TSP Held-Karp | Θ(n² 2^n) | same | same | Θ(n 2^n) |
| Ford-Fulkerson (DFS) | O(E·f*) | O(E·f*) | O(E·f*) | Θ(V²) |
| Naive match | Θ(n) | Θ(n) | Θ(nm) | O(1) |
| Rabin-Karp | Θ(n+m) | Θ(n+m) | Θ(nm) | O(1) |
| Automaton / KMP | Θ(n+m·|Σ|) / Θ(n+m) | same | same | Θ(m|Σ|) / Θ(m) |
\* this implementation sorts Kruskal's edges with insertion sort, so its sort step is O(E²).

## 5. Complexity classes and reduction
* **P:** max non-overlapping bookings on one space (greedy), min spaces needed (partitioning), shortest route, MST cabling, driver-space matching and lane capacity (max flow).
* **NP-complete (decision versions):** "revenue >= K within C space-hours?" (0/1 knapsack, weakly NP-complete: DP is pseudo-polynomial), "do durations sum exactly to C?" (subset sum), "is there a patrol tour of length <= K?" (TSP).
* **NP-hard (optimisation versions):** maximum-revenue booking selection, shortest patrol tour.
* **Reduction: SUBSET-SUM <=p 0/1-KNAPSACK.** Given a_1..a_n and target T, build items with weight = value = a_i, capacity T, and ask for value >= T. A subset sums to T iff the knapsack reaches T. The mapping is O(n), so knapsack is at least as hard as subset sum. Subset sum is NP-complete, so knapsack decision is NP-hard, and it is in NP, so NP-complete.

## 6. Algorithm-selection guide
| Situation | Use | Why |
|---|---|---|
| Nearest space for a car | Dijkstra (or precomputed distances + merge sort) | non-negative weights, tiny graph |
| Sort a large booking list | merge sort | guaranteed Θ(n log n); quick sort worst case Θ(n²) on sorted input |
| Rebuild many distances (all pairs) | Floyd-Warshall for V <= ~100; else Dijkstra from each source | simple, V³ |
| Graph with refunds (negative arcs) | Bellman-Ford | handles negative edges and detects negative cycles |
| Pick bookings under capacity C | knapsack DP when W is small; least-cost B&B when W is huge | DP is Θ(nW); LC B&B expanded the fewest nodes in our runs |
| Scan big logs for a plate | KMP or automaton (worst-case linear); Rabin-Karp for many patterns | naive degrades on repetitive text |
| Large matrix products | classical for n <= ~128; Strassen only for very large n | Strassen does fewer multiplications but pays allocation and addition overhead |
| Assign drivers to spaces | bipartite matching via max flow | exact and polynomial |

## 7. Correctness and test evidence
`make test` runs 139 automated checks: oracle comparisons (sorts vs each other; Dijkstra vs Bellman-Ford vs Floyd-Warshall; Prim vs Kruskal; knapsack DP vs backtracking vs three B&B variants; four string matchers on random DNA text; Strassen vs classical), textbook known answers (matrix chain 4500, LCS 4, OBST 142, TSP 80, N-Queens 92, max flow 23, CRT 23), adversarial inputs (sorted input for quick sort, disconnected graph, negative cycle), invalid input (bad reservation times, duplicate plate, full lot), and persistence round trips.

## 8. Limitations and ethics
* Fixed-size arrays (32 spaces, 128 reservations, 64 graph nodes); a production system would use dynamic structures and a database.
* Time is whole hours; no payment integration; plates are personal data, so real deployments need access control and data-retention limits.
* Pricing and allocation rules must be explainable and non-discriminatory.
* Memory figures are bytes allocated by the tracked allocator, not process peak memory.
