dnl Process this file with autoconf to produce a configure script.
AC_INIT(main.c)
AC_CANONICAL_SYSTEM
AC_CONFIG_HEADER(config.h)


dnl Checks for programs.
AC_PROG_CC

EXTRA_OBJS=
dnl Do user command line args.. AFTER getting AC_PROG_CC to set CFLAGS et al. for us
AC_ARG_ENABLE(instr-profile,
[  --enable-instr-profile  compile with instruction profiling (timing)],
[if test "$enableval" = "yes"; then 
	AC_DEFINE(INSTRUCTION_TIMING, 1, [Whether each instruction should be timed])
	EXTRA_OBJS="$EXTRA_OBJS profile.o"
fi])

AC_ARG_ENABLE(mem-stats,
[  --enable-mem-stats      gather memory stats while running],
[if test "$enableval" = "yes"; then
	AC_DEFINE(MEMORY_STATS, 1, [Whether stats about the number and type of address]
				   [ accesses should be generated])
	EXTRA_OBJS="$EXTRA_OBJS stats.o"
fi])

AC_ARG_ENABLE(profile,
[  --enable-profile        adds -pg to the compile/link options],
[if test "$enableval" = "yes";
 	then LDFLAGS="$LDFLAGS -pg"; CFLAGS="$CFLAGS -pg";
fi])

dnl I thought about this, but if it was optional, then we couldn't fire the
dnl  timer based on cpu cycles which have passed, unless we want more (ugh)
dnl  ifdefs
dnl AC_ARG_ENABLE(cycle-counting,
dnl [  --enable-cycle-counting Keep record of CPU cycles for each instruction],
dnl [if test "$enableval" = "yes"; then
dnl  	AC_DEFINE(USE_CYCLE_COUNTING, 1, [Keep record of CPU cycles used by each instruction])
dnl 	EXTRA_OBJS="$EXTRA_OBJS cycle.o"
dnl fi])

AC_SUBST(EXTRA_OBJS)

dnl Check for cygwin windows environment
AC_CYGWIN

dnl Checks for libraries.

dnl In windows, we need to check for the following, in unix, we don't
if test "$CYGWIN" = "yes"; then
	dnl Replace `main' with a function in -lgmon:
	AC_CHECK_LIB(gmon, main)
	dnl Replace `main' with a function in -lwininet:
	AC_CHECK_LIB(wininet, main)
fi

dnl should we check for curses terminfo termlib, too?
for lib in ncurses termcap ; do
   AC_CHECK_LIB(${lib}, tgoto, [LIBS="-l${lib} $LIBS"; break])
done
AC_CHECK_LIB(readline, readline,)

AC_CHECK_LIB(pthread, pthread_create,)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h sys/time.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME

dnl Chech endianness
AC_C_BIGENDIAN

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
dnl Check for -lnsl for Solaris
AC_CHECK_FUNCS(gethostbyname,, 
	AC_CHECK_LIB(nsl, gethostbyname,, AC_CHECK_LIB(socket, gethostbyname,, , -lnsl), -lsocket))
dnl Check for -lsocket for Solaris
AC_CHECK_FUNCS(connect,,AC_CHECK_LIB(socket,connect))
AC_CHECK_FUNCS(select socket,,,$LIBS)

AC_CHECK_FUNCS(_snprintf snprintf)
	
AC_MSG_CHECKING(for unaligned long accesses)
AC_TRY_RUN([
	void main(void)
	{	unsigned char data[sizeof(long)*2];
		long *ptr;

		#if !defined(__i386__) /* Assume unaligned accesses on i386 */
		exit(1);
		#endif

		data[0] = 0xa5;
		ptr = (long *)(&data[1]);
		*ptr = 0x1a253a45;

		if(data[0] == 0xa5 && *ptr == 0x1a253a45) exit(0);
		exit(1);
	}],
	AC_MSG_RESULT(permitted) AC_DEFINE(UNALIGNED_ACCESS,1,
		[Define if the system can do a unaligned longword access]),
	AC_MSG_RESULT(not permitted), 
	AC_MSG_RESULT(unknown, assuming not permitted)
)
	
MAKEFILE_RULES=Makefile.rules
AC_SUBST_FILE(MAKEFILE_RULES)

AC_OUTPUT( [Makefile.rules
	Makefile 
	isa_a/Makefile
	isa_b/Makefile
	monitor/Makefile
	peripherals/Makefile
	tracer/Makefile ])
