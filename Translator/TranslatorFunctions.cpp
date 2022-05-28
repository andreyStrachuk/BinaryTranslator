#include "translator.h"

#define NEWSHIFT softBin->currentp - softBin->buff

const int gSizeOfJmpAddr = 4;

static int gShiftSizeOfJmp = 3;

#define PUTBYTE(arg)    *(bin->currentp++) = arg;   \
                        bin->size++;                \


const char gPushVal  = 0x68;

const char gBackupVal = 25;      // 0 < size < capacity - backupVal

#define LABELPUSHBACK(label, size, NEWSHIFT)            if (label->numberOfPass == FIRST) {             \
                                                            LabelPushBack (label, size, NEWSHIFT);      \
                                                        }


#define DEF_CMD(name, number, numOfParams, code)        if (typeOfCmd == number) {      \
                                                            code                        \
                                                        }                               \

int BinaryTranslate (BinCode *bin, BinCode *softBin, Label *label, int numberOfPass) {
    assert (bin);

    label->numberOfPass = numberOfPass;

    int res = CheckSignature (softBin);
    if (res != OK) return WRONGSIGNATURE;

    SaveRetAddr (bin, softBin);

    while (true) {
        res = CheckBuffOverflow (bin, softBin, label);
        if (res == NULLELEMENT) break;

        int typeOfCmd = 0x1F & *(softBin->currentp);

        #include "../opcodes_def.h"
    }

    ModifyBinTo2ndPass (bin, softBin);

    return OK;
}

#undef DEF_CMD

int CheckSignature (BinCode *softBin) {
    assert (softBin);

    if (softBin->buff[0] == 0x41 && softBin->buff[1] == 0x53) {
        softBin->currentp += 2;
        softBin->buff += 2;

        return OK;
    }

    return WRONGSIGNATURE;
}

int BinCodeInit (BinCode *bin, int cap) {
    assert (bin);

    bin->capacity = cap;
    bin->size = 0;

    bin->buff = (unsigned char *)aligned_alloc (4096, bin->capacity * sizeof (char));
    if (bin->buff == nullptr) return ALLOCFAILED;

    bin->currentp = bin->buff;

    return OK;
}

int BinCodeDestruct (BinCode *bin) {
    assert (bin);

    free (bin->buff);

    return OK;
}

int SoftBinCodeInit (BinCode *softBin, const char *fileName) {
    assert (softBin);

    FILE *sbin = fopen (fileName, "rb+");
    if (sbin == nullptr) return UNABLETOOPENFILE;

    int fileSize = GetFileSize (fileName);
    
    softBin->buff = (unsigned char *)calloc (fileSize + 1, sizeof (char));
    if (softBin->buff == nullptr) return ALLOCFAILED;

    fileSize = fread (softBin->buff, sizeof (char), fileSize, sbin);

    softBin->currentp = softBin->buff;
    softBin->capacity = softBin->size = fileSize;

    return OK;
}

int ArrangePushPop (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);

    int typeOfCmd = *softBin->currentp;

    if ((typeOfCmd & 0x1F) == PUSH) {
        WritePush (bin, softBin, label);
    }

    if ((typeOfCmd & 0x1F) == POP) {
        WritePop (bin, softBin, label);
    }

    return OK;
}

int WritePush (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);    

    int typeOfCmd = *softBin->currentp;
    softBin->currentp++;

    int res = CheckIfMem (typeOfCmd);
    if (res == MEM) {
        int shift = *(u_int16_t *)softBin->currentp;
        softBin->currentp += 2;

        PUTBYTE (0x50);         // push [val]
        PUTBYTE (0xB8);

        PutInt (bin, shift);

        PUTBYTE (0xFF);
        PUTBYTE (0x30);

        PUTBYTE (0x50);

        LABELPUSHBACK (label, bin->size, NEWSHIFT);

        return OK;
    }
    else if (res == MEMREG) {
        char regNum = *softBin->currentp;
        softBin->currentp++;

        PUTBYTE (0xFF);                // push [reg_name]
        PUTBYTE (0x30 + regNum);

        LabelPushBack (label, bin->size, NEWSHIFT);

        return OK;
    }
    else if (res == MEMREGIMM) {
        char regNum = *softBin->currentp;
        softBin->currentp++;

        int shift = *(u_int16_t *)softBin->currentp;
        softBin->currentp += 2;

        PUTBYTE (0xFF);
        PUTBYTE (0xB0 + regNum);

        PutInt (bin, shift);

        LABELPUSHBACK (label, bin->size, NEWSHIFT);

        return OK;
    }

    res = CheckIfReg (typeOfCmd);
    if (res == REG) {
        char regNum = *softBin->currentp;
        softBin->currentp++;

        PUTBYTE (0x50 + regNum);

        LABELPUSHBACK (label, bin->size, NEWSHIFT);

        return OK;
    }

    res = CheckIfImm (typeOfCmd);
    if (res == IMM) {
        double value = *(double *)softBin->currentp;
        softBin->currentp += 8;

        PUTBYTE (gPushVal);

        PutInt (bin, (int)value);

        LABELPUSHBACK (label, bin->size, NEWSHIFT);

        return OK;
    }

    return WRONGINSTRUCTION;
}

int WritePop (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);

    int typeOfCmd = *softBin->currentp;
    softBin->currentp++;

    int res = CheckIfReg (typeOfCmd);
    if (res == REG) {
        char regNum = *(softBin->currentp++);

        PUTBYTE (0x58 + regNum);

        LABELPUSHBACK (label, bin->size, NEWSHIFT);
    }

    return OK;
}

int WriteInc (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);

    softBin->currentp++;

    // 3 00000000 4989C5                  	mov r13, rax
    // 4 00000003 58                      	pop rax
    // 5 00000004 48FFC0                  	inc rax
    // 6 00000007 50                      	push rax
    // 7 00000008 4C89E8                  	mov rax, r13

    PUTBYTE (0x49); PUTBYTE (0x89); PUTBYTE (0xC5);

    PUTBYTE (0x58);

    PUTBYTE (0x48); PUTBYTE (0xFF); PUTBYTE (0xC0);

    PUTBYTE (0x50);

    PUTBYTE (0x4C); PUTBYTE (0x89); PUTBYTE (0xE8);

    LABELPUSHBACK (label, bin->size, NEWSHIFT);

    return OK;
}



int WriteDec (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);

    softBin->currentp++;

    //  3 00000000 5F                              pop rdi
    //  4 00000001 48FFCF                          dec rdi
    //  5 00000004 57                       	   push rdi

    PUTBYTE (0x5F);

    PUTBYTE (0x48); PUTBYTE (0xFF); PUTBYTE (0xCF);

    PUTBYTE (0x57);

    LABELPUSHBACK (label, bin->size, NEWSHIFT);

    return OK;
}

int WriteAdd (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);

    softBin->currentp++;

    // 3 00000000 5E                              pop rsi
    // 4 00000001 5F                              pop rdi
    // 5                                  
    // 6 00000002 4801FE                          add rsi, rdi
    // 7 00000005 56                              push rsi

    PUTBYTE (0x5E); PUTBYTE (0x5F);

    PUTBYTE (0x48); PUTBYTE (0x01); PUTBYTE (0xFE);

    PUTBYTE (0x56);

    LABELPUSHBACK (label, bin->size, NEWSHIFT);

    return OK;
}

int WriteSub (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);

    softBin->currentp++;

    //  3 00000000 4989C5                  	mov r13, rax
    //  4 00000003 4989DC                  	mov r12, rbx
    //  5 00000006 58                      	pop rax
    //  6 00000007 5B                      	pop rbx
    //  7 00000008 4829D8                  	sub rax, rbx
    //  8 0000000B 50                      	push rax
    //  9 0000000C 4C89E8                  	mov rax, r13
    // 10 0000000F 4C89E3                  	mov rbx, r12

    PUTBYTE (0x49); PUTBYTE (0x89); PUTBYTE (0xC5);

    PUTBYTE (0x49); PUTBYTE (0x89); PUTBYTE (0xDC);

    PUTBYTE (0x58);

    PUTBYTE (0x5B);

    PUTBYTE (0x48); PUTBYTE (0x29); PUTBYTE (0xD8);

    PUTBYTE (0x50);

    PUTBYTE (0x4C); PUTBYTE (0x89); PUTBYTE (0xE8);

    PUTBYTE (0x4C); PUTBYTE (0x89); PUTBYTE (0xE3);

    LABELPUSHBACK (label, bin->size, NEWSHIFT);

    return OK;
}

int SaveRetAddr (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);

    // 3 00000000 415F                    	pop r15

    PUTBYTE (0x41);
    PUTBYTE (0x5F);

    return OK;
}

int RestoreRetAddr (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);

    // 3 00000000 4157                    	push r15

    PUTBYTE (0x41);
    PUTBYTE (0x57);

    return OK;
}

int WriteOut (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);

    softBin->currentp++;

    PUTBYTE (0x5F);        // pop rdi

    PUTBYTE (0x52);
    PUTBYTE (0x50);

    // 3 00000000 41BAval0000            	mov r10, val

    PUTBYTE (0x41);
    PUTBYTE (0xBA);

    WriteFuncAddr (bin, (void *)&Out);

    // 3 00000000 41FFD2                  	call r10

    PUTBYTE (0x41);
    PUTBYTE (0xFF);
    PUTBYTE (0xD2);

    PUTBYTE (0x58);
    PUTBYTE (0x5A);

    LABELPUSHBACK (label, bin->size, NEWSHIFT);

    return OK;
}

int WriteFuncAddr (BinCode *bin, void *pointer) {
    assert (bin);
    assert (pointer);

    // addr of func is 3byte val

    unsigned long addr = (unsigned long)pointer;

    for (int i = 0; i < 3; i++) {
        PUTBYTE (*((unsigned char *)&addr + i));
    }

    PUTBYTE (0x00);    

    return OK;
}

int WriteRet (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);

    softBin->currentp++;

    PUTBYTE (0xC3);
    bin->size += 5;

    LABELPUSHBACK (label, bin->size, NEWSHIFT);

    return OK;
}

int WriteHlt (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);

    softBin->currentp++;

    RestoreRetAddr (bin, softBin);

    PUTBYTE (0xC3);

    LABELPUSHBACK (label, bin->size, NEWSHIFT);

    return OK;
}

int WriteIn (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);

    softBin->currentp++;

    // 2 00000000 4989C7                  	mov r14, rax
    PUTBYTE (0x49); PUTBYTE (0x89); PUTBYTE (0xC6);

    // 3 00000000 41BAval0000            	mov r10, val
    PUTBYTE (0x41); PUTBYTE (0xBA);
    WriteFuncAddr (bin, (void *)&In);

    // 3 00000000 4831C0                  	xor rax, rax
    PUTBYTE (0x48); PUTBYTE (0x31); PUTBYTE (0xC0);

    PUTBYTE (0x51); PUTBYTE (0x52); PUTBYTE (0x53);

    // 3 00000000 55                      	push rbp
    // 4 00000001 4889E5                  	mov rbp, rsp
    PUTBYTE (0x55);
    PUTBYTE (0x48); PUTBYTE (0x89); PUTBYTE (0xE5);

    // 3 00000000 4883E4F0                	and rsp,0xFFFFFFFFFFFFFFF0
    PUTBYTE (0x48); PUTBYTE (0x83); PUTBYTE (0xE4); PUTBYTE (0xF0);

    // 3 00000000 41FFD2                  	call r10
    PUTBYTE (0x41); PUTBYTE (0xFF); PUTBYTE (0xD2);

    // 6 00000004 4889EC                  	mov rsp, rbp
    //  7 00000007 5D                      	pop rbp
    PUTBYTE (0x48); PUTBYTE (0x89); PUTBYTE (0xEC);
    PUTBYTE (0x5D);

    PUTBYTE (0x5B); PUTBYTE (0x5A); PUTBYTE (0x59);

    //  2 00000000 50                      	push rax
    PUTBYTE (0x50);

    // 4 00000003 4C89F8                  	mov rax, r14
    PUTBYTE (0x4C); PUTBYTE (0x89); PUTBYTE (0xF0);

    LABELPUSHBACK (label, bin->size, NEWSHIFT);

    return OK;
}

int WriteJmp (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);
    assert (label);

    softBin->currentp++;

    // 3 00000000 5F                      	pop rdi
    // 4 00000001 5E                      	pop rsi

    PUTBYTE (0x5F); PUTBYTE (0x5E);

    // 3 00000000 4839FE                  	cmp rsi, rdi
    
    PUTBYTE (0x48); PUTBYTE (0x39); PUTBYTE (0xFE);
    
    // 4 00000003 74**                    	je label
    PUTBYTE (0x0F);
    PUTBYTE (0x84);

    int absShift = *(u_int16_t *)(softBin->currentp);
    softBin->currentp += 2;

    int current = bin->currentp - bin->buff;

    int res = CheckIfLblContainsAddr (label, absShift);

    int relShiftBin = label->bin[res] - current;

    if (res != -1) {
        if (relShiftBin < 0) {
            PutInt (bin, label->bin[res] - current - gShiftSizeOfJmp);
        }
        else {
            PutInt (bin, label->bin[res] - current - gSizeOfJmpAddr);
        }
    }
    else {
        // 3 00000000 90                      	nop
        PUTBYTE (0x90); PUTBYTE (0x90); PUTBYTE (0x90); PUTBYTE (0x90);
    }

    LABELPUSHBACK (label, bin->size, NEWSHIFT);

    return OK;
}

int WriteJl (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);
    assert (label);

    softBin->currentp++;

    // 3 00000000 5F                      	pop rdi
    // 4 00000001 5E                      	pop rsi

    PUTBYTE (0x5F); PUTBYTE (0x5E);

    // 3 00000000 4839FE                  	cmp rsi, rdi
    
    PUTBYTE (0x48); PUTBYTE (0x39); PUTBYTE (0xFE);
    
    // 4 00000003 74**                    	jb label
    PUTBYTE (0x0F);
    PUTBYTE (0x8C);

    int absShift = *(u_int16_t *)(softBin->currentp);
    softBin->currentp += 2;

    int current = bin->currentp - bin->buff;

    int res = CheckIfLblContainsAddr (label, absShift);
    int relShiftBin = label->bin[res] - current;

    if (res != -1) {
        if (relShiftBin < 0) {
            PutInt (bin, label->bin[res] - current - gShiftSizeOfJmp);
        }
        else {
            PutInt (bin, label->bin[res] - current - gSizeOfJmpAddr);
        }
    }
    else {
        // 3 00000000 90                      	nop
        PUTBYTE (0x90); PUTBYTE (0x90); PUTBYTE (0x90); PUTBYTE (0x90);
    }

    LABELPUSHBACK (label, bin->size, NEWSHIFT);

    return OK;
}

int WriteMul (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);
    assert (label);

    softBin->currentp++;

    //  3 00000000 5E                      pop rsi
    //  4 00000001 5F                      pop rdi
    PUTBYTE (0x5E); PUTBYTE (0x5F);

    PUTBYTE (0x52);

    //  6 00000002 4989C2                  mov r10, rax
    PUTBYTE (0x49); PUTBYTE (0x89); PUTBYTE (0xC2);

    //  7 00000005 4889F0                  mov rax, rsi
    PUTBYTE (0x48); PUTBYTE (0x89); PUTBYTE (0xF0);

    //  8 00000008 48F7E7                  mul rdi
    PUTBYTE (0x48); PUTBYTE (0xF7); PUTBYTE (0xEF);

    PUTBYTE (0x5A);

    // 10 0000000B 50                      push rax
    PUTBYTE (0x50);

    // 12 0000000C 4C89D0                  mov rax, r10
    PUTBYTE (0x4C); PUTBYTE (0x89); PUTBYTE (0xD0);

    return OK;
}

int WriteCall (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);
    assert (label);

    softBin->currentp++;

    // 18 00000011 E8addr4b              	call proc

    PUTBYTE (0xE8);

    int absShift = *(u_int16_t *)(softBin->currentp);
    softBin->currentp += 2;

    int current = bin->currentp - bin->buff;

    int res = CheckIfLblContainsAddr (label, absShift);

    int relShiftBin = label->bin[res] - current;

    if (res != -1) {
        if (relShiftBin < 0) {
            PutInt (bin, label->bin[res] - current);
        }
        else {
            PutInt (bin, label->bin[res] - current);
        }
    }
    else {
        // 3 00000000 90                      	nop
        PUTBYTE (0x90); PUTBYTE (0x90); PUTBYTE (0x90); PUTBYTE (0x90);
    }

    LABELPUSHBACK (label, bin->size, NEWSHIFT);

    return OK;
}

int WriteDiv (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);
    assert (label);

    softBin->currentp++;

    //  3 00000000 5E                      pop rsi
    //  4 00000001 5F                      pop rdi
    PUTBYTE (0x5E); PUTBYTE (0x5F);

    //  6 00000002 4989C2                  mov r10, rax
    PUTBYTE (0x49); PUTBYTE (0x89); PUTBYTE (0xC2);

    // 3 00000000 52                       push rdx
    PUTBYTE (0x52);

    //  7 00000005 4889F0                  mov rax, rsi
    PUTBYTE (0x48); PUTBYTE (0x89); PUTBYTE (0xF0);

    // 3 00000000 4831D2                   xor rdx, rdx
    PUTBYTE (0x48); PUTBYTE (0x31); PUTBYTE (0xD2);

    //  8 00000008 48F7F7                  div rdi
    PUTBYTE (0x48); PUTBYTE (0xF7); PUTBYTE (0xF7);

    // 5 00000001 5A                       pop rdx
    PUTBYTE (0x5A);

    // 10 0000000B 50                      push rax
    PUTBYTE (0x50);

    // 12 0000000C 4C89D0                  mov rax, r10
    PUTBYTE (0x4C); PUTBYTE (0x89); PUTBYTE (0xD0);
    
    LABELPUSHBACK (label, bin->size, NEWSHIFT);

    return OK;
}

int WriteSqrt (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);
    assert (label);

    softBin->currentp++;

    // 2 00000000 4989C4                          mov r12, rax
    // 3 00000000 5F                              pop rdi
    // 4 00000001 41BAval                         mov r10, val
    // 5 00000007 41FFD2                          call r10
    // 6 00000003 4C89E0                          mov rax, r12

    PUTBYTE (0x49); PUTBYTE (0x89); PUTBYTE (0xC4);

    PUTBYTE (0x5F);

    PUTBYTE (0x41); PUTBYTE (0xBA);
    WriteFuncAddr (bin, (void *)&Sqrt);

    PUTBYTE (0x41); PUTBYTE (0xFF); PUTBYTE (0xD2);

    // 3 00000000 50                              push rax
    PUTBYTE (0x50);

    PUTBYTE (0x4C); PUTBYTE (0x89); PUTBYTE (0xE0);

    LABELPUSHBACK (label, bin->size, NEWSHIFT);

    return OK;
}

int Sqrt (const int val) {
    return (int)sqrt (val);
}

int PutLong (BinCode *bin, int *addr) {
    assert (bin);
    assert (addr);

    int sizeOfLong = sizeof (long);

    unsigned long tmp = (unsigned long)addr;

    for (int i = 0; i < sizeOfLong; i++) {
        *(bin->currentp++) = *((unsigned char *)&tmp + i);
    }

    bin->size += 8;

    return OK;
}

int In () {
    int value = 0;
    scanf ("%d", &value);

    return value;
}

int CheckIfImm (const int type) {
    if (type & 0x20 && !(type & 0x40) && !(type & 0x80)) {
        return IMM;
    }
    
    return 0;
}

int CheckIfReg (const int type) {
    if (type & 0x40 && !(type & 0x80) && !(type & 0x20)) { 
        return REG;
    }

    return 0;
}

int CheckIfMem (const int type) {
    if (type & 0x80 && !(type & 0x40) && !(type & 0x20)) {
        return MEM;
    }
    else if (type & 0x80 && type & 0x40 && !(type & 0x20)) {
        return MEMREG;
    }
    else if (type & 0x80 && type & 0x40 && type & 0x20) {
        return MEMREGIMM;
    }

    return 0;
}

void PutInt (BinCode *bin, const int value) {
    assert (bin);

    int sizeOfInt = sizeof (int);

    for (int i = 0; i < sizeOfInt; i++) {
        *bin->currentp = *((unsigned char *)&value + i);

        bin->currentp++;
    }

    bin->size += sizeOfInt;
}

void Out (const int value) {

    printf ("%d\n", value);

}

int CheckBuffOverflow (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);

    int length = bin->currentp - bin->buff;

    if (length >= bin->capacity - gBackupVal) {
        void *ptr = bin->buff;
        ptr = realloc (ptr, bin->capacity * 2 * sizeof (char));
        if (ptr == nullptr) return ALLOCFAILED;

        bin->buff = (unsigned char *)ptr;
        bin->currentp = bin->buff + length;
        bin->capacity *= 2;
    }

    if (softBin->currentp - softBin->buff == softBin->size) {
        WriteHlt (bin, softBin, label);

        return NULLELEMENT;
    }

    return OK;
}

int LabelsInit (Label *label, const int initCap) {
    assert (label);

    label->capacity = initCap;
    label->size = 1;

    label->bin = (int *)calloc (initCap, sizeof (int));
    if (label->bin == nullptr) return ALLOCFAILED;

    label->softBin = (int *)calloc (initCap, sizeof (int));
    if (label->softBin == nullptr) return ALLOCFAILED;

    label->bin[0]       = 2;
    label->softBin[0]   = 0;

    return OK;
}

int LabelsResize (Label *label) {
    assert (label);

    label->capacity *= 2;

    void *ptr = (void *)label->bin;
    ptr = realloc (ptr, label->capacity);
    if (ptr == nullptr) return ALLOCFAILED;
    label->bin = (int *)ptr;

    ptr = (void *)label->softBin;
    ptr = realloc (ptr, label->capacity);
    if (ptr == nullptr) return ALLOCFAILED;
    label->softBin = (int *)ptr;

    return OK;
}

int LabelPushBack (Label *label, const int binVal, const int softVal) {
    assert (label);

    if (label->numberOfPass == FIRST) {
        if (label->size >= label->capacity) {
            LabelsResize (label);
        }

        label->softBin[label->size] = softVal;
        label->bin[label->size]     = binVal;

        label->size++;
    }

    return OK;
}

int CheckIfLblContainsAddr (Label *label, const int value) {
    assert (label);

    int dest = -1;

    for (int i = 0; i < label->size; i++) {
        if (label->softBin[i] == value) {
            dest = i;
            break;
        }
    }

    return dest;
}

int ModifyBinTo2ndPass (BinCode *bin, BinCode *softBin) {
    assert (softBin);

    softBin->buff -= 2;
    softBin->currentp = softBin->buff;

    bin->currentp = bin->buff;
    bin->size = 0;

    return OK;
}

int LabelsDestruct (Label *label) {
    assert (label);

    free (label->bin);

    free (label->softBin);

    return OK;
}