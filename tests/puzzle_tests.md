# Checks

## Checks for the code

1. `BEEP_HISTORY_ON_FAILURE` must not be defined

## Tests to carry out by hand on the final puzzle

1. Boot sound should play right after power on.
2. Whenever moving to an even position, a small "locked in" sound should sound. Odd positions should not give feedback.
3. When moving one of these position, check the morse messages:

```
0 (WHI) ->  .-- | .... |  ..
2 (TEN) ->   -  |   .  |  -.
4 (SMI) ->  ... |  --  |  ..
6 (CAP) -> -.-. |  .-  | .--.
8 (ECC) ->   .  | -.-. | -.-.
```

* solution combination: `8 2 4 6 0 (ECC, TEN, SMI, CAP, WHI)`
* hint combination:     `6 2 0 4 2 (CAP, TEN, WHI, SMI, TEN)`

4. Tests for solution: For each of the following cases check that
   1. enter solution, give each morse code time to finish
   2. enter solution, abort some morse codes in between
   3. enter solution, move to invalid/odd position in between (2s)

   The correct songs appear on positions 10 12 14. Also check whether

   1. when we directly move to another song position after we heard one song, this song should also play without having reentered the solution
   2.  do the songs abort on wheel change?

5. Tests for hint:
   1. does hint play after entering hint combination?
   2. does hint abort on wheel change?

6. Other tests:
   3.  go from position 0 to 1 and back to 0. Does the message replay?
   4. stay on one position for 10s. Does the message only play once?
