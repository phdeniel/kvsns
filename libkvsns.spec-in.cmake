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
BuildRequires: cmake hiredis-devel libini_config-devel
BuildRequires: gcc
Requires: redis hiredis libini_config
Provides: %{name} = %{version}-%{release}

# Conditionally enable KVS and object stores
#
# 1. rpmbuild accepts these options (gpfs as example):
#    --with mero
#    --without redis

%define on_off_switch() %%{?with_%1:ON}%%{!?with_%1:OFF}

# A few explanation about %bcond_with and %bcond_without
# /!\ be careful: this syntax can be quite messy
# %bcond_with means you add a "--with" option, default = without this feature
# %bcond_without adds a"--without" so the feature is enabled by default

@BCOND_KVS_REDIS@ kvs_redis
%global use_kvs_redis %{on_off_switch kvs_redis}

@BCOND_KVS_MERO@ kvs_mero
%global use_kvs_mero %{on_off_switch kvs_mero}

@BCOND_POSIX_STORE@ posix_store
%global use_posix_store %{on_off_switch posix_store}

@BCOND_POSIX_OBJ@ posix_obj
%global use_posix_obj %{on_off_switch posix_obj}

@BCOND_MERO_STORE@ mero_store
%global use_mero_store %{on_off_switch mero_store}

%description
The libkvsns is a library that allows of a POSIX namespace built on top of
a Key-Value Store.

%package devel
Summary: Development file for the library libkvsns
Group: Development/Libraries
Requires: %{name} = %{version}-%{release} pkgconfig
Requires: hiredis-devel
Provides: %{name}-devel = %{version}-%{release}


%description devel
The libkvsns is a library that allows of a POSIX namespace built on top of
a Key-Value Store.
This package contains tools for libkvsns.

%package utils
Summary: Development file for the library libkvsns
Group: Development/Libraries
Requires: %{name} = %{version}-%{release} pkgconfig
Requires: redis hiredis libkvsns
Provides: %{name}-utils = %{version}-%{release}

%description utils
The libkvsns is a library that allows of a POSIX namespace built on top of
a Key-Value Store.
This package contains the tools for libkvsns.

%prep
%setup -q -n %{sourcename}

%build
cmake . -DUSE_KVS_REDIS=%{use_kvs_redis}     \
	-DUSE_KVS_MERO=%{use_kvs_mero}       \
	-DUSE_POSIX_STORE=%{use_posix_store} \
	-DUSE_POSIX_OBJ=%{use_posix_obj}     \
	-DUSE_MERO_STORE=%{use_mero_store}

make %{?_smp_mflags} || make %{?_smp_mflags} || make

%install

mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_libdir}
mkdir -p %{buildroot}%{_libdir}/pkgconfig
mkdir -p %{buildroot}%{_includedir}/kvsns
mkdir -p %{buildroot}%{_sysconfdir}/kvsns.d
install -m 644 include/kvsns/kvsns.h  %{buildroot}%{_includedir}/kvsns
install -m 644 include/kvsns/kvsal.h  %{buildroot}%{_includedir}/kvsns
install -m 644 include/kvsns/extstore.h  %{buildroot}%{_includedir}/kvsns
install -m 644 kvsal/libkvsal.so %{buildroot}%{_libdir}
install -m 644 extstore/libextstore.so %{buildroot}%{_libdir}
install -m 644 kvsns/libkvsns.so %{buildroot}%{_libdir}
install -m 644 libkvsns.pc  %{buildroot}%{_libdir}/pkgconfig
install -m 755 kvsns_shell/kvsns_busybox %{buildroot}%{_bindir}
install -m 755 kvsns_shell/kvsns_cp %{buildroot}%{_bindir}
install -m 755 kvsns_attach/kvsns_attach %{buildroot}%{_bindir}
install -m 644 kvsns.ini %{buildroot}%{_sysconfdir}/kvsns.d

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_libdir}/libkvsal.so*
%{_libdir}/libextstore.so*
%{_libdir}/libkvsns.so*
%config(noreplace) %{_sysconfdir}/kvsns.d/kvsns.ini

%files devel
%defattr(-,root,root)
%{_libdir}/pkgconfig/libkvsns.pc
%{_includedir}/kvsns/kvsns.h
%{_includedir}/kvsns/kvsal.h
%{_includedir}/kvsns/extstore.h

%files utils
%defattr(-,root,root)
%{_bindir}/kvsns_busybox
%{_bindir}/kvsns_cp
%{_bindir}/kvsns_attach

%changelog
* Thu Jun 13 2017 Philippe DENIEL <philippe.deniel@cea.fr> 1.2.0
- Add support for config file via libini_config

* Fri Jun  2 2017 Philippe DENIEL <philippe.deniel@cea.fr> 1.1.3
- Add kvsns_cp

* Fri Jun  2 2017 Philippe DENIEL <philippe.deniel@cea.fr> 1.1.2
- Add kvsns_attach feature

* Wed Mar  1 2017 Philippe DENIEL <philippe.deniel@cea.fr> 1.1.1
- API refurbished after mero injection

* Wed Nov 16 2016 Philippe DENIEL <philippe.deniel@cea.fr> 1.0.1
- Release candidate

* Mon Aug  1 2016 Philippe DENIEL <philippe.deniel@cea.fr> 0.9.2
- Source code tree refurbished

* Mon Jul 25 2016 Philippe DENIEL <philippe.deniel@cea.fr> 0.9.1
- First alpha version
