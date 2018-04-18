Usage
======
Adjusting debug output verbosity
---------------------------------
Prior to compilation of source file 'FsFltStrhide.c', adjust the value of global variable _gTraceFlags_ as a desired bitmask of the _PTDBG\_TRACE\_\*_ #defines.

Depending on the value of _gTraceFlags_ when building the driver, some debug output can be enabled immediately at runtime (via kernel debugger) or on boot (via registry editing) by specifying a 'component filter mask'.

Using a kernel debugger to view or edit the current component filter mask involves the following commands:
```
kd> *Display current value (dd=Display DWORD)
kd> dd nt!Kd_XXX_Mask

kd> *Edit current value (ed=Enter DWORD)
kd> ed nt!Kd_XXX_Mask 0xF

Where XXX is
  DEFAULT (for unconverted DbgPrint output), or
  IHVDRIVER (for DbgPrintEx output)
and 0xF is a bitmask of desired importance level:
  0x1 - Error
  0x2 - Warning
  0x4 - Trace
  0x8 - Informational
  0xF - All of the above
```

For more information (including registry settings for permanent debug output configuration), see 'Microsoft Docs: Reading and Filtering Debugging Messages' at https://docs.microsoft.com/en-us/windows-hardware/drivers/devtest/reading-and-filtering-debugging-messages .
