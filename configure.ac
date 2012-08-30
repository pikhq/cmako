AC_INIT([cmako], [git])
AC_CONFIG_AUX_DIR([build-aux])

m4_include([build-aux/lean.m4])

AM_INIT_AUTOMAKE([1.11 silent-rules foreign subdir-objects])
AM_SILENT_RULES([yes])

AC_PROG_CC_C99

AM_PATH_SDL()
PKG_CHECK_MODULES([SDL_GFX], [SDL_gfx])

AC_MSG_CHECKING([if we have computed goto])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
			int main() {void *foo = &&FOO; FOO: goto *foo;}])],
		[
			AC_DEFINE([HAVE_COMPUTED_GOTO], [1])
			AC_MSG_RESULT([yes])
		], [
			AC_MSG_RESULT([no])
		])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
