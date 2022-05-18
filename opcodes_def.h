DEF_CMD (push, 0x01, 1, 
{
    ArrangePushPop (bin, softBin, label);
})

DEF_CMD (pop, 0x02, 1, 
{
    ArrangePushPop (bin, softBin, label);
})

DEF_CMD (hlt, 0x00, 0,
{
    WriteHlt (bin, softBin, label);
})

DEF_CMD (ret, 0x11, 0,
{
    WriteRet (bin, softBin, label);
})

DEF_CMD (inc, 0x13, 0, 
{
    WriteInc (bin, softBin, label);
})

DEF_CMD (dec, 0x10, 0, 
{
    WriteDec (bin, softBin, label);
})

DEF_CMD (add, 0x03, 0,
{
    WriteAdd (bin, softBin, label);
})

DEF_CMD (sub, 0x05, 0,
{
    WriteSub (bin, softBin, label);
})

DEF_CMD (out, 0x06, 0,
{
    WriteOut (bin, softBin, label);
})

DEF_CMD (in, 0x07, 0,
{
    WriteIn (bin, softBin, label);
})

DEF_CMD (jmp, 0x0D, 2,
{
    WriteJmp (bin, softBin, label);
})