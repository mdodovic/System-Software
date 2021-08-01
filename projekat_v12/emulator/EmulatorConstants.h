#ifndef EMULATOR_CONSTANTS_H
#define EMULATOR_CONSTANTS_H

enum Register
{
    r0,
    r1,
    r2,
    r3,
    r4,
    r5,
    r6,
    sp = r6,
    r7,
    pc = r7,
    psw,
    unimportant_reg = 0xF
};

#endif //EMULATOR_CONSTANTS_H