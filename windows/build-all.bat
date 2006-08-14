call vcvars32
call copy-files
msdev keymap2cpp.dsp /make "keymap2cpp - Win32 Release" /rebuild
Release\keymap2cpp
msdev skin2cpp.dsp /make "skin2cpp - Win32 Release" /rebuild
Release\skin2cpp
msdev Free42Binary.dsp /make "Free42Binary - Win32 Release" /rebuild
msdev Free42Decimal.dsp /make "Free42Decimal - Win32 Release" /rebuild
