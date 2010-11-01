import pymdmp
import os

errorTexts = dict([(getattr(pymdmp, x), x) for x in dir(pymdmp) if x.startswith('MDMP_ERR_')])

def dump(procSelMode, dumpSelMode, flags, **kargs):
    res = pymdmp.dump(procSelMode, dumpSelMode, flags, **kargs)

    if type(res) == type([]):
       for e in res:
           open(r'%s-%s-%d-%08X.dump' % e[:-1], 'wb').write(e[-1])
    else:
       print 'Error: %08X (%s)!' % (res, errorTexts[res])

dump(pymdmp.SEL_BY_NAME, pymdmp.DUMP_IMAGE_BY_NAME, 0, processName='explo', moduleName='kern')

dump(pymdmp.SEL_BY_PID, pymdmp.DUMP_MAIN_IMAGE, 0, processID=1234)