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

noinst_LIBRARIES = libaishared.a
libaishared_a_SOURCES =					\
	egg-debug.c					\
	egg-debug.h					\
	ai-database.c					\
	ai-database.h					\
	ai-result.c					\
	ai-result.h					\
	ai-utils.c					\
	ai-utils.h					\
	ai-common.h					\
	$(NULL)

libaishared_a_CFLAGS = $(WARNINGFLAGS_C)

sbin_PROGRAMS = app-install-create app-install-remove app-install-add

bin_PROGRAMS = app-install-extract-package app-install-generate app-install-query

app_install_extract_package_SOURCES =			\
	ai-extract-package.c				\
	$(NULL)
app_install_extract_package_LDADD = $(GLIB_LIBS) $(ARCHIVE_LIBS) libaishared.a
app_install_extract_package_CFLAGS = $(WARNINGFLAGS_C)

app_install_create_SOURCES =				\
	ai-create.c					\
	$(NULL)
app_install_create_LDADD = $(GIO_LIBS) $(GLIB_LIBS) $(ARCHIVE_LIBS) $(SQLITE_LIBS) libaishared.a
app_install_create_CFLAGS = $(WARNINGFLAGS_C)

app_install_query_SOURCES =				\
	ai-query.c					\
	$(NULL)
app_install_query_LDADD = $(GIO_LIBS) $(GLIB_LIBS) $(ARCHIVE_LIBS) $(SQLITE_LIBS) libaishared.a
app_install_query_CFLAGS = $(WARNINGFLAGS_C)

app_install_remove_SOURCES =				\
	ai-remove.c					\
	$(NULL)
app_install_remove_LDADD = $(GIO_LIBS) $(GLIB_LIBS) $(ARCHIVE_LIBS) $(SQLITE_LIBS) libaishared.a
app_install_remove_CFLAGS = $(WARNINGFLAGS_C)

app_install_add_SOURCES =				\
	ai-add.c					\
	$(NULL)
app_install_add_LDADD = $(GIO_LIBS) $(GLIB_LIBS) $(ARCHIVE_LIBS) $(SQLITE_LIBS) libaishared.a
app_install_add_CFLAGS = $(WARNINGFLAGS_C)

app_install_generate_SOURCES =				\
	ai-generate.c					\
	$(NULL)
app_install_generate_LDADD = $(GIO_LIBS) $(GLIB_LIBS) $(ARCHIVE_LIBS) $(SQLITE_LIBS) libaishared.a
app_install_generate_CFLAGS = $(WARNINGFLAGS_C)

check_PROGRAMS =					\
	ai-self-test

ai_self_test_SOURCES =					\
	ai-self-test.c					\
	$(libaishared_a_SOURCES)			\
	$(NULL)

ai_self_test_LDADD =					\
	$(GLIB_LIBS)					\
	$(SQLITE_LIBS)					\
	$(GIO_LIBS)					\
	$(ARCHIVE_LIBS)					\
	$(NULL)

ai_self_test_CFLAGS = -DEGG_TEST $(AM_CFLAGS)

TESTS = ai-self-test
install-data-hook:
	if test -w $(DESTDIR)$(prefix)/; then \
		mkdir -p $(DESTDIR)$(localstatedir)/lib/app-install; \
		mkdir -p $(DESTDIR)$(datadir)/app-install/icons; \
		mkdir -p $(DESTDIR)$(datadir)/app-install/desktop; \
	fi

