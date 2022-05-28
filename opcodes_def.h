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

DEF_CMD (je, 0x0D, 2,
{
    WriteJmp (bin, softBin, label);
})

DEF_CMD (jb, 0x14, 2,
{
    WriteJl (bin, softBin, label);
})

DEF_CMD (call, 0x12, 2,
{
    WriteCall (bin, softBin, label);
})

DEF_CMD (mul, 0x04, 0,
{
    WriteMul (bin, softBin, label);
})

DEF_CMD (div, 0x09, 0,
{
    WriteDiv (bin, softBin, label);
})

DEF_CMD(sqrt, 0x08, 0, 
{
    WriteSqrt (bin, softBin, label);
})