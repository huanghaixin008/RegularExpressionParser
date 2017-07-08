# RegularExpressionParser
A regular expression parser implemented in C++, supporting ASCII character set.

# Structure
Consist of one regex parser, one state machine supporting e-NFA and one state machine supporting only DFA.
main.cpp consists of some test codes.

# Performance
To parse a regex R"([0-9]+|/\*([^\*]|\*+[^\*/])*\*+/|//.*\n)" and select digits and annotation from a 3.64MB file, which contains 3,818,268 bytes or characters or symbols:
e-NFA machine selects 23870 targets in 36s, throughput 106063 symbols per second;
DFA machine selects 23870 targets in 35s, throughput 109093 symbols per second.
