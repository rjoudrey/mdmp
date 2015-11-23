# MDmp #

MDmp is an open-source process memory dumper for Windows. It is able to dump the executable images from the memory space of a process, as well as arbitrary memory areas (stacks, heaps, areas with eXecute bit set, explicit address & size etc.).

It is written in C and has bindings included for Python (bindings for Delphi are planned).

The purpose of this library/tool is to assist in reverse engineering (particularly malware).

# mdmp.exe #
A tool called mdmp.exe (built on libmdmp) is available in the downloads and described [here](mdmpexe.md).

# pymdmp.pyd #
The binding for Python (currently Python 2.7) documented [here](pymdmp.md).