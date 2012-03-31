AC_INIT([cmako], [git])
AM_INIT_AUTOMAKE([1.11 silent-rules foreign])
AM_SILENT_RULES([yes])

AC_PROG_CC_C99

AC_ARG_WITH([gl], AS_HELP_STRING([--with-gl], [Build with OpenGL accelerated graphics]), [], [with_gl=check])

AS_IF([test "x$with_gl" != "xno"], [
	PKG_CHECK_MODULES([GL], [gl], [
		AC_DEFINE([USE_GL], [1], [Use OpenGL])
	])
])

AM_PATH_SDL()

AC_SUBST([GL_CFLAGS])
AC_SUBST([GL_LIBS])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
