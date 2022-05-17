#include "translator.h"

static int cap = 200;

int main (const int argc, const char *argv[]) {
    if (argc < 2) { PrintErrors (WRONGNUMBEROFPARAMETERS); return 1; }

    BinCode binCode {};
    BinCodeInit (&binCode, cap);

    BinCode softBinCode {};
    SoftBinCodeInit (&softBinCode, argv[1]);

    BinaryTranslate (&binCode, &softBinCode);

    int res = mprotect (binCode.buff, binCode.capacity, PROT_EXEC | PROT_WRITE);
    printf ("res - %d\n", res);

    void (* JIT) (void);
    JIT = (void (*) (void)) (binCode.buff);

    JIT();

    printf ("I am here!\n");
    // exit (1);

    return 0;
}