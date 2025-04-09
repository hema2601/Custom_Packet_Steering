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
(This caused performance regression, look into the reason)

0.1.8

Introduce RPS-like steering between all IAPS-available cores (change the RPS Backup Core Decision)


1.0.0

No feature change compare to 0.1.8, but it runs now on the newer kernel patch (wiht the addition of a small debug statement)

1.0.1

Added risk_reorder parameter and potentialReorder stat. Packets can now be forcably steered away from an overloaded core, even if it risks reordering

1.0.2

Added idle backlog and active idle activation. This messes with other schemes (IAPS+RFS/+RPS), which will need to be fixed.

1.0.3

Fixed some stability problems and updated some outdated defaults. The idle backlog is now fairly stable. IAPS+RPS was fixed, IAPS+RFS still does not work. (Consider simply removing?)
