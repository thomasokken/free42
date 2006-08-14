path "C:\Program Files\Microsoft eMbedded Tools\Common\EVC\Bin";%PATH%
path "C:\Windows CE Tools\wce300\Pocket PC 2002\support\ActiveSync\windows ce application installation\cabwiz";%PATH%
evc Free42Binary.vcp /make "Free42Binary - Win32 (WCE ARM) Release" /rebuild
evc Free42Decimal.vcp /make "Free42Decimal - Win32 (WCE ARM) Release" /rebuild
cabwiz Free42Binary.inf /cpu PPC2002_ARM
cabwiz Free42Decimal.inf /cpu PPC2002_ARM
