#include "translator.h"

static int cap = 200;

int main (const int argc, const char *argv[]) {
    if (argc < 2) { PrintErrors (WRONGNUMBEROFPARAMETERS); return 1; }

    BinCode binCode {};
    BinCodeInit (&binCode, cap);

    BinCode softBinCode {};
    SoftBinCodeInit (&softBinCode, argv[1]);

    Label label {};
    LabelsInit (&label, softBinCode.capacity);

    printf ("before\n");
    BinaryTranslate (&binCode, &softBinCode, &label, FIRST);

    softBinCode.buff -= 2;
    softBinCode.currentp = softBinCode.buff;

    binCode.currentp = binCode.buff;
    binCode.size = 0;

    BinaryTranslate (&binCode, &softBinCode, &label, SECOND);
    printf ("after bt\n");

    int res = mprotect (binCode.buff, binCode.capacity, PROT_EXEC | PROT_WRITE);
    printf ("res - %d\n", res);

    void (* JIT) (void);
    JIT = (void (*) (void)) (binCode.buff);

    JIT();

    printf ("I am here!\n");
    // exit (1);

    return 0;
}