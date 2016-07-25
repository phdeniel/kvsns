Name: libkvsns 
Version: 0.9.1
Release: 1.0
Summary: Library to access to a namespace inside a KVS
License: LGPLv3 
Group: Development/Libraries
Url: http://github.com/phdeniel/libkvsns
Source0: %{name}-%{version}-Source.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root-%(%{__id_u} -n)
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

%package utils
Summary: Tool to manage a the kvsns namespace
Group: Applications/File
Requires: %{name} = %{version}-%{release}


%description devel
The libkvsns is a library that allows of a POSIX namespace built on top of
a Key-Value Store.
This package contains the development headers for libkvsns.

%description utils
The libkvsns is a library that allows of a POSIX namespace built on top of
a Key-Value Store.
This package contains the tools to administrate the kvsns namespace.


%prep
%setup -q -n %{name}-%{version}-Source

%build
cmake .
make %{?_smp_mflags} || make %{?_smp_mflags} || make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_libdir}
mkdir -p $RPM_BUILD_ROOT%{_libdir}/pkgconfig

make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README
%{_libdir}/libkvsns.so.*

%files devel
%defattr(-,root,root)
%{_libdir}/pkgconfig/libkvsns.pc
%{_includedir}/libzfswrap.h
%{_libdir}/libzfswrap.so
%{_libdir}/libzfswrap.a

%files utils
%defattr(-,root,root)
%{_bindir}/lzw_zfs
%{_bindir}/lzw_zpool


%changelog
* Mon Jul 25 2016 Philippe DENIEL <philippe.deniel@cea.fr> 0.9.1
- First alpha version
