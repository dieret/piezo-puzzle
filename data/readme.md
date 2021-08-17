Some csv files that store songs and a julia script that generates
C code from them.
The songs are stored as a collection of tuples (frequency, duration).
The generated C code defines a `float <song_name>[][2]` variable
containing the data.

To install the necessary Julia packages, run
```bash
julia --project
```
and then run
```
instantiate
```
from the REPL.

To generate C code, either stay in the REPL
and run
```julia
include("make_c_song_code.jl")
```
or exit julia and run
```bash
julia --project make_c_song_code.jl
```

Tested with Julia 1.6.1
