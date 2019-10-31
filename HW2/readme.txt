B06902102
???

1. Execution:

My program is opened by compiling using make and executing
using ./bidding_system n_hosts n_players

2. Description:

The program solves the homework problem.

3. Self-Examination:

I finished all assignments.

Bidding_system works fine. It only forks n_host times.
It uses select to assign a new competition to an available host.

Host works fine. It waits for its children to die before returning
the message to the bidding_system

Player works fine. It returns the correct values.

The three programs work well put together and output the right result.

Makefile compiles the three programs using make.