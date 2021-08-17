# Simple script that generates C code from the song csv files
using Printf

max_freq = 600

output_file = "songs.c"
input_files = ["song_hint.csv", "song0.csv", "song1.csv", "song2.csv"]
variable_names = ["hint", "song0", "song1", "song2"]

shift = 2^(-9/12)

# utility functions
line_to_note(line) =  parse.([Float64, Int64], split(line, ","))
transpose(note, factor) = [note[1] * factor, note[2]]
discretize_to_halftones(factor) = 2.0^(ceil(12*log2(factor))/12)
necessary_shift(note, mfreq) = min(1.0, mfreq / note[1])

open(output_file, "w") do out
    for (ifile, var_name) in zip(input_files, variable_names)
        input_lines = open(readlines, ifile, "r")
        filter!(!isempty, input_lines)

        len = length(input_lines) + 1
        notes = line_to_note.(input_lines)

        # assure that maximum frequency is under max_freq
        shift = minimum(necessary_shift.(notes, max_freq))
        notes = transpose.(notes, discretize_to_halftones(shift))

        println(out, "float $var_name[$len][2] = {")
        for note in notes
            println(out, (@sprintf "    {%.3f, %3d}," note[1] note[2]))
        end
        println(out, "    {0.0, -1.0}\n};")
        println(out)
    end
end
