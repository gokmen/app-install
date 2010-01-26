Summary: Tools for managing application install data
Name: app-install
Version: 002
Release: 1
License: GPLv2+
Group: System Environment/Libraries
URL: http://github.com/hughsie/app-install
Source0: http://www.packagekit.org/releases/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: glib2-devel
BuildRequires: sqlite-devel
BuildRequires: intltool
BuildRequires: gettext

%description 
These tools are used when software sources register and unregister database
entries.

%prep
%setup -q

%build
%configure

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
# we don't have any translation just yet... soon
#%find_lang %name

%clean
rm -rf $RPM_BUILD_ROOT

%post
# we create the database file if it does not exist
/usr/sbin/app-install-create

%files
#%files -f %{name}.lang
%defattr(-,root,root,-)
%doc AUTHORS COPYING ChangeLog
%{_sbindir}/app-install-generate-yum.py*
%{_sbindir}/app-install-add
%{_sbindir}/app-install-create
%{_sbindir}/app-install-generate
%{_sbindir}/app-install-remove
%dir %{_datadir}/app-install
%dir %{_datadir}/app-install/docs
%dir %{_datadir}/app-install/icons
%dir %{_datadir}/app-install/desktop
%dir %{_localstatedir}/lib/app-install
%{_datadir}/app-install/docs/app-install-v1.draft

%changelog
* Fri Mar 06 2009 Richard Hughes <rhughes@redhat.com> - 002-1
- Initial build.

