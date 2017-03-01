%define sourcename @CPACK_SOURCE_PACKAGE_FILE_NAME@
%global dev_version %{lua: extraver = string.gsub('@LIBKVSNS_EXTRA_VERSION@', '%-', '.'); print(extraver) }

Name: libkvsns 
Version: @LIBKVSNS_BASE_VERSION@
Release: 0%{dev_version}%{?dist}
Summary: Library to access to a namespace inside a KVS
License: LGPLv3 
Group: Development/Libraries
Url: http://github.com/phdeniel/libkvsns
Source: %{sourcename}.tar.gz
BuildRequires: cmake hiredis-devel
BuildRequires: gcc
Requires: redis hiredis

%description
The libkvsns is a library that allows of a POSIX namespace built on top of
a Key-Value Store.

%package devel
Summary: Development file for the library libkvsns
Group: Development/Libraries
Requires: %{name} = %{version}-%{release} pkgconfig
Requires: hiredis-devel

%description devel
The libkvsns is a library that allows of a POSIX namespace built on top of
a Key-Value Store.
This package contains tools for libkvsns.

%package utils
Summary: Development file for the library libkvsns
Group: Development/Libraries
Requires: %{name} = %{version}-%{release} pkgconfig
Requires: redis hiredis

%description utils
The libkvsns is a library that allows of a POSIX namespace built on top of
a Key-Value Store.
This package contains the tools for libkvsns.

%prep
%setup -q -n %{sourcename}

%build
cmake .
make %{?_smp_mflags} || make %{?_smp_mflags} || make

%install

mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_libdir}/pkgconfig
mkdir -p %{buildroot}%{_includedir}/kvsns
install -m 644 kvsns/libkvsns.so %{buildroot}%{_libdir}
install -m 644 kvsal/libkvsal.so %{buildroot}%{_libdir}
install -m 644 extstore/libextstore.so %{buildroot}%{_libdir}
install -m 644 include/kvsns/kvsns.h  %{buildroot}%{_includedir}/kvsns
install -m 644 include/kvsns/kvsal.h  %{buildroot}%{_includedir}/kvsns
install -m 644 include/kvsns/extstore.h  %{buildroot}%{_includedir}/kvsns
install -m 644 libkvsns.pc  %{buildroot}%{_libdir}/pkgconfig
install -m 755 kvsns_shell/kvsns_busybox %{buildroot}%{_bindir}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_libdir}/libkvsns.so*
%{_libdir}/libkvsal.so*
%{_libdir}/libextstore.so*

%files devel
%defattr(-,root,root)
%{_libdir}/pkgconfig/libkvsns.pc
%{_includedir}/kvsns/kvsns.h
%{_includedir}/kvsns/kvsal.h
%{_includedir}/kvsns/extstore.h

%files utils
%defattr(-,root,root)
%{_bindir}/kvsns_busybox

%changelog
* Wed Mar  1 2017 Philippe DENIEL <philippe.deniel@cea.fr> 1.1.1
- API refurbished after mero injection

* Wed Nov 16 2016 Philippe DENIEL <philippe.deniel@cea.fr> 1.0.1
- Release candidate

* Mon Aug  1 2016 Philippe DENIEL <philippe.deniel@cea.fr> 0.9.2
- Source code tree refurbished

* Mon Jul 25 2016 Philippe DENIEL <philippe.deniel@cea.fr> 0.9.1
- First alpha version
