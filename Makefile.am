AUTOMAKE_OPTIONS = 1.7

SUBDIRS = po src docs contrib data

snapshot:
	$(MAKE) dist distdir=$(PACKAGE)-$(VERSION)-`date +"%Y%m%d"`

DISTCLEANFILES =					\
	app-install-*.tar.gz				\
	intltool-extract				\
	intltool-merge					\
	intltool-update

MAINTAINERCLEANFILES =					\
	*~						\
	intltool-*.in					\
	compile						\
	configure					\
	INSTALL						\
	install-sh					\
	missing						\
	mkinstalldirs					\
	config.guess					\
	ltmain.sh					\
	config.sub					\
	depcomp						\
	Makefile.in					\
	config.h.*					\
	aclocal.m4

EXTRA_DIST =						\
	COPYING						\
	MAINTAINERS					\
	AUTHORS						\
	INSTALL						\
	README						\
        TODO						\
	NEWS						\
        autogen.sh					\
	config.h					\
	ChangeLog					\
        intltool-extract.in				\
        intltool-merge.in				\
        intltool-update.in				\
	$(NULL)

