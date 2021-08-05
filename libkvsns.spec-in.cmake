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
BuildRequires: cmake libini_config-devel
BuildRequires: gcc
Requires: libini_config
Provides: %{name} = %{version}-%{release}

# Remove implicit dep to libkvsns (which prevent from building libkvsns-utils
%global __requires_exclude ^libkvsns\\.so.*$

# Conditionally enable KVS and object stores
#
# 1. rpmbuild accepts these options (gpfs as example):
#    --without redis

%define on_off_switch() %%{?with_%1:ON}%%{!?with_%1:OFF}

# A few explanation about %bcond_with and %bcond_without
# /!\ be careful: this syntax can be quite messy
# %bcond_with means you add a "--with" option, default = without this feature
# %bcond_without adds a"--without" so the feature is enabled by default

@BCOND_RADOS@ rados
%global use_rados %{on_off_switch rados}

@BCOND_MOTR@ motr
%global use_motr %{on_off_switch motr}

@BCOND_REDIS@ redis
%global use_redis %{on_off_switch redis}

%description
The libkvsns is a library that allows of a POSIX namespace built on top of
a Key-Value Store.

%package devel
Summary: Development file for the library libkvsns
Group: Development/Libraries
Requires: %{name} = %{version}-%{release} pkgconfig
Provides: %{name}-devel = %{version}-%{release}

# REDIS
%if %{with redis}
%package redis
Summary: The REDIS based kvsal
Group: Applications/System
Requires: %{name} = %{version}-%{release} librados2
Provides: %{name}-rados = %{version}-%{release}
Requires: redis hiredis
BuildRequires: hiredis-devel

%description redis
This package contains a library for using REDIS as a KVS for libkvsns
%endif


# RADOS
%if %{with rados}
%package rados
Summary: The RADOS based backend for libkvsns
Group: Applications/System
Requires: %{name} = %{version}-%{release} librados2
Provides: %{name}-rados = %{version}-%{release}

%description rados
This package contains a library for using RADOS as a backed for libkvsns
%endif

# MOTR
%if %{with motr}
%package motr
Summary: The MOTR based backend for libkvsns
Group: Applications/System
Requires: %{name} = %{version}-%{release} cortx-motr
Provides: %{name}-motr = %{version}-%{release}

%description motr
This package contains libraries for using CORTX-MOTR as a backend for libkvsns
%endif


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
cmake . 

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
%if %{with redis}
install -m 644 kvsal/redis/libkvsal_redis.so %{buildroot}%{_libdir}
%endif

install -m 644 extstore/posix_store/libextstore_posix.so %{buildroot}%{_libdir}
install -m 644 extstore/crud_cache/libextstore_crud_cache.so %{buildroot}%{_libdir}
install -m 644 extstore/crud_cache/libobjstore_cmd.so %{buildroot}%{_libdir}
%if %{with rados gnutls}
install -m 644 extstore/rados/libextstore_rados.so %{buildroot}%{_libdir}
%endif
%if %{with motr}
install -m 644 extstore/motr/libextstore_motr.so %{buildroot}%{_libdir}
install -m 644 kvsal/motr/libkvsal_motr.so %{buildroot}%{_libdir}
install -m 644 motr/libm0common.so %{buildroot}%{_libdir}
%endif

install -m 644 kvsns/libkvsns.so %{buildroot}%{_libdir}
install -m 644 libkvsns.pc  %{buildroot}%{_libdir}/pkgconfig
install -m 755 kvsns_shell/kvsns_busybox %{buildroot}%{_bindir}
install -m 755 kvsns_shell/kvsns_cp %{buildroot}%{_bindir}
install -m 755 kvsns_hsm/kvsns_hsm %{buildroot}%{_bindir}
install -m 755 kvsns_script/kvsns_script %{buildroot}%{_bindir}
install -m 755 scripts/cp_put.sh %{buildroot}%{_bindir}
install -m 755 scripts/cp_get.sh %{buildroot}%{_bindir}
install -m 755 kvsns_attach/kvsns_attach %{buildroot}%{_bindir}
install -m 644 kvsns.ini %{buildroot}%{_sysconfdir}/kvsns.d

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_libdir}/libextstore_posix.so*
%{_libdir}/libextstore_crud_cache.so*
%{_libdir}/libobjstore_cmd.so*
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
%{_bindir}/kvsns_hsm
%{_bindir}/kvsns_script
%{_bindir}/cp_put.sh
%{_bindir}/cp_get.sh
 
%if %{with redis}
%files redis
%{_libdir}/libkvsal_redis.so*
%endif

%if %{with motr}
%files motr
%{_libdir}/libextstore_motr.so*
%{_libdir}/libkvsal_motr.so*
%{_libdir}/libm0common.so*
%endif

%if %{with rados}
%files rados
%{_libdir}/libextstore_rados.so*
%endif


%changelog
* Mon Jan  4 2021 Philippe DENIEL <philippe.deniel@cea.fr> 1.2.11
- Build libkvsns does not requires redis or hiredis, a special rpm is created

* Fri Dec 18 2020 Philippe DENIEL <philippe.deniel@cea.fr> 1.2.10
- Support for COTRX-MOTR as a backed for both kvsal and extstore

* Tue Aug  4 2020 Philippe DENIEL <philippe.deniel@cea.fr> 1.2.9
- Rename extstore_crud_cache_cmd and add objstore_cmd

* Mon Aug  3 2020 Philippe DENIEL <philippe.deniel@cea.fr> 1.2.8
- Change dependencies so that libkvsns-utils can be built

* Wed Jun 24 2020 Philippe DENIEL <philippe.deniel@cea.fr> 1.2.7
- Use dlopen and dlsym to manage kvsal

* Wed Jun 24 2020 Philippe DENIEL <philippe.deniel@cea.fr> 1.2.6
- Use dlopen and dlsym to manage extstore

* Tue Jun 23 2020 Philippe DENIEL <philippe.deniel@cea.fr> 1.2.5
- More modularity in crud_cache

* Tue Jun 16 2020 Philippe DENIEL <philippe.deniel@cea.fr> 1.2.4
- Add crud_cache feature for object stores that support only put/get/delete

* Tue Oct 24 2017 Philippe DENIEL <philippe.deniel@cea.fr> 1.2.3
- Support RADOS as an object store

* Tue Jun 27 2017 Philippe DENIEL <philippe.deniel@cea.fr> 1.2.2
- Modification of internal API

* Tue Jun 20 2017 Philippe DENIEL <philippe.deniel@cea.fr> 1.2.1
- Some bug fixed

* Tue Jun 13 2017 Philippe DENIEL <philippe.deniel@cea.fr> 1.2.0
- Add support for config file via libini_config

* Fri Jun  2 2017 Philippe DENIEL <philippe.deniel@cea.fr> 1.1.3
- Add kvsns_cp

* Fri Jun  2 2017 Philippe DENIEL <philippe.deniel@cea.fr> 1.1.2
- Add kvsns_attach feature

* Wed Nov 16 2016 Philippe DENIEL <philippe.deniel@cea.fr> 1.0.1
- Release candidate

* Mon Aug  1 2016 Philippe DENIEL <philippe.deniel@cea.fr> 0.9.2
- Source code tree refurbished

* Mon Jul 25 2016 Philippe DENIEL <philippe.deniel@cea.fr> 0.9.1
- First alpha version
