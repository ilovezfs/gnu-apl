if ! WANT_ANDROID
EXTRA_DIRS = APs emacs_mode sql workspaces
else
EXTRA_DIRS =
endif
SUBDIRS = native $(EXTRA_DIRS)
DIST_SUBDIRS = $(SUBDIRS) testcases

bin_PROGRAMS = apl

datadir = @datadir@
localedir = $(datadir)/locale

apl_SOURCES = \
			buildtag		buildtag.hh		\
						../config.h		\
						APL_types.hh		\
Archive.cc					Archive.hh		\
ArrayIterator.cc				ArrayIterator.hh	\
Assert.cc					Assert.hh		\
Avec.cc			Avec.def		Avec.hh			\
Backtrace.cc					Backtrace.hh		\
Bif_F12_FORMAT.cc				Bif_F12_FORMAT.hh	\
Bif_OPER1_COMMUTE.cc				Bif_OPER1_COMMUTE.hh	\
Bif_OPER1_EACH.cc				Bif_OPER1_EACH.hh	\
Bif_OPER2_PRODUCT.cc				Bif_OPER2_PRODUCT.hh	\
Bif_OPER2_RANK.cc				Bif_OPER2_RANK.hh	\
Bif_OPER1_REDUCE.cc				Bif_OPER1_REDUCE.hh	\
Bif_OPER1_SCAN.cc				Bif_OPER1_SCAN.hh	\
CDR.cc						CDR.hh			\
Cell.cc						Cell.hh			\
CharCell.cc					CharCell.hh		\
CollatingCache.cc				CollatingCache.hh	\
Command.cc		Command.def		Command.hh		\
Common.cc					Common.hh		\
ComplexCell.cc				    	ComplexCell.hh		\
Cores.cc					Cores.hh		\
DiffOut.cc					DiffOut.hh		\
DynamicObject.cc				DynamicObject.hh	\
						EOC_handler_args.hh	\
Error.cc		Error.def		Error.hh		\
						ErrorCode.hh		\
Executable.cc					Executable.hh		\
FloatCell.cc					FloatCell.hh		\
Function.cc					Function.hh		\
DerivedFunction.cc				DerivedFunction.hh	\
Id.cc			Id.def			Id.hh			\
IndexExpr.cc					IndexExpr.hh		\
IndexIterator.cc				IndexIterator.hh	\
Input.cc					Input.hh		\
InputFile.cc					InputFile.hh		\
IntCell.cc					IntCell.hh		\
LApack.cc								\
LibPaths.cc					LibPaths.hh		\
Logging.cc	    Logging.def			Logging.hh		\
LvalCell.cc					LvalCell.hh		\
main.cc						main.hh			\
						makefile.h		\
Malloc_hooks.cc								\
Nabla.cc					Nabla.hh		\
NamedObject.cc					NamedObject.hh		\
NativeFunction.cc				NativeFunction.hh	\
						Native_interface.hh	\
NumericCell.cc					NumericCell.hh		\
Output.cc					Output.hh		\
Parser.cc					Parser.hh		\
Prefix.cc		Prefix.def		Prefix.hh		\
PointerCell.cc					PointerCell.hh		\
PrimitiveFunction.cc				PrimitiveFunction.hh	\
PrimitiveOperator.cc				PrimitiveOperator.hh	\
PrintBuffer.cc					PrintBuffer.hh		\
						PrintContext.hh		\
						PrintOperator.hh	\
QuadFunction.cc					QuadFunction.hh		\
ProcessorID.cc					ProcessorID.hh		\
Quad_FX.cc					Quad_FX.hh		\
Quad_RL.cc					Quad_RL.hh		\
Quad_SVx.cc					Quad_SVx.hh		\
Quad_TF.cc					Quad_TF.hh		\
RealCell.cc					RealCell.hh		\
Shape.cc					Shape.hh		\
						SharedValuePointer.hh	\
						Simple_string.hh	\
ScalarFunction.cc				ScalarFunction.hh	\
						Source.hh		\
StateIndicator.cc				StateIndicator.hh	\
StaticObjects.cc							\
Svar_DB.cc					Svar_DB.hh		\
Svar_record.cc					Svar_record.hh		\
udp_signal.m4		Svar_signals.def	Svar_signals.hh		\
Symbol.cc					Symbol.hh		\
SymbolTable.cc					SymbolTable.hh		\
			SystemLimits.def	SystemLimits.hh		\
SystemVariable.cc	SystemVariable.def	SystemVariable.hh	\
IO_Files.cc					IO_Files.hh		\
Token.cc		Token.def		Token.hh		\
						TokenEnums.hh		\
Tokenizer.cc					Tokenizer.hh		\
UCS_string.cc					UCS_string.hh		\
						Unicode.hh		\
UserFunction.cc					UserFunction.hh		\
UserPreferences.cc				UserPreferences.hh	\
UTF8_string.cc					UTF8_string.hh		\
Value.cc		Value.icc		Value.hh		\
ValueHistory.cc					ValueHistory.hh		\
Workspace.cc					Workspace.hh


# normally we would say:
#
# Svar_signals.hh: Svar_signals.def udp_signal.m4
#
# but what if m4 is missing? So we explicitly 'make update-svar_signals'
# when Svar_signals.def or udp_signal.m4 have changed (which rarely happens)
#
.PHONY:	update-svar_signals
update-svar_signals:
	m4 -D protocol=Svar_signals udp_signal.m4 > Svar_signals.hh

AM_CXXFLAGS = $(GPROF_WANTED) $(CXX_RDYNAMIC) $(OPENMP)

if DEVELOP
   AM_CXXFLAGS += -Werror -Wall
   AM_CXXFLAGS += -Wno-sign-compare -Wno-strict-aliasing
endif

apl_LDADD = $(LIBS) $(LIBREADLINE)

apl.lines: apl
	objdump -j .text -l -d $< > $@

test: all
	./apl --id 1010 -T testcases/*tc

test1: all
	./apl --id 1010 --TM 3 -T testcases/*\.tc

test2: all
	./apl --id 1010 --TR -T testcases/*tc

# run testcases related to APs
testAP: all
	./apl --id 1010 -T testcases/AP*tc

perf: apl
	./apl --id 1010 --TM 3 -T testcases/Performance.pt


AM_MAKEFLAGS = -j 8

makefile.h: Makefile
	@echo "// some strings exported from Makefile"           > $@
	@echo "//"                                              >> $@
	@echo "#define Makefile__sysconfdir \"${sysconfdir}\""  >> $@
	@echo "#define Makefile__pkglibdir  \"${pkglibdir}\""   >> $@
	@echo "#define Makefile__localedir  \"${localedir}\""   >> $@
	@echo                                                   >> $@

.PHONY:	patch
patch:
	mkdir -p patches
	svn diff > patches/`date +patch_%F-%R.diff`

