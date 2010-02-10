## We require new-style dependency handling.
AUTOMAKE_OPTIONS = 1.7

INCLUDES =						\
	$(GLIB_CFLAGS)					\
	$(GIO_CFLAGS)					\
	$(ARCHIVE_CFLAGS)				\
	$(SQLITE_CFLAGS)				\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"		\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 		\
	-DDATADIR=\""$(datadir)"\" 			\
	-DBINDIR=\""$(bindir)"\" 			\
	-DLOCALSTATEDIR=\""$(localstatedir)"\" 		\
	-DEGG_LOG_FILE=\""$(LOG_DIR)/app-install"\"	\
	-DEGG_VERBOSE="\"VERBOSE\""			\
	-DEGG_LOGGING="\"LOGGING\""			\
	-DEGG_CONSOLE="\"CONSOLE\""

sbin_PROGRAMS = app-install-create app-install-remove app-install-add

bin_PROGRAMS = app-install-extract-package app-install-generate

app_install_extract_package_SOURCES =			\
	ai-extract-package.c				\
	$(NULL)
app_install_extract_package_LDADD = $(GLIB_LIBS) $(ARCHIVE_LIBS)
app_install_extract_package_CFLAGS = $(WARNINGFLAGS_C)

app_install_create_SOURCES =				\
	egg-debug.c					\
	egg-debug.h					\
	ai-create.c					\
	ai-common.h					\
	$(NULL)
app_install_create_LDADD = $(GIO_LIBS) $(GLIB_LIBS) $(SQLITE_LIBS)
app_install_create_CFLAGS = $(WARNINGFLAGS_C)

app_install_remove_SOURCES =				\
	egg-debug.c					\
	egg-debug.h					\
	ai-remove.c					\
	ai-common.h					\
	$(NULL)
app_install_remove_LDADD = $(GIO_LIBS) $(GLIB_LIBS) $(SQLITE_LIBS)
app_install_remove_CFLAGS = $(WARNINGFLAGS_C)

app_install_add_SOURCES =				\
	egg-debug.c					\
	egg-debug.h					\
	ai-add.c					\
	ai-common.h					\
	$(NULL)
app_install_add_LDADD = $(GIO_LIBS) $(GLIB_LIBS) $(SQLITE_LIBS)
app_install_add_CFLAGS = $(WARNINGFLAGS_C)

app_install_generate_SOURCES =				\
	egg-debug.c					\
	egg-debug.h					\
	ai-generate.c					\
	ai-common.h					\
	$(NULL)
app_install_generate_LDADD = $(GIO_LIBS) $(GLIB_LIBS) $(SQLITE_LIBS)
app_install_generate_CFLAGS = $(WARNINGFLAGS_C)

install-data-hook:
	if test -w $(DESTDIR)$(prefix)/; then \
		mkdir -p $(DESTDIR)$(localstatedir)/lib/app-install; \
		mkdir -p $(DESTDIR)$(datadir)/app-install/icons; \
		mkdir -p $(DESTDIR)$(datadir)/app-install/desktop; \
	fi

