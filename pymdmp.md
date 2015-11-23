# Introduction #

The library pymdmp.pyd implements Python bindings for the libmdmp module.

# Example #
```
import pymdmp

results = pymdmp.dump(pymdmp.SEL_BY_NAME, pymdmp.DUMP_IMAGE_BY_NAME, 0, processName="calc", moduleName="kernel")

if type(results) == type([]): # DON'T do this kind of "error-checking" in real life! 
   for res in results:
       open(r'%s-%s-%d-%08X.dump' % res[:-1], 'wb').write(res[-1])
       # produces something similar to calc.exe-pe[kernel32.dll]-3720-7C800000.dump
```

# API #

## pydump.dump() ##
`dump(processSelectionMode, dumpMode, flags, [optionalArgs])`

**Arguments**:
  * processSelectionMode(integer): `SEL_*` constant
  * dumpMode(integer): `DUMP_*` constant
  * flags(integer): OR-combination of `FLAG_*` constants
  * optionalArgs:
    * `processName(string)` if `processSelectionMode == SEL_BY_NAME`
    * `processID(string)` if `processSelectionMode == SEL_BY_PID`
    * `moduleName(string)` if `dumpMode == DUMP_IMAGE_BY_NAME`
    * `moduleAddr(string)` if `dumpMode == DUMP_IMAGE_BY_IMAGEBASE`
    * `regStart(integer)`, `regEnd(integer)` if `dumpMode == DUMP_REGION`

**Return value**: if successful, it returns a list of tuples of the form `(processName, dumpName, pid, dumpAddr, dumpData)`; if it fails, it returns an ERR**_error code._

## pymdmp.ver() ##
Returns the**libmdmp**version as a string.**

## constants ##
**`DUMP_*`**: `DUMP_REGION`, `DUMP_MAIN_IMAGE`, `DUMP_ALL_IMAGES`, `DUMP_IMAGE_BY_IMAGEBASE`, `DUMP_IMAGE_BY_NAME`, `DUMP_STACKS`, `DUMP_HEAPS`, `DUMP_SMART`, `DUMP_EXECUTABLE`, `DUMP_ALL_MEM`

**`SEL_*`**: `SEL_BY_NAME`, `SEL_BY_PID`, `SEL_ALL`

**`FLAG_*`**: `FLAG_DONT_FIX_IMAGES`, `FLAG_FIX_IMPORTS` (not yet implemented), `FLAG_SORT_BY_ADDR`

**Error codes:** `MDMP_OK`, `MDMP_ERR_MEM_ALLOC_FAILED`, `MDMP_ERR_ACCESS_DENIED`, `MDMP_ERR_PROC_SNAPSHOT_FAILED`, `MDMP_ERR_NO_PROCESS_MATCHES`, `MDMP_ERR_READ_MEM_FAILED`, `MDMP_ERR_INVALID_ARGS`, `MDMP_ERR_WRITING_TO_DISK`,
`MDMP_ERR_UNK_ERROR`