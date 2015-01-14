VUOCOMPILE_TYPE_FLAGS = --header-search-path $${ROOT}/node \
	--header-search-path $${ROOT}/node \
	--header-search-path $${ROOT}/type \
	--header-search-path $${ROOT}/type/list \
	--header-search-path $${ROOT}/runtime \
	--header-search-path $${ICU_ROOT}/include \
	--header-search-path $${JSONC_ROOT}/include
type.input = TYPE_SOURCES
type.depend_command = $$QMAKE_CC -o /dev/null -E -MD -MF - \
	-I$${ROOT}/node \
	-I$${ROOT}/type \
	-I$$ROOT/type/list \
	-I$$ROOT/runtime \
	-I$$ICU_ROOT/include \
	-I$$JSONC_ROOT/include \
	${QMAKE_FILE_NAME} | sed \"s,^.*: ,,\"
type.output = ${QMAKE_FILE_IN_BASE}.bc
type.commands = $$VUOCOMPILE $$VUOCOMPILE_TYPE_FLAGS --output ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
type.variable_out = TYPE_OBJECTS
type.CONFIG = target_predeps
QMAKE_EXTRA_COMPILERS += type
