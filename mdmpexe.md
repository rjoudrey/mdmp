# Introduction #

The tool **mdmp.exe** is a Windows **process memory dumper**. It demonstrates some of the available functionality in libmdmp: it can dump **PE images, stacks, heaps** and other memory areas.
Run it without arguments to get the usage help below.

# Usage #

## Process selection ##
|`/a`|dump from all processes (default)|
|:---|:--------------------------------|
|`/p:###`|by PID (dump from process with PID = **###** (decimal))|
|`/n:###`|by name (dump from process with image name containing "**###**")|

## Dump target selection ##
_default_: main executable image(s)
|`/m`|all the memory from the selected process(es) - smart (recognize images, stacks, heaps)|
|:---|:-------------------------------------------------------------------------------------|
|`/M`|all the memory from the selected process(es) - direct                                 |
|`/x`|all executable images                                                                 |
|`/e:###`|executable image(s) containing "###" in the name                                      |
|`/b:###`|executable image(s) with imagebase = ### (hex)                                        |
|`/r:###:$$$`|memory region: $$$ (hex) bytes from address ### (hex)                                 |
|`/k`|all stacks                                                                            |
|`/h`|all heaps                                                                             |
|`/X`|all areas with eXecutable attribute set                                               |

## Options ##
|`/F`|DON'T fix image dumps|
|:---|:--------------------|
|`/I`|fix imports          |
|`/A`|sort dumped file names by address|

## Notes ##
  * at least one of target or process selection required
  * "/" can be replaced with "-"