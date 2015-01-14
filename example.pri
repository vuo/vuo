include(vuo.pri)

example.input = EXAMPLE_SOURCES
example.output = ${QMAKE_FILE_IN_BASE}
example.depends = $$VUOCOMPILE $$VUOLINK $$ROOT/node/*.vuonode $$ROOT/type/*.bc $$ROOT/type/list/*.bc $$ROOT/library/*.bc
example.commands += \
	$$VUOCOMPILE --output ${QMAKE_FILE_IN_BASE}.bc ${QMAKE_FILE_IN} && \
	$$VUOLINK $$VUOLINK_FLAGS --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN_BASE}.bc
QMAKE_EXTRA_COMPILERS += example
OTHER_FILES += $$EXAMPLE_SOURCES

QMAKE_CLEAN += *.bc
