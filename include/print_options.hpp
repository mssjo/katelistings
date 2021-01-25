#ifndef PRINT_OPTIONS_H
#define PRINT_OPTIONS_H


#define PRINT_OPT(OPT) ((opts & OPT) == OPT)
#define INDENT(N) std::string(4*N, ' ')

enum print_options {
    QUIET        = 0b00000,
    NORMAL       = 0b00001,
    VERBOSE      = 0b00011,
    DEBUG        = 0b01111,
    VERBOSITY    = 0b00111,
    ECHO_INPUT   = 0b01000,
    USE_COMMANDS = 0b10000
};

#endif
