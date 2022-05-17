#include "translator.h"

FILE *global = fopen ("testik.asm", "w");


const char pushByte = 0x6A;
const char pushVal  = 0x68;

const char backupVal = 25;      // 0 < size < capacity - backupVal


#define DEF_CMD(name, number, numOfParams, code)        if (typeOfCmd == number) {      \
                                                            code                        \
                                                        }                               \

int BinaryTranslate (BinCode *bin, BinCode *softBin) {
    assert (bin);

    int res = CheckSignature (softBin);
    if (res != OK) return WRONGSIGNATURE;

    SaveRetAddr (bin, softBin);

    while (true) {
        res = CheckBuffOverflow (bin, softBin);
        if (res == NULLELEMENT) break;

        int typeOfCmd = 0x1F & *(softBin->currentp);

        #include "../opcodes_def.h"
    }

    // fwrite (bin->buff, bin->size, sizeof (char), global);

    printf ("bin size = %d\n", bin->size);

    for (int i = 0; i < bin->size; i++) {
        printf ("%X ", bin->buff[i]);
    }
    printf ("\n");

    return OK;
}

#undef DEF_CMD

int CheckSignature (BinCode *softBin) {
    assert (softBin);

    if (softBin->buff[0] == 0x41 && softBin->buff[1] == 0x53) {
        softBin->currentp += 2;

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

int ArrangePushPop (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);

    int typeOfCmd = *softBin->currentp;

    if ((typeOfCmd & 0x1F) == PUSH) {
        WritePush (bin, softBin);
    }

    if ((typeOfCmd & 0x1F) == POP) {
        WritePop (bin, softBin);
    }

    return OK;
}

int WritePush (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);    

    int typeOfCmd = *softBin->currentp;
    softBin->currentp++;

    int res = CheckIfMem (typeOfCmd);
    if (res == MEM) {
        int shift = *(u_int16_t *)softBin->currentp;
        softBin->currentp += 2;

        fprintf (global, "push rax\nmov rax, %d\npush [rax]\npop rax", shift);

        *bin->currentp = 0x50;          // push [val]
        *(bin->currentp + 1) = 0xB8;
        bin->currentp += 2;

        PutInt (bin, shift);

        *(bin->currentp) = 0xFF;
        *(bin->currentp + 1) = 0x30;
        bin->currentp += 2;

        *(bin->currentp) = 0x50;
        bin->currentp++;

        bin->size += 7;

        return OK;
    }
    else if (res == MEMREG) {
        char regNum = *softBin->currentp;
        softBin->currentp++;

        fprintf (global, "push [%d - reg num]\n", regNum);

        *bin->currentp       = 0xFF;                // push [reg_name]
        *(bin->currentp + 1) = 0x30 + regNum;

        bin->size += 2;    

        return OK;
    }
    else if (res == MEMREGIMM) {
        char regNum = *softBin->currentp;
        softBin->currentp++;

        int shift = *(u_int16_t *)softBin->currentp;
        softBin->currentp += 2;

        fprintf (global, "push [%d -reg + %d]\n", regNum, shift);

        *bin->currentp = 0xFF;
        if (shift >= -128 && shift < 128) {
            *(bin->currentp + 1) = 0x70 + regNum;
        }
        else {
            *(bin->currentp + 1) = 0xB0 + regNum;
        }

        bin->currentp += 2;

        PutInt (bin, shift);

        bin->size += 6;

        return OK;
    }

    res = CheckIfReg (typeOfCmd);
    if (res == REG) {
        char regNum = *softBin->currentp;
        softBin->currentp += 2;

        fprintf (global, "push %d - reg\n", regNum);

        *bin->currentp = 0x50 + regNum;
        bin->currentp++;
        bin->size++;

        return OK;
    }

    res = CheckIfImm (typeOfCmd);
    if (res == IMM) {
        double value = *(double *)softBin->currentp;
        softBin->currentp += 8;

        fprintf (global, "push %d\n", (int)value);

        if (value >= -128 && value < 128) {
            *(bin->currentp++) = pushByte;
        }
        else {
            *(bin->currentp++) = pushVal;
        }

        PutInt (bin, (int)value);

        bin->size += 5;

        return OK;
    }

    return WRONGINSTRUCTION;
}

int WritePop (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);

    int typeOfCmd = *softBin->currentp;
    softBin->currentp++;

    int res = CheckIfReg (typeOfCmd);
    if (res == REG) {
        char regNum = *(softBin->currentp++);
        printf ("reg num - %X\n", regNum);

        *(bin->currentp++) = 0x58 + regNum;

        bin->size++;
    }

    return OK;
}

int WriteInc (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);

    softBin->currentp++;

    printf ("@@@@@\n");
    // 3 00000000 4989C5                  	mov r13, rax
    // 4 00000003 58                      	pop rax
    // 5 00000004 48FFC0                  	inc rax
    // 6 00000007 50                      	push rax
    // 7 00000008 4C89E8                  	mov rax, r13

    *(bin->currentp++) = 0x49;
    *(bin->currentp++) = 0x89;
    *(bin->currentp++) = 0xC5;

    *(bin->currentp++) = 0x58;

    *(bin->currentp++) = 0x48;
    *(bin->currentp++) = 0xFF;
    *(bin->currentp++) = 0xC0;

    *(bin->currentp++) = 0x50;

    *(bin->currentp++) = 0x4C;
    *(bin->currentp++) = 0x89;
    *(bin->currentp++) = 0xE8;

    bin->size += 11;

    return OK;
}

int WriteDec (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);

    //  3 00000000 4989C5                  	mov r13, rax
    //  4 00000003 58                      	pop rax
    //  5 00000004 48FFC8                  	dec rax
    //  6 00000007 50                      	push rax
    //  7 00000008 4C89E8                  	mov rax, r13

    *(bin->currentp++) = 0x49;
    *(bin->currentp++) = 0x89;
    *(bin->currentp++) = 0xC5;

    *(bin->currentp++) = 0x58;

    *(bin->currentp++) = 0x48;
    *(bin->currentp++) = 0xFF;
    *(bin->currentp++) = 0xC8;

    *(bin->currentp++) = 0x50;

    *(bin->currentp++) = 0x4C;
    *(bin->currentp++) = 0x89;
    *(bin->currentp++) = 0xE8;

    bin->size += 11;

    softBin->currentp++;

    printf ("DONE\n");

    return OK;
}

int WriteAdd (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);

    //  3 00000000 4989C5                  	mov r13, rax
    //  4 00000003 4989DC                  	mov r12, rbx
    //  5 00000006 58                      	pop rax
    //  6 00000007 5B                      	pop rbx
    //  7 00000008 4801D8                  	add rax, rbx
    //  8 0000000B 50                      	push rax
    //  9 0000000C 4C89E8                  	mov rax, r13
    // 10 0000000F 4C89E3                  	mov rbx, r12

    *(bin->currentp++) = 0x49;
    *(bin->currentp++) = 0x89;
    *(bin->currentp++) = 0xC5;

    *(bin->currentp++) = 0x49;
    *(bin->currentp++) = 0x89;
    *(bin->currentp++) = 0xDC;

    *(bin->currentp++) = 0x58;

    *(bin->currentp++) = 0x5B;

    *(bin->currentp++) = 0x48;
    *(bin->currentp++) = 0x01;
    *(bin->currentp++) = 0xD8;

    *(bin->currentp++) = 0x50;

    *(bin->currentp++) = 0x4C;
    *(bin->currentp++) = 0x89;
    *(bin->currentp++) = 0xE8;

    *(bin->currentp++) = 0x4C;
    *(bin->currentp++) = 0x89;
    *(bin->currentp++) = 0xE3;

    bin->size += 18;

    softBin->currentp++;

    return OK;
}

int SaveRetAddr (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);

    // 3 00000000 415F                    	pop r15

    *(bin->currentp++) = 0x41;
    *(bin->currentp++) = 0x5F;

    bin->size += 2;

    return OK;
}

int RestoreRetAddr (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);

    // 3 00000000 4157                    	push r15

    *(bin->currentp++) = 0x41;
    *(bin->currentp++) = 0x57;

    bin->size += 2;

    return OK;
}

int WriteOut (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);

    *(bin->currentp++) = 0x5F;      // pop rdi
    bin->size++;

    // 3 00000000 41BAval0000            	mov r10, val

    *(bin->currentp++) = 0x41;
    *(bin->currentp++) = 0xBA;

    bin->size += 2;

    WriteAddrOutFunc (bin);

    // 3 00000000 41FFD2                  	call r10

    *(bin->currentp++) = 0x41;
    *(bin->currentp++) = 0xFF;
    *(bin->currentp++) = 0xD2;

    bin->size += 3;

    softBin->currentp++;

    return OK;
}

int WriteAddrOutFunc (BinCode *bin) {
    assert (bin);

    // addr of func is 3byte val

    unsigned long addr = (unsigned long)Out;

    for (int i = 0; i < 3; i++) {
        *(bin->currentp++) = *((unsigned char *)&addr + i);
    }

    *(bin->currentp++) = 0x00;

    bin->size += 4;

    return OK;
}

int WriteAddrInFunc (BinCode *bin) {
    assert (bin);

    // addr of func is 3byte val

    unsigned long addr = (unsigned long)In;

    printf ("pointer - %p\n", (void *)In);
    for (int i = 0; i < 3; i++) {
        *(bin->currentp++) = *((unsigned char *)&addr + i);
        printf ("%X ", *((unsigned char *)&addr + i));
    }
    printf ("\n");

    *(bin->currentp++) = 0x00;

    bin->size += 4;

    return OK;
}

int WriteRet (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);

    *(bin->currentp++) = 0xC3;
    bin->size++;

    softBin->currentp++;

    return OK;
}

int WriteHlt (BinCode *bin, BinCode *softBin) {
    assert (bin);

    RestoreRetAddr (bin, softBin);

    *(bin->currentp++) = 0xC3;
    bin->size++;

    softBin->currentp++;

    return OK;
}

int WriteIn (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);

    // 2 00000000 4989C7                  	mov r14, rax
    *(bin->currentp++) = 0x49;
    *(bin->currentp++) = 0x89;
    *(bin->currentp++) = 0xC6;

    // 3 00000000 41BAval0000            	mov r10, val
    *(bin->currentp++) = 0x41;
    *(bin->currentp++) = 0xBA;
    WriteAddrInFunc (bin);

    // 2 00000000 55                      	push rbp
    *(bin->currentp++) = 0x55;

    // 3 00000000 41FFD2                  	call r10
    *(bin->currentp++) = 0x41;
    *(bin->currentp++) = 0xFF;
    *(bin->currentp++) = 0xD2;

    // 5 00000004 5D                      	pop rbp
    *(bin->currentp++) = 0x5D;

    //  2 00000000 50                      	push rax
    *(bin->currentp++) = 0x50;

    // 4 00000003 4C89F8                  	mov rax, r14
    *(bin->currentp++) = 0x4C;
    *(bin->currentp++) = 0x89;
    *(bin->currentp++) = 0xF0;

    bin->size += 14;

    softBin->currentp++;

    return OK;
}

int PutLong (BinCode *bin, int *addr) {
    assert (bin);
    assert (addr);

    int sizeOfLong = sizeof (long);

    unsigned long tmp = (unsigned long)addr;

    printf ("pointer - %p\n", addr);
    for (int i = 0; i < sizeOfLong; i++) {
        *(bin->currentp++) = *((unsigned char *)&tmp + i);
        printf ("%X ", *((unsigned char *)&tmp + i));
    }
    printf ("\n");

    bin->size += 8;

    return OK;
}

int In () {
    int value = 0;

    printf ("value pointer - %p\n", &value);

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
}

void Out (const int value) {

    printf ("%d\n", value);

}

int CheckBuffOverflow (BinCode *bin, BinCode *softBin) {
    assert (bin);
    assert (softBin);

    int length = bin->currentp - bin->buff;

    if (length >= bin->capacity - backupVal) {
        void *ptr = bin->buff;
        ptr = realloc (ptr, bin->capacity * 2 * sizeof (char));
        if (ptr == nullptr) return ALLOCFAILED;

        bin->buff = (unsigned char *)ptr;
        bin->currentp = bin->buff + length;
        bin->capacity *= 2;
    }

    if (*softBin->currentp == '\0') {
        WriteHlt (bin, softBin);

        return NULLELEMENT;
    }

    return OK;
}