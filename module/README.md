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

