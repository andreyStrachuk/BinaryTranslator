#include "translator.h"

FILE *global = fopen ("testik.asm", "w");

#define NEWSHIFT softBin->currentp - softBin->buff

#define PUTBYTE(arg)    *(bin->currentp++) = arg;   \
                        bin->size++;                \


const char pushByte = 0x6A;
const char pushVal  = 0x68;

const char backupVal = 25;      // 0 < size < capacity - backupVal


#define DEF_CMD(name, number, numOfParams, code)        if (typeOfCmd == number) {      \
                                                            code                        \
                                                        }                               \

int BinaryTranslate (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);

    printf ("in tr!\n");

    int res = CheckSignature (softBin);
    if (res != OK) return WRONGSIGNATURE;

    SaveRetAddr (bin, softBin);

    while (true) {
        res = CheckBuffOverflow (bin, softBin, label);
        printf ("res - %d\n", res);
        if (res == NULLELEMENT) break;

        int typeOfCmd = 0x1F & *(softBin->currentp);

        #include "../opcodes_def.h"
    }

    // fwrite (bin->buff, bin->size, sizeof (char), global);

    printf ("bin size = %d\n", bin->size);

    for (int i = 0; i < bin->size; i++) {
        printf ("%X ", bin->buff[i]);
    }
    printf ("\n\n");

    printf ("printing array-----------------------------\n");

    for (int i = 0; i < label->size; i++) {
        printf ("%2d ", label->bin[i]);
    }
    printf ("\n");

    for (int i = 0; i < label->size; i++) {
        printf ("%2d ", label->softBin[i]);
    }
    printf ("\n");

    printf ("end of printing array----------------------\n");

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

        fprintf (global, "push rax\nmov rax, %d\npush [rax]\npop rax", shift);

        PUTBYTE (0x50);         // push [val]
        PUTBYTE (0xB8);

        PutInt (bin, shift);

        PUTBYTE (0xFF);
        PUTBYTE (0x30);

        PUTBYTE (0x50);

        LabelPushBack (label, bin->size, NEWSHIFT);

        return OK;
    }
    else if (res == MEMREG) {
        char regNum = *softBin->currentp;
        softBin->currentp++;

        fprintf (global, "push [%d - reg num]\n", regNum);

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

        fprintf (global, "push [%d -reg + %d]\n", regNum, shift);

        PUTBYTE (0xFF);
        PUTBYTE (0xB0 + regNum);

        PutInt (bin, shift);

        LabelPushBack (label, bin->size, NEWSHIFT);

        return OK;
    }

    res = CheckIfReg (typeOfCmd);
    if (res == REG) {
        char regNum = *softBin->currentp;
        softBin->currentp += 2;

        fprintf (global, "push %d - reg\n", regNum);

        PUTBYTE (0x50 + regNum);

        LabelPushBack (label, bin->size, NEWSHIFT);

        return OK;
    }

    res = CheckIfImm (typeOfCmd);
    if (res == IMM) {
        double value = *(double *)softBin->currentp;
        softBin->currentp += 8;

        fprintf (global, "push %d\n", (int)value);

        PUTBYTE (pushVal);

        PutInt (bin, (int)value);

        LabelPushBack (label, bin->size, NEWSHIFT);

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
        printf ("reg num - %X\n", regNum);

        PUTBYTE (0x58 + regNum);

        LabelPushBack (label, bin->size, NEWSHIFT);
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

    LabelPushBack (label, bin->size, NEWSHIFT);

    return OK;
}



int WriteDec (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);

    softBin->currentp++;

    //  3 00000000 4989C5                  	mov r13, rax
    //  4 00000003 58                      	pop rax
    //  5 00000004 48FFC8                  	dec rax
    //  6 00000007 50                      	push rax
    //  7 00000008 4C89E8                  	mov rax, r13

    PUTBYTE (0x49); PUTBYTE (0x89); PUTBYTE (0xC5);

    PUTBYTE (0x58);

    PUTBYTE (0x48); PUTBYTE (0xFF); PUTBYTE (0xC8);

    PUTBYTE (0x50);

    PUTBYTE (0x4C); PUTBYTE (0x89); PUTBYTE (0xE8);

    LabelPushBack (label, bin->size, NEWSHIFT);

    printf ("DONE\n");

    return OK;
}

int WriteAdd (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);

    softBin->currentp++;

    //  3 00000000 4989C5                  	mov r13, rax
    //  4 00000003 4989DC                  	mov r12, rbx
    //  5 00000006 58                      	pop rax
    //  6 00000007 5B                      	pop rbx
    //  7 00000008 4801D8                  	add rax, rbx
    //  8 0000000B 50                      	push rax
    //  9 0000000C 4C89E8                  	mov rax, r13
    // 10 0000000F 4C89E3                  	mov rbx, r12

    PUTBYTE (0x49); PUTBYTE (0x89); PUTBYTE (0xC5);

    PUTBYTE (0x49); PUTBYTE (0x89); PUTBYTE (0xDC);

    PUTBYTE (0x58);

    PUTBYTE (0x5B);

    PUTBYTE (0x48); PUTBYTE (0x01); PUTBYTE (0xD8);

    PUTBYTE (0x50);

    PUTBYTE (0x4C); PUTBYTE (0x89); PUTBYTE (0xE8);

    PUTBYTE (0x4C); PUTBYTE (0x89); PUTBYTE (0xE3);

    LabelPushBack (label, bin->size, NEWSHIFT);

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

    LabelPushBack (label, bin->size, NEWSHIFT);

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

    // 3 00000000 41BAval0000            	mov r10, val

    PUTBYTE (0x41);
    PUTBYTE (0xBA);

    WriteAddrOutFunc (bin);

    // 3 00000000 41FFD2                  	call r10

    PUTBYTE (0x41);
    PUTBYTE (0xFF);
    PUTBYTE (0xD2);

    LabelPushBack (label, bin->size, NEWSHIFT);

    return OK;
}

int WriteAddrOutFunc (BinCode *bin) {
    assert (bin);

    // addr of func is 3byte val

    unsigned long addr = (unsigned long)Out;

    for (int i = 0; i < 3; i++) {
        PUTBYTE (*((unsigned char *)&addr + i));
    }

    PUTBYTE (0x00);

    return OK;
}

int WriteAddrInFunc (BinCode *bin) {
    assert (bin);

    // addr of func is 3byte val

    unsigned long addr = (unsigned long)In;

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

    LabelPushBack (label, bin->size, NEWSHIFT);

    return OK;
}

int WriteHlt (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);

    softBin->currentp++;

    RestoreRetAddr (bin, softBin);

    PUTBYTE (0xC3);

    LabelPushBack (label, bin->size, NEWSHIFT);

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
    WriteAddrInFunc (bin);

    // 3 00000000 4831C0                  	xor rax, rax
    PUTBYTE (0x48); PUTBYTE (0x31); PUTBYTE (0xC0);

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

    //  2 00000000 50                      	push rax
    PUTBYTE (0x50);

    // 4 00000003 4C89F8                  	mov rax, r14
    PUTBYTE (0x4C); PUTBYTE (0x89); PUTBYTE (0xF0);

    LabelPushBack (label, bin->size, NEWSHIFT);

    return OK;
}

int WriteJmp (BinCode *bin, BinCode *softBin, Label *label) {
    assert (bin);
    assert (softBin);

    softBin->currentp++;

    // 3 00000000 5F                      	pop rdi
    // 4 00000001 5E                      	pop rsi

    PUTBYTE (0x5F); PUTBYTE (0x5E);

    // 3 00000000 4839FE                  	cmp rsi, rdi
    
    PUTBYTE (0x48); PUTBYTE (0x39); PUTBYTE (0xFE);
    
    // 4 00000003 74**                    	je label
    PUTBYTE (0x0F);
    PUTBYTE (0x84);

    int absShift = *(char *)(softBin->currentp);
    softBin->currentp += 2;

    int current = bin->currentp - bin->buff - 2;

    int res = CheckIfLblContainsAddr (label, absShift);
    if (res != -1) {

        PutInt (bin, label->bin[res] - current - 3);
    }
    else {
        // 3 00000000 90                      	nop
        PUTBYTE (0x90); PUTBYTE (0x90); PUTBYTE (0x90); PUTBYTE (0x90);
    }

    LabelPushBack (label, bin->size, NEWSHIFT);

    return OK;
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

    if (length >= bin->capacity - backupVal) {
        void *ptr = bin->buff;
        ptr = realloc (ptr, bin->capacity * 2 * sizeof (char));
        if (ptr == nullptr) return ALLOCFAILED;

        bin->buff = (unsigned char *)ptr;
        bin->currentp = bin->buff + length;
        bin->capacity *= 2;
    }

    if (softBin->currentp - softBin->buff == softBin->size - 3) {
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

    if (label->size >= label->capacity) {
        LabelsResize (label);
    }

    label->softBin[label->size] = softVal;
    label->bin[label->size]     = binVal;

    label->size++;

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