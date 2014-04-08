AC_INIT([eudev],[1.5.3])
AC_CONFIG_SRCDIR([src/udev/udevd.cxx])
AC_CONFIG_AUX_DIR([autom4te.cache])
AC_CONFIG_MACRO_DIR([autom4te.cache])
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])

LT_INIT

# Checks for programs.
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL

# Set paths here
AC_ARG_WITH(
        [runstatedir],
        [AS_HELP_STRING(
		        [--with-runstatedir=DIR],
                [Where runtime subdirectory will be created (default=/var/run/udev)])],
        [],
        [with_runstatedir="/var/run/udev"]
)

AC_SUBST([runstatedir], [$with_runstatedir])
AC_SUBST(UDEV_VERSION, 211)

datadir=${datadir}/udev
libexecdir=${libdir}/udev

AC_CONFIG_FILES([Makefile
                 src/udev/Makefile])

AC_OUTPUT

# ------------------------------------------------------------------------------

AC_MSG_RESULT([
        prefix:                  ${prefix}
        runstatedir:             ${runstatedir}
])

