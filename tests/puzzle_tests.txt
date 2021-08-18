# Checks

## Checks for the code

1. `BEEP_HISTORY_ON_FAILURE` must not be defined

## Tests to carry out by hand on the final puzzle

1. Test morse messages:

```
0 (WHI) ->  .-- | .... |  ..
2 (TEN) ->   -  |   .  |  -.
4 (SMI) ->  ... |  --  |  ..
6 (CAP) -> -.-. |  .-  | .--.
8 (ECC) ->   .  | -.-. | -.-.
```

* solution combination: `8 2 4 6 0 (ECC, TEN, SMI, CAP, WHI)`
* hint combination:     `6 2 0 4 2 (CAP, TEN, WHI, SMI, TEN)`

2. Tests for solution:
   1. enter solution, give each morse code time to finish
   2.  enter solution, abort some morse codes in between
   3.  enter solution, move to invalid position in between (2s)
   4. can we play another song without reentering solution?
   5.  do the songs abort on wheel change?

3. Tests for hint:
   1. does hint play after entering hint combination?
   2. does hint abort on wheel change?

4. Other tests:
   3.  go from position 0 to 1 and back to 0. Does the message replay?
   4. stay on one position for 10s. Does the message only play once?
