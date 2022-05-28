#include "translator.h"

static int gIntialCapacity = 1000;

int main (const int argc, const char *argv[]) {
    if (argc < 2) { PrintErrors (WRONGNUMBEROFPARAMETERS); return 1; }

    BinCode binCode {};
    BinCodeInit (&binCode, gIntialCapacity);

    BinCode softBinCode {};
    SoftBinCodeInit (&softBinCode, argv[1]);

    Label label {};
    LabelsInit (&label, softBinCode.capacity);

    int res = BinaryTranslate (&binCode, &softBinCode, &label, FIRST);
    if (res != OK) return res;

    res = BinaryTranslate (&binCode, &softBinCode, &label, SECOND);
    if (res != OK) return res;

    res = mprotect (binCode.buff, binCode.capacity, PROT_EXEC | PROT_WRITE);
    if (res != 0) { printf ("Your memory is not aligned by 4096!\n"); return res; }

    void (* JIT) (void) = (void (*) (void)) (binCode.buff);

    SAVEREGS
    JIT ();
    RESTOREREGS
    
    BinCodeDestruct (&binCode);
    BinCodeDestruct (&softBinCode);
    LabelsDestruct (&label);

    return 0;
}
