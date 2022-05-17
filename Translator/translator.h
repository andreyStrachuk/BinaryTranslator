#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <sys/mman.h>

#include "../lib/Text/TextAnalyze.h"

struct BinCode {
    unsigned char *buff;
    unsigned char *currentp;

    int size;
    int capacity;
};

int BinCodeInit (BinCode *bin, int cap);

int SoftBinCodeInit (BinCode *softBin, const char *fileName);

int BinaryTranslate (BinCode *bin, BinCode *softBin);

int BinCodeDestruct (BinCode *bin);

int WritePop (BinCode *bin, BinCode *softBin);

int CheckSignature (BinCode *bin);

int CheckIfImm (const int type);

int CheckIfReg (const int type);

int CheckIfMem (const int type);

int ArrangePushPop (BinCode *bin, BinCode *softBin);

int WritePush (BinCode *bin, BinCode *softBin);

int WritePop (BinCode *bin, BinCode *softBin);

int WriteRet (BinCode *bin, BinCode *softBin);

int WriteHlt (BinCode *bin, BinCode *softBin);

void PutInt (BinCode *bin, const int value);

int CheckBuffOverflow (BinCode *bin, BinCode *softBin);

int WriteInc (BinCode *bin, BinCode *softBin);

int WriteDec (BinCode *bin, BinCode *softBin);

int WriteAdd (BinCode *bin, BinCode *softBin);

int SaveRetAddr (BinCode *bin, BinCode *softBin);

int RestoreRetAddr (BinCode *bin, BinCode *softBin);

int WriteOut (BinCode *bin, BinCode *softBin);

void Out (const int value);

int WriteAddrOutFunc (BinCode *bin);

int WriteIn (BinCode *bin, BinCode *softBin);

int PutLong (BinCode *bin, int *addr);

int WriteAddrInFunc (BinCode *bin);

int In ();

enum TypeOfCommand {
    PUSH = 1,
    POP = 2,
    RET = 13,
    INC,
    DEC,
    SQRT,
    IN,
    OUT
};

enum TypeOfArg {
    IMM = 1,
    MEM,
    REG,
    MEMREG,
    MEMREGIMM
};

enum RegsNumbersx86 {
    RAX = 0,
    RCX = 1,
    RDX = 2,
    RBX = 3
};

#endif