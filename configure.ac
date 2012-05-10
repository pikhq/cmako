# Copyright (c) 2012 cmako contributors
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

AC_INIT([cmako], [git])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.11 silent-rules foreign subdir-objects])
AM_SILENT_RULES([yes])

AC_PROG_CC_C99
AM_PROG_CC_C_O

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
