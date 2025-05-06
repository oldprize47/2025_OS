Creative Service: Flexible Character Search Mode

Purpose:
- Character Inclusion: Finds and highlights any line containing any character from the search keyword.
- Multi-Character Search: Allows searching for multiple characters in a single run.
- Spelling & Order Tolerance: Matches even if the characters in the keyword are out of order or contain minor typos.

Usage:
./wspipe <command> <search_word> [mode_flag]

Note: Set the last argument to -f or -F to enable Flexible Character Search Mode.

Example:

1. Default mode (exact string match):
   ./wspipe 'ls -al' target

2. Flexible character mode (any char in "target" matches):
   ./wspipe 'ls -al' target -f

In the second command, any line containing t, a, r, g, e, or t will be highlighted.