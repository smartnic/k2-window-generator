## K2 Window Generator

This repository attempts to automatically generate windows that can be fed into K2.

Each window is a contiguous chunk of instructions that are neither jump sources nor destinations. A jump source is a literal jump instruction, and a jump destination is an instruction reached from a jump source.

Furthermore, each obtained window must be within a specified size. The default is [5, 10], but this can be configured in the source code as needed.

Lastly, each window is measured by a heuristic which is currently the number of memory operations. Windows with a higher heuristic number are preferred and returned.

*ARGUMENTS:*
1) An insns.txt file from the {bpf-elf-tools/text-extractor, ebpf-extractor} repos
2) (OPTIONAL) an upper bound for the number of windows to generate

*OUTPUTS:*
1) a debug.txt file if DEBUG is defined in the source code
2) An output.txt file with K2-formatted arguments containing the calculated windows (e.g. --win_s_list 0,1 --win_e_list 2,3)
