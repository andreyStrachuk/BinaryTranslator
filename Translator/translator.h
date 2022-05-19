#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define LABELSNUMBER 20

#include "../lib/Text/TextAnalyze.h"

struct BinCode {
    unsigned char *buff;
    unsigned char *currentp;

    int size;
    int capacity;
};

struct Label {
    int *bin;
    int *softBin;

    int size;
    int capacity;

    char numberOfPass;
};

int LabelsInit (Label *label, const int initCap);

int LabelsResize (Label *label);

int WriteJmp (BinCode *bin, BinCode *softBin, Label *label);

int BinCodeInit (BinCode *bin, int cap);

int SoftBinCodeInit (BinCode *softBin, const char *fileName);

int BinaryTranslate (BinCode *bin, BinCode *softBin, Label *label, int numberOfPass);

int BinCodeDestruct (BinCode *bin);

int WritePop (BinCode *bin, BinCode *softBin, Label *label);

int CheckSignature (BinCode *bin);

int CheckIfImm (const int type);

int CheckIfReg (const int type);

int CheckIfMem (const int type);

int ArrangePushPop (BinCode *bin, BinCode *softBin, Label *label);

int WritePush (BinCode *bin, BinCode *softBin, Label *label);

int WritePop (BinCode *bin, BinCode *softBin, Label *label);

int WriteRet (BinCode *bin, BinCode *softBin, Label *label);

int WriteHlt (BinCode *bin, BinCode *softBin, Label *label);

void PutInt (BinCode *bin, const int value);

int CheckBuffOverflow (BinCode *bin, BinCode *softBin, Label *label);

int WriteInc (BinCode *bin, BinCode *softBin, Label *label);

int WriteDec (BinCode *bin, BinCode *softBin, Label *label);

int WriteAdd (BinCode *bin, BinCode *softBin, Label *label);

int SaveRetAddr (BinCode *bin, BinCode *softBin);

int RestoreRetAddr (BinCode *bin, BinCode *softBin);

int WriteOut (BinCode *bin, BinCode *softBin, Label *label);

void Out (const int value);

int WriteAddrOutFunc (BinCode *bin);

int WriteIn (BinCode *bin, BinCode *softBin, Label *label);

int PutLong (BinCode *bin, int *addr);

int WriteAddrInFunc (BinCode *bin);

int In ();

int WriteSub (BinCode *bin, BinCode *softBin, Label *label);

int LabelPushBack (Label *label, const int binVal, const int softVal);

int CheckIfLblContainsAddr (Label *label, const int value);

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

enum NumberOfPass {
    FIRST,
    SECOND
};

#endif