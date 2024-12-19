## K2 Window Generator

This repository attempts to generate windows that can be fed into K2 automatically.

We define a window as a contiguous chunk of instructions that are neither jump sources nor destinations. A jump source is a jump instruction, whereas a jump destination is an instruction reachable from a jump source. Furthermore, each obtained window must be within a specified size. The default is [5, 10], but this can be configured in the source code as needed.

Lastly, each window is measured by a heuristic which is currently the number of memory operations. Windows with a higher heuristic number are preferred and returned.

### Arguments
1) An insns.txt file from the {bpf-elf-tools/text-extractor, ebpf-extractor} repos
2) (OPTIONAL) an upper bound for the number of windows to generate

### Outputs
1) a debug.txt file if DEBUG is defined in the source code -- this outputs the instruction bitmap, where 1 denotes a JMP_{SRC/DST} and 0 does not.
2) the K2-formatted arguments containing the calculated windows (e.g. --win_s_list 0,1 --win_e_list 2,3) **printed to standard output**.\

#### Background
Rather than inspecting an eBPF program as a whole (which K2 can certainly do), it is preferred for K2 to look at and try to optimize smaller portions of interest. Previously, these windows were obtained by hand, but this program makes it simple to generate automatically several windows that can be directly fed into K2. This way, there is no chance to generate problematic windows (for example, a window containing a JMP_SRC that references a JMP_DST outside the window).

#### Notes
- if DEBUG is defined in the source code, each window (with its corresponding heuristic cost) is also printed to standard output.
- if the program finds no windows, it early exits with a failed return code.
- more specifically, insns.txt is the ASCII representation of a program's instructions obtained when an object file is fed into one of {bpf-elf-tools/text-extractor, ebpf-extractor}. 
