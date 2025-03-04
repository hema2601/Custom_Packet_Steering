Version History (started on 2025-02-20, so not complete)

0.1.0

Return to stable state where performance is promising
Load balancing is implemented, overload detection and reaction is not
RFS is already separated, IAPS flow table exists


0.1.1

Changed busy core detection.
Now a new target is chosen if
(prev_invalid || (!in_NAPI && input_q_empty) )
Before it was
(prev_invalid || !in_NAPI || input_q_empty)

0.1.2

Reintroduced overload checking at busy core detection.
No overload handling yet

0.1.3

Introduce additional overload check that rules out that packets get steered from their previous core while older packets are still being processed

0.1.4

Loop over busy list until non-overloaded target is found, otherwise send to previous.
Changes constant time access to busy list to O(n). 

0.1.5

Reintroduced some values to track the efficiency of the overload handling mechanism

0.1.6

Reintroducing the busy histogram

0.1.7

Restored load balancing timer (not using cfs utilization)
