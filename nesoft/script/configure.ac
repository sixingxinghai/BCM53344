##
# #Configure template file for PacOS.
## autoconf will generate configure script.
##
##  Copyright (c) 2001, 2002, 2003 
##
AC_PREREQ(2.53)

AC_INIT(PacOS, 1.0)
AC_CONFIG_SRCDIR(../src/lib/lib.h)
AM_INIT_AUTOMAKE

dnl -----------------------------------
dnl Get hostname and other information.
dnl -----------------------------------
AC_CANONICAL_HOST

dnl ------------
dnl Check CFLAGS
dnl ------------
if test -n "$CFLAGS" ; then
   cflags_specified=yes
else
   CFLAGS="-g -O"
fi

dnl --------
dnl Check CC
dnl --------
AC_PROG_CC
# Check and see if these compiler options are supported....
if test "x$GCC" = "xyes" ; then
   echo "main() {}" >conftest.c
   # Check -Wno-pointer-sign
   AC_MSG_CHECKING([if gcc accepts -Wno-pointer-sign option])
   if AC_TRY_COMMAND($CC -Wno-pointer-sign conftest.c) > /dev/null 2>&1 ; then
      nopointersign=yes
      AC_MSG_RESULT([yes])
   else
        nopointersign=no
        AC_MSG_RESULT([no])
   fi
   # Check -fno-working-directory
   AC_MSG_CHECKING([if gcc acceps -fno-working-directory option])
   if AC_TRY_COMMAND($CC -fno-working-directory conftest.c) > /dev/null 2>&1 ; then
      noworkingdirectory=yes
      AC_MSG_RESULT([yes])
   else
        noworkingdirectory=no
        AC_MSG_RESULT([no])
   fi
fi


dnl ------------------------------------------
dnl If CLFAGS doesn\'t exist set default value
dnl ------------------------------------------
if test "x$cflags_specified" = "x"
then
CFLAGS="$CFLAGS"
fi

dnl --------------
dnl Check programs
dnl --------------
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(RANLIB, ranlib, :)

dnl ---------
dnl AIX check
dnl ---------
AC_AIX

dnl ----------------------
dnl Packages configuration
dnl ----------------------
AC_ARG_ENABLE(imi,
[  --enable-imi            Enable Integrated Management Interface (IMI)])
AC_ARG_ENABLE(wmi,
[  --enable-wmi            Enable Web Management Interface (WMI)])
AC_ARG_ENABLE(imish,
[  --enable-imish          Enable IMI and IMI shell (IMI+IMISH)])
AC_ARG_ENABLE(basic-access,
[  --enable-basic-access   Enable Basic Access Services(DNS, DHCP, NAT, ACL)])
AC_ARG_ENABLE(sysconf,
[  --enable-sysconf        Enable Basic Access Services(DNS, DHCP, NAT, ACL)])
AC_ARG_ENABLE(imi-sysconfig,
[  --disable-imi-sysconfig Read system configurations on startup])
AC_ARG_ENABLE(dns-client,
[  --disable-dns-client    Disable DNS Client for Basic Access])
AC_ARG_ENABLE(dhcp-client,
[  --disable-dhcp-client   Disable DHCP Client for Basic Access])
AC_ARG_ENABLE(dhcp-server,
[  --disable-dhcp-server   Disable DHCP Server for Basic Access])
AC_ARG_ENABLE(dhcp-update-style,
[  --disable-dhcp-update-style   Disable DHCP Server update for Basic Access])
AC_ARG_ENABLE(pppoe-client,
[  --disable-pppoe-client  Disable PPPoE Client for Basic Access])
AC_ARG_ENABLE(acl,
[  --disable-nat           Disable NAT for Basic Access])
AC_ARG_ENABLE(ntp,
[  --disable-acl           Disable ACL for Basic Access])
AC_ARG_ENABLE(nat,
[  --disable-ntp           Disable Network Time Protocol for Basic Access])
AC_ARG_ENABLE(hostname-change,
[  --enable-hostname-change Change system hostname by hostname command])
AC_ARG_ENABLE(ipv6,
[  --disable-ipv6          turn off IPv6 related features and daemons])
AC_ARG_ENABLE(rtadv,
[  --disable-rtadv         disable Router Advertisement feature])
AC_ARG_ENABLE(mip6,
[  --disable-mip6          disable Mobile IPv6 feature])
AC_ARG_ENABLE(ipv4-tunnel,
[  --enable-ipv4-tunnel    Enable IPv4 tunneling])
AC_ARG_ENABLE(ipv6-transit,
[  --enable-ipv6-transit   Enable IPv6 transition])
AC_ARG_ENABLE(nsm,
[  --disable-nsm           do not build nsm daemon])
AC_ARG_ENABLE(if-unnumbered,
[  --enable-if-unnumbered  Enable NSM IPv4 and IPv6 Unnumbered Interface])
AC_ARG_ENABLE(if-arbiter,
[  --enable-if-arbiter     Enable NSM Interface Arbiter])
AC_ARG_ENABLE(bgpd,
[  --disable-bgpd          do not build bgpd])
AC_ARG_ENABLE(order-nexthop,
[ --disable-order-nexthop	disable order-nextop])
AC_ARG_ENABLE(bgpd-extasn,
[  --disable-bgpd-extasn  Disable bgp extended-asn capability])
AC_ARG_ENABLE(onmd,
[  --enable-onmd          Enable onmd])
AC_ARG_ENABLE(cfm,
[  --enable-cfm           Enable cfm])
AC_ARG_ENABLE(cfm,
[  --enable-cfm-Y1731     Enable cfm Y1731])
AC_ARG_ENABLE(lldp,
[  --enable-lldp          Enable lldp])
AC_ARG_ENABLE(efm,
[  --enable-efm           Enable efm])
AC_ARG_ENABLE(ripd,
[  --disable-ripd          do not build ripd])
AC_ARG_ENABLE(ripngd,
[  --disable-ripngd        do not build ripngd])
AC_ARG_ENABLE(ospfd,
[  --disable-ospfd         do not build ospfd])
AC_ARG_ENABLE(ospf6d,
[  --enable-ospf6d         build ospf6d])
AC_ARG_ENABLE(bfd,
[  --disable-bfd            build bfd])
AC_ARG_ENABLE(bgp-rt2fib,
[  --disable-bgp-rt2fib    do not install BGP routes in FIB])
AC_ARG_ENABLE(netlink,
[  --enable-netlink        force to use Linux netlink interface])
AC_ARG_ENABLE(mpls,
[  --enable-mpls           enable mpls functionality])
AC_ARG_ENABLE(pbr,
[  --enable-pbr           enable pbr support])
AC_ARG_ENABLE(mpls-oam,
[  --enable-mpls-oam       enable mpls Operations and Management features])
AC_ARG_ENABLE(mpls-bk-lsp,
[  --enable-mpls-bk-lsp    enable mpls-install-back-lsp functionality])
AC_ARG_ENABLE(6pe,
[  --enable-6pe            enable 6pe functionality])
AC_ARG_ENABLE(ldpd,
[  --disable-ldpd          do not build ldpd])
AC_ARG_ENABLE(ldp-inter-area,
[  --enable-ldp-inter-area    do not build with ldp-inter-area])
AC_ARG_ENABLE(ldp-suppress-fec,
[  --enable-ldp-suppress-fec    enable ldp-suppress-fec feature])
AC_ARG_ENABLE(rsvpd,
[  --disable-rsvpd         do not build rsvpd])
AC_ARG_ENABLE(isisd,
[  --disable-isisd         do not build isisd])
AC_ARG_ENABLE(pdmd,
[  --disable-pdmd          do not build pdmd (PIM-DM)])
AC_ARG_ENABLE(pimd,
[  --disable-pimd          do not build pimd])
AC_ARG_ENABLE(pim-ssm,
[  --disable-pim-ssm       do not build pim-ssm])
AC_ARG_ENABLE(pimpktgend,
[  --enable-pimpktgend     enable PIM packet generator])
AC_ARG_ENABLE(dvmrpd,
[  --disable-dvmrpd        do not build dvmrpd])
AC_ARG_ENABLE(dvmrp-ssm,
[  --disable-dvmrp-ssm     do not build dvmrp-ssm])
AC_ARG_ENABLE(pim6d,
[  --disable-pim6d         do not build pim6d])
AC_ARG_ENABLE(pim6-ssm,
[  --disable-pim6-ssm      do not build pim6-ssm])
AC_ARG_ENABLE(mcast4-intf-limit,
[  --disable-mcast4-intf-limit      disable IPv4 multicast interface limit check])
AC_ARG_ENABLE(mcast4-intf-limit-ignore,
[  --enable-mcast4-intf-limit-ignore      ignore IPv4 multicast interface limit check for Control Plane verification])
AC_ARG_ENABLE(mcast6-intf-limit,
[  --disable-mcast6-intf-limit      disable IPv6 multicast interface limit check])
AC_ARG_ENABLE(mcast6-intf-limit-ignore,
[  --enable-mcast6-intf-limit-ignore      ignore IPv6 multicast interface limit check for Control Plane verification])
AC_ARG_ENABLE(stpd,
[  --enable-stpd           build stpd])
AC_ARG_ENABLE(rstpd,
[  --enable-rstpd          build rstpd])
AC_ARG_ENABLE(authd,
[  --enable-authd          build authd])
AC_ARG_ENABLE(mstpd,
[  --enable-mstpd          build mstpd])
AC_ARG_ENABLE(intervlan-routing,
[  --enable-intervlan-routing           build inter VLAN routing])
AC_ARG_ENABLE(vlan,
[  --enable-vlan           build vlan])
AC_ARG_ENABLE(pvlan,
[  --enable-pvlan          build private vlan])
AC_ARG_ENABLE(rmond,
[  --enable-rmond          build rmond])
AC_ARG_ENABLE(ipsec,
[  --enable-ipsec          build ipsec])
AC_ARG_ENABLE(firewall,
[  --enable-firewall       build firewall])
AC_ARG_ENABLE(vlan-class,
[  --enable-vlan-class     build vlan classifier])
AC_ARG_ENABLE(vlan-stack,
[  --enable-vlan-stack     build vlan stacking (IEEE 802.1Q encapulation)])
AC_ARG_ENABLE(rate-limit,
[  --enable-rate-limit     build layer2 rate limiting])
AC_ARG_ENABLE(l2swfwdr,
[  --enable-swfwdr         build layer2 software forwarder ])
AC_ARG_ENABLE(storage-dev,
[  --disable-storage-dev   disable Storage Device support])
AC_ARG_ENABLE(hal,
[  --enable-hal            build HAL])
AC_ARG_ENABLE(axelx,
[ --enable-axelx           build Axelx Hardware Services Layer(HSL)])
AC_ARG_ENABLE(user-hsl,
[ --enable-user-hsl        build User Space Hardware Services Layer(HSL)])
AC_ARG_ENABLE(broadcom,
[  --enable-broadcom=@<:@bmw|gto@:>@       build Broadcom Hardware Services Layer(HSL)])
AC_ARG_ENABLE(marvell,
[  --enable-marvell        build Marvell Hardware Services Layer(HSL)])
AC_ARG_ENABLE(marvell-ls,
[  --enable-marvell-ls     build Marvell LS Hardware Services Layer(HSL)])
AC_ARG_ENABLE(intel,
[  --enable-intel=ARG      build Intel NP Hardware Services Layer(HSL). ARG is CP-PDK path])
AC_ARG_ENABLE(igmp_snoop,
[  --enable-igmp-snoop     enable IGMP snooping])
AC_ARG_ENABLE(mld_snoop,
[  --enable-mld-snoop     enable MLD snooping])
AC_ARG_ENABLE(mld_default_no_snoop,
[  --enable-mld-default-no-snoop     enable default MLD snooping])
AC_ARG_ENABLE(igmp_v3,
[  --disable-igmp-v3       disable IGMPv3 ])
AC_ARG_ENABLE(default_bridge,
[  --enable-default-bridge   enable default bridge])
AC_ARG_ENABLE(provider_bridge,
[  --enable-provider-bridge  enable provider bridge])
AC_ARG_ENABLE(i_pbb,
[  --enable-i-pbb  enable i pbb])
AC_ARG_ENABLE(b_pbb,
[  --enable-b-pbb  enable b pbb])
AC_ARG_ENABLE(pbb_te,
[  --enable-pbb-te  enable pbb te])
AC_ARG_ENABLE(g8031,
[ --disable-g8031	disable ethernet linear protection])
AC_ARG_ENABLE(g8032,
[ --disable-g8032	disable ethernet ring protection])
AC_ARG_ENABLE(gmrp,
[  --enable-gmrp           enable gmrp])
AC_ARG_ENABLE(mmrp,
[  --enable-mmrp           enable mmrp])
AC_ARG_ENABLE(gvrp,
[  --enable-gvrp           enable gvrp])
AC_ARG_ENABLE(mvrp,
[  --enable-mvrp           enable mvrp])
AC_ARG_ENABLE(te,
[  --enable-te             enable Traffic Engineering feature])
AC_ARG_ENABLE(gmpls,
[  --enable-gmpls          enable GMPLS feature])
AC_ARG_ENABLE(broken-aliases,
[  --enable-broken-aliases enable aliases as distinct interfaces for Linux 2.2.X])
AC_ARG_ENABLE(snmp,
[  --disable-snmp          disable SNMP support])
AC_ARG_ENABLE(agentx,
[  --enable-agentx         enable AGENTX feature])
AC_ARG_ENABLE(mac_auth,
[  --enable-mac-auth       enable MAC AUTH feature])
AC_ARG_WITH(libpam,
[  --with-libpam           use libpam for PAM support in vtysh])
AC_ARG_WITH(ipnet2,
[  --with-ipnet2           Use Interpeak IPNet TCP/IP stack])
AC_ARG_WITH(vxw54,
[  --with-vxw54           Use TCP/IP stack for VXWorks54])
AC_ARG_WITH(pne22,
[  --with-pne22            Use PNE2.2 TCP/IP stack for VxWorks])
AC_ARG_WITH(pne33,
[ --with-pne33             Use PNE3.3 TCP/IP stack for vxWorks])
AC_ARG_WITH(pne37,
[ --with-pne37		   Use PNE3.7 TCP/IP stack for vxWorks])
AC_ARG_ENABLE(tcpsock,
[  --enable-tcp-message    enable TCP/IP socket connection between daemons])
AC_ARG_ENABLE(rip-ecmp,
[  --disable-rip-ecmp      disable RIP ECMP option])
AC_ARG_ENABLE(nssa,
[  --disable-nssa          disable OSPF NSSA option])
AC_ARG_ENABLE(ospf-db-overflow,
[  --disable-ospf-db-overflow   disable OSPF database overflow option (RFC1765)
])
AC_ARG_ENABLE(ospf-multi-area,
[  --enable-ospf-multi-area  enable Support of Multiple Areas on single interface for OSPF])
AC_ARG_ENABLE(ospf-multi-inst,
[  --enable-ospf-multi-inst  enable Support of Multiple Instance for OSPF])
AC_ARG_ENABLE(bfd-line,
[  --enable-bfd-line      enable BFD Distribution option])
AC_ARG_ENABLE(bfd-main,
[  --enable-bfd-main      enable BFD Distribution option])
AC_ARG_ENABLE(bfd-mono,
[  --disable-bfd-mono       disable BFD Monolithic option])
AC_ARG_ENABLE(opaque-lsa,
[  --disable-opaque-lsa    disable OSPF Opaque-LSA support (RFC2370)])
AC_ARG_ENABLE(multi-topology,
[  --enable-multi-topology enable IS-IS Multi topology feature])
AC_ARG_ENABLE(mpls-fwd,
[  --enable-mpls-fwd       enable MPLS Forwarder support])
AC_ARG_ENABLE(vrrp,
[  --enable-vrrp           enable VRRP])
AC_ARG_ENABLE(vrrp-link-addr,
[  --enable-vrrp-link-addr enable VRRP (virtual IP is connected address)])
AC_ARG_ENABLE(rmm,
[  --enable-rmm            enable Redundancy Management Module])
AC_ARG_ENABLE(rmm-testprimary,
[ --enable-rmm-testprimary enable Linux RMM test default primary])
AC_ARG_ENABLE(rmm-testbackup,
[ --enable-rmm-testbackup  enable Linux RMM test default backup])
AC_ARG_ENABLE(vrf,
[  --enable-vrf            enable VPN Routing Forwarding feature])
AC_ARG_ENABLE(pece-ospf,
[  --enable-pece-ospf      enable draft-ishiguro-ppvpn-pe-ce-ospf])
AC_ARG_ENABLE(pece-rip,
[  --enable-pece-rip       enable RIP as the CE/PE protocol to support VPN])
AC_ARG_ENABLE(pece-ospf6,
[  --enable-pece-ospf6     enable OSPFv3 as the CE/PE protocol to support VPN])
AC_ARG_ENABLE(pece-ripng,
[  --enable-pece-ripng     enable RIPNG as the CE/PE protocol to support VPN])
AC_ARG_ENABLE(cr-ldp,
[  --enable-cr-ldp         enable CR-LDP support])
AC_ARG_ENABLE(vr,
[  --enable-vr             enable virtual routing])
AC_ARG_ENABLE(vrx,
[  --enable-vrx            enable virtual routing for CheckPoint VSX])
AC_ARG_ENABLE(multiple-fib,
[  --enable-multiple-fib   enable Multiple FIB kernel support])
AC_ARG_ENABLE(crx,
[  --enable-crx            enable PacOS Clustering solution])
AC_ARG_ENABLE(all-in-one,
[  --enable-all-in-one     make all program linked verion])
AC_ARG_ENABLE(ospf-cspf,
[  --enable-ospf-cspf      enable CSPF feature for OSPF])
AC_ARG_ENABLE(ospf6-cspf,
[  --enable-ospf6-cspf      enable CSPF feature for OSPF])
AC_ARG_ENABLE(isis-cspf,
[  --enable-isis-cspf      enable CSPF feature for ISIS])
AC_ARG_ENABLE(static,
[  --enable-static         enable static library linking feature])
AC_ARG_ENABLE(memmgr,
[  --disable-memmgr        disable memory manager feature])
AC_ARG_ENABLE(splat,
[  --enable-splat       enable secure platform feature])
AC_ARG_ENABLE(splat-test,
[  --enable-splat-test     enable secure platform testing])
AC_ARG_ENABLE(mcast-ipv4,
[  --enable-mcast-ipv4     enable IPv4 MRIB in NSM])
AC_ARG_ENABLE(mcast-ipv6,
[  --enable-mcast-ipv6     enable IPv6 MRIB in NSM])
AC_ARG_ENABLE(licmgr,
[  --enable-licmgr         enable licmgr manager feature])
AC_ARG_ENABLE(dev-test,
[  --enable-dev-test       enable Development Testing mode])
AC_ARG_ENABLE(efence,
[  --enable-efence         Link all exes with Efence Library except IMI/IMISH])
AC_ARG_ENABLE(mpls-vc,
[  --enable-mpls-vc        enable MPLS based virtual circuit feature])
AC_ARG_ENABLE(vccv,
[  --enable-vccv           enable Virtual Circuit Connectivity Verification feature])
AC_ARG_ENABLE(ms-pw,
[  --enable-ms-pw          enable Multi-Segment Psuedowire feature])
AC_ARG_ENABLE(fec-129,
[  --enable-fec129         enable LDP FEC 129 for MPLS virtual circuits])
AC_ARG_ENABLE(vpls,
[  --enable-vpls           enable MPLS Virtual Private Lan Service feature])
AC_ARG_ENABLE(restart,
[  --enable-restart        enable Graceful and Hitless Restart feature])
AC_ARG_ENABLE(rsvp-graceful,
[  --enable-rsvp-graceful  enable RSVP Graceful Restart feature])
AC_ARG_ENABLE(diffserv,
[  --enable-diffserv,      enable MPLS Diffserv support])
AC_ARG_ENABLE(dste,
[  --enable-dste,          enable MPLS Diffserv TE support])
AC_ARG_ENABLE(sun-cc,
[  --enable-sun-cc         Use Sun CC for compilation])
AC_ARG_ENABLE(sun-cc-64-bit,
[  --enable-sun-cc-64-bit  Use Sun CC 64-bit for compilation])
AC_ARG_ENABLE(bgp-dump,
[  --enable-bgp-dump       enable BGP dump feature])
AC_ARG_ENABLE(mpls-frr,
[  --enable-mpls-frr       enable MPLS Fast Reroute feature])
AC_ARG_ENABLE(custom1,
[  --enable-custom1       enable CUSTOM1 feature])
AC_ARG_ENABLE(bigendian,
[ --enable-bigendian      enable Big Endian support])
AC_ARG_ENABLE(qos,
[ --enable-qos          enable QOS support])
AC_ARG_ENABLE(l2lern,
[ --enable-l2lern          enable L2LERN support])
AC_ARG_ENABLE(hwstack,
[ --enable-hwstack      enable hardware stacking])
AC_ARG_ENABLE(multi-conf-ses,
[ --enable-multi-conf-ses  enable multiple configuration sessions])
AC_ARG_ENABLE(config_compare,
[  --enable-config-compare  enable CONFIG_COMPARE feature])
AC_ARG_ENABLE(enea_element,
[  --enable-enea-element   enable support from ENEA Element checkpointing])
AC_ARG_ENABLE(open_saf,
[  --enable-open-saf   enable support from OpenSAF checkpointing])
AC_ARG_ENABLE(ha,
[  --enable-ha             enable HA support])
AC_ARG_ENABLE(pacos_debug,
[  --enable-pacos-debug    enable global PACOS_DEBUG macro for the developer build])
AC_ARG_ENABLE(fm_sim,
[  --enable-fm-sim         enable FM simulation support])
AC_ARG_ENABLE(smi,
[  --enable-smi            enable Simple Management Interface (SMI)])
AC_ARG_ENABLE(rpvst-plus,
[  --enable-rpvst-plus     enable Rapid Pervlan Spanning Tree (RPVST_PLUS)])
AC_ARG_ENABLE(vlogd,
[  --enable-vlogd     enable VR context sensitive logging])
AC_ARG_ENABLE(dcb,
[  --enable-dcb     enable DCB feature])
AC_ARG_ENABLE(elmid,
[  --enable-elmid     enable elmi])
AC_ARG_ENABLE(lmpd,
[  --enable-lmpd     enable LMP support])
AC_ARG_ENABLE(packet,
[  --enable-packet   enable MPLS GMPLS PACKET support])
AC_ARG_ENABLE(gels-edge,
[  --enable-gels-edge   enable GMPLS PBB-TE support for edge nodes])
AC_ARG_ENABLE(gels-core,
[  --enable-gels-core   enable GMPLS PBB-TE support for core nodes])
AC_ARG_ENABLE(agentx-unix-domain,
[  --enable-agentx-unix-domain      enable Unix domain socket for Agentx])

dnl Some systems (Solaris 2.x) require libnsl (Network Services Library)
case "$host" in
  *-sunos5.6* | *-solaris2.6*)
      opsys=solaris
      OPSYSDIR=../platform/solaris
      AC_DEFINE(SUNOS_5,,Solaris)
      AC_CHECK_LIB(xnet, main)
      CURSES=-lcurses
      AC_DEFINE(IF_BANDWIDTH_INFO,,Interface Bandwidth Information)
  ;;
  *-sunos5* | *-solaris2*)
      opsys=solaris
      OPSYSDIR=../platform/solaris
      AC_DEFINE(SUNOS_5,,Solaris)
      AC_CHECK_LIB(socket, main)
      AC_CHECK_LIB(nsl, main)
      CURSES=-lcurses
      AC_DEFINE(IF_BANDWIDTH_INFO,,Interface Bandwidth Information)
  ;;
  *-linux-*)
      opsys=gnu-linux
      OPSYSDIR=../platform/linux
      AC_DEFINE(GNU_LINUX,,GNU/Linux)
      AC_DEFINE(HAVE_MULTIPATH,,Multipath)
  dnl -ldl lib required for static linking of -lcrypto
      LDL=-ldl
  dnl   MPLS="mpls"
  dnl   LDPD="ldpd"
  ;;
  *-nec-sysv4*)
      AC_CHECK_LIB(nsl, gethostbyname)
      AC_CHECK_LIB(socket, socket)
  ;;
  *freebsd*)
      OPSYSDIR=../platform/freebsd
      AC_DEFINE(FREEBSD_32,,FreeBSD 3.2)
  dnl -ldl lib required for static linking of -lcrypto
      dnlLDL=-ldl
  ;;
  *-openbsd*)
      opsys=openbsd
      OPSYSDIR=../platform/openbsd
      AC_DEFINE(OPEN_BSD,,OpenBSD)
  ;;
  *netbsd*)
      opsys=netbsd
      OPSYSDIR=../platform/netbsd
      AC_DEFINE(NET_BSD,,NetBSD)
      OTHER_METHOD="mtu_kvm.o"
      AC_CHECK_LIB(kvm, main)
  ;;
  *-vxworks)
      if test "$with_ipnet2" = "yes";then
        OPSYSDIR=../platform/vxworks_ipnet2
      else
        OPSYSDIR=../platform/vxworks
      fi
      if test "$with_pne22" = "yes";then
        AC_DEFINE(PNE_VERSION_2_2,,PNE22)
        pacos_cv_pne22=yes
      fi
      if test "$with_vxw54" = "yes";then
        AC_DEFINE(VXW_VERSION_5_4,,VXW54)
      fi
      if test "$with_pne33" = "yes" ; then
         AC_DEFINE(PNE_VERSION_3_3,,PNE33)
         pacos_cv_pne33=yes
      fi
      if test "$with_pne37" = "yes" ; then
         AC_DEFINE(PNE_VERSION_3_7,,PNE37)
         pacos_cv_pne37=yes
      fi


  ;;
esac

case "$target" in
  *-sunos5.6* | *-solaris2.6*)
      opsys=solaris
      OPSYSDIR=../platform/solaris
      AC_DEFINE(SUNOS_5,,Solaris)
      AC_CHECK_LIB(xnet, main)
      CURSES=-lcurses
      AC_DEFINE(IF_BANDWIDTH_INFO,,Interface Bandwidth Information)
  ;;
  *-sunos5* | *-solaris2*)
      opsys=solaris
      OPSYSDIR=../platform/solaris
      AC_DEFINE(SUNOS_5,,Solaris)
      AC_CHECK_LIB(socket, main)
      AC_CHECK_LIB(nsl, main)
      CURSES=-lcurses
      AC_DEFINE(IF_BANDWIDTH_INFO,,Interface Bandwidth Information)
  ;;
  *-linux-*)
      opsys=gnu-linux
      OPSYSDIR=../platform/linux
      AC_DEFINE(GNU_LINUX,,GNU/Linux)
      AC_DEFINE(HAVE_MULTIPATH,,Multipath)
  dnl -ldl lib required for static linking of -lcrypto
      LDL=-ldl
  dnl   MPLS="mpls"
  dnl   LDPD="ldpd"
  ;;
  *-nec-sysv4*)
      AC_CHECK_LIB(nsl, gethostbyname)
      AC_CHECK_LIB(socket, socket)
  ;;
  *freebsd*)
      OPSYSDIR=../platform/freebsd
      AC_DEFINE(FREEBSD_32,,FreeBSD 3.2)
  dnl -ldl lib required for static linking of -lcrypto
      dnlLDL=-ldl
  ;;
  *-openbsd*)
      opsys=openbsd
      OPSYSDIR=../platform/openbsd
      AC_DEFINE(OPEN_BSD,,OpenBSD)
  ;;
  *netbsd*)
      opsys=netbsd
      OPSYSDIR=../platform/netbsd
      AC_DEFINE(NET_BSD,,NetBSD)
      OTHER_METHOD="mtu_kvm.o"
      AC_CHECK_LIB(kvm, main)
  ;;
  *-vxworks)
      if test "$with_ipnet2" = "yes";then
        OPSYSDIR=../platform/vxworks_ipnet2
      else
        OPSYSDIR=../platform/vxworks
      fi
      if test "$with_pne22" = "yes";then
        AC_DEFINE(PNE_VERSION_2_2,,PNE22)
        pacos_cv_pne22=yes
      fi
      if test "$with_vxw54" = "yes";then
        AC_DEFINE(VXW_VERSION_5_4,,VXW54)
      fi
      if test "$with_pne33" = "yes" ; then
        AC_DEFINE(PNE_VERSION_3_3,,PNE33)
      fi
      if test "$with_pne37" = "yes" ; then
        AC_DEFINE(PNE_VERSION_3_7,,PNE37)
      fi

  ;;
esac

case "${IAPN_PACOS_PLATFORM}" in
  *solaris*)
      OPSYSDIR=../platform/solaris
  ;;
  *linux*)
      OPSYSDIR=../platform/linux
  ;;
  *freebsd*)
      OPSYSDIR=../platform/freebsd
  ;;
  *openbsd*)
      OPSYSDIR=../platform/openbsd
  ;;
  *netbsd*)
      OPSYSDIR=../platform/netbsd
  ;;
  *dummy*)
      OPSYSDIR=../platform/dummy
  ;;
esac

AC_DEFINE_UNQUOTED(HOST_NAME, "$host",Host name)
AC_CONFIG_HEADER($OPSYSDIR/config.h:config.h.in)

dnl ------------------
dnl Rules.options file
dnl ------------------
OPTIONS_FILE=$OPSYSDIR/Rules.options
>$OPTIONS_FILE

dnl --------------------------------------------------
dnl Default option settings.
dnl If set to "yes" use disable--option to disable it.
dnl If set to "no" use enable-option to enable it.
dnl --------------------------------------------------

ENABLE_IPV6=yes
ENABLE_RTADV=yes
ENABLE_MIP6=yes
ENABLE_TUNNEL=no
ENABLE_NSM=yes
ENABLE_NSM_IF_UNNUMBERED=no
ENABLE_NSM_IF_ARBITER=no
ENABLE_BGPD=yes
ENABLE_BGPD_EXTASN=yes
ENABLE_ONMD=no
ENABLE_CFM=no
ENABLE_G8031=no
ENABLE_G8032=no
ENABLE_LLDP=no
ENABLE_EFM=no
ENABLE_ELMID=no
ENABLE_RIPD=yes
ENABLE_RIPNGD=yes
ENABLE_OSPFD=yes
ENABLE_OSPF6D=yes
ENABLE_BFD=yes
ENABLE_BFD_MONO=yes
ENABLE_BFD_MAIN=no
ENABLE_BFD_LINE=no
ENABLE_BGP_RT2FIB=yes
ENABLE_NETLINK=no
ENABLE_MPLS=no
ENABLE_MPLS_OAM=no
ENABLE_PBR=no
ENABLE_LDPD=yes
ENABLE_LDP_INTER_AREA=no
ENABLE_LDP_SUPPRESS_FEC=no
ENABLE_RSVPD=yes
ENABLE_ISISD=yes
ENABLE_PDMD=no
ENABLE_PIMD=yes
ENABLE_PIM6D=yes
ENABLE_PIMPKTGEND=no
ENABLE_DVMRPD=yes
ENABLE_MCAST_IPV4=no
ENABLE_MCAST_IPV6=no
ENABLE_TE=no
ENABLE_GMPLS=no
ENABLE_BROKEN_ALIASES=no
ENABLE_SPLAT=no
ENABLE_SNMP=yes
ENABLE_AGENTX=no
WITH_LIBPAM=no
ENABLE_TCP_MESSAGE=no
ENABLE_NSSA=yes
ENABLE_RIP_ECMP=yes
ENABLE_L3_NPF=no
ENABLE_OSPF_DB_OVERFLOW=yes
ENABLE_OSPF_MULTI_AREA=no
ENABLE_OSPF_MULTI_INST=no
ENABLE_OPAQUE_LSA=yes
ENABLE_OSPF_TE=no
ENABLE_OSPF6_TE=no
ENABLE_MULTI_TOPOLOGY=no
ENABLE_MPLS_FWD=no
ENABLE_VRRP=no
ENABLE_RMM=no
ENABLE_RMM_TESTPRIMARY=no
ENABLE_RMM_TESTBACKUP=no
ENABLE_VRF=no
ENABLE_CR_LDP=no
ENABLE_VR=no
ENABLE_AIO=no
ENABLE_CSPF=no
ENABLE_OSPF_CSPF=no
ENABLE_OSPF6_CSPF=no
ENABLE_ISIS_CSPF=no
ENABLE_MEMMGR=yes
ENABLE_MEMLIMIT=no
ENABLE_MEMACCT=no
ENABLE_MEMACCT_DETAIL=no
ENABLE_MEMCHK=no
ENABLE_MEMLOG=no
ENABLE_MPLS_VC=no
ENABLE_VCCV=no
ENABLE_MS_PW=no
ENABLE_FEC129=no
ENABLE_VPLS=no
ENABLE_MPLS_TDM_VC=no
ENABLE_MPLS_ATM_VC=no
ENABLE_RESTART=no
ENABLE_RSVP_GRACEFUL=no
ENABLE_SUN_CC=no
ENABLE_SUN_CC_64_BIT=no
ENABLE_DEV_TEST=no
ENABLE_LICMGR=no
ENABLE_STATIC=no
ENABLE_DIFFSERV=no
ENABLE_DSTE=no
ENABLE_BASIC_ACCESS=no
ENABLE_DNS_CLIENT=no
ENABLE_DHCP_CLIENT=no
ENABLE_DHCP_SERVER=no
ENABLE_PPPOE_CLIENT=no
ENABLE_NAT=no
ENABLE_ACL=no
ENABLE_NTP=no
ENABLE_CRX=no
ENABLE_RSTPD=no
ENABLE_AUTHD=no
ENABLE_STPD=no
ENABLE_LACPD=no
ENABLE_RMOND=no
ENABLE_VLAN=no
ENABLE_PVLAN=no
ENABLE_IPSEC=no
ENABLE_FIREWALL=no
ENABLE_VLAN_CLASS=no
ENABLE_VLAN_STACK=no
ENABLE_RATE_LIMIT=no
ENABLE_MSTPD=no
ENABLE_RPVST_PLUS=no
ENABLE_SWFWDR=no
ENABLE_IGMP_SNOOP=no
ENABLE_MLD_SNOOP=no
DISABLE_DEFAULT_MLD_SNOOP=no
ENABLE_DCB=no
ENABLE_GMRP=no
ENABLE_MMRP=no
ENABLE_GVRP=no
ENABLE_MVRP=no
ENABLE_MPLS_FRR=no
ENABLE_L2=no
ENABLE_L3=no
ENABLE_CUSTOM1=no
ENABLE_HAL=no
ENABLE_BROADCOM=no
ENABLE_MARVELL=no
ENABLE_INTEL=no
ENABLE_STORAGE_DEV=yes
ENABLE_IPNET=no
ENABLE_QOS=no
ENABLE_L2LERN=no
ENABLE_MAC_AUTH=no
ENABLE_HWSTACK=no
ENABLE_BIGENDIAN=no
ENABLE_MULTI_CONF_SES=no
ENABLE_CONFIG_COMPARE=no
ENABLE_ENEA_ELEMENT=no
ENABLE_OPEN_SAF=no
ENABLE_HA=no
ENABLE_PACOS_DEBUG=no
ENABLE_FM_SIM=no
ENABLE_SMI=no
ENABLE_AXELX=no
ENABLE_USER_HSL=no
ENABLE_MARVELL_LS=no
ENABLE_6PE=no
ENABLE_VLOGD=no
ENABLE_LDP_INTER_AREA=no
ENABLE_ORDER_NEXTHOP=yes
ENABLE_PNE_3_7=no

dnl ---------------------------------------------------------------------------
dnl                              Platform checks START
dnl ---------------------------------------------------------------------------

dnl ------------------
dnl IPNet TCP/IP stack
dnl ------------------
AC_MSG_CHECKING(With IPNet TCP/IP stack)
if test "$with_ipnet2" = "yes";then
  AC_MSG_RESULT(yes)
  pacos_cv_ipnet=yes
  AC_DEFINE(HAVE_IPNET,,IPNet2)
  ENABLE_IPNET=yes
else
  AC_MSG_RESULT(no)
  ENABLE_IPNET=no
fi

dnl --------------
dnl Broken aliases
dnl --------------
if test "${enable_broken_aliases}" = "yes"; then
  if test "${enable_netlink}" = "yes"
  then
    echo "Sorry, you can't use netlink with broken aliases"
    exit 1
  fi
  AC_DEFINE(HAVE_BROKEN_ALIASES,,Broken Aliases)
  enable_netlink=no
fi

dnl ---------------
dnl Sun CC compiler
dnl ---------------
if test "${enable_sun_cc}" = "yes"; then
  AC_DEFINE(HAVE_SUN_CC,,SUN C Compiler)
  ENABLE_SUN_CC=yes
fi

dnl ----------------------
dnl Sun CC 64 bit compiler
dnl ----------------------
if test "${enable_sun_cc_64_bit}" = "yes"; then
  AC_DEFINE(HAVE_SUN_CC,,SUN C Compiler)
  ENABLE_SUN_CC=yes
  ENABLE_SUN_CC_64_BIT=yes
fi

dnl -------------------
dnl Check header files.
dnl -------------------
AC_STDC_HEADERS
AC_CHECK_HEADERS(string.h stropts.h sys/conf.h sys/ksym.h sys/time.h sys/times.h sys/select.h sys/sysctl.h sys/sockio.h sys/types.h netpacket/packet.h net/ethernet.h net/if_ether.h netinet/if_ether.h net/if_dl.h net/if_var.h linux/version.h kvm.h netdb.h netinet/in.h net/netopt.h netinet/in_var.h netinet/in6_var.h netinet/in6.h inet/nd.h asm/types.h netinet/icmp6.h netinet6/nd6.h netinet6/mip6.h libutil.h getopt.h sys/md5.h linux/tcp_md5.h netinet/ip6.h linux/mroute6.h)

dnl check some types
AC_C_CONST
dnl AC_TYPE_PID_T
AC_TYPE_SIGNAL

AC_SUBST(OPSYSDIR)

dnl ------------------------------------------
dnl Check for support of ISO varargs in macros.
dnl ------------------------------------------
AC_MSG_CHECKING(whether ISO varargs in macros are supported)
AC_TRY_COMPILE([#include <stdio.h>],[
#define TEST_VARARG(...)  printf(__VA_ARGS__);
TEST_VARARG("%s", "ISO vararg works");],
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_ISO_MACRO_VARARGS,,ISO varargs in macros)],
AC_MSG_RESULT(no))

dnl ----------
dnl PAM module
dnl ----------
if test "$with_libpam" = "yes";then
 if test x"$opsys" = x"gnu-linux"; then
  # I think we mimght only need this on redhat linux 6.x
  # and Debian 2.2 so please contribute
  # I do not have access to other OS than FreeBSD >3 and
  # some different linux distributions
  # Someone who knows how should add some more detailed
  # tests for -lpam_misc and -ldl in here
  LIBPAM="-lpam -lpam_misc -ldl"
 else
  LIBPAM="-lpam"
 fi
  AC_DEFINE(USE_PAM,,PAM)
fi
AC_SUBST(LIBPAM)

dnl -------------------------------
dnl Endian-ness check
dnl -------------------------------
dnl Use detected endian-ness
if test "$enable_bigendian" = "" ; then
   AC_WORDS_BIGENDIAN	    
   echo ENDIAN=${ac_cv_c_bigendian}
   if test "${ac_cv_c_bigendian}" = "yes" ; then
      AC_DEFINE(WORDS_BIGENDIAN,1,bigendian)
    else
      AC_DEFINE(WORDS_LITTLEENDIAN,1,littleendian)
   fi
   ENABLE_BIGENDIAN=${ac_cv_c_bigendian}
fi

dnl Check for user override of endian-ness
if test "$enable_bigendian" = "yes" ; then
   AC_DEFINE(WORDS_BIGENDIAN,1,bigendian)
   ENABLE_BIGENDIAN=yes
   ac_cv_c_bigendian=yes
fi
if test "$enable_bigendian" = "no" ; then
   AC_DEFINE(WORDS_LITTLEENDIAN,1,littleendian)
   ac_cv_c_bigendian=no
   ENABLE_BIGENDIAN=no
fi

dnl ---
dnl QOS
dnl ---
if test "$enable_qos" = "yes"; then
   AC_DEFINE(HAVE_QOS,,qos)
   ENABLE_QOS=yes
else
   ENABLE_QOS=no
fi

dnl ---
dnl L2LERN
dnl ---
if test "$enable_l2lern" = "yes"; then
   AC_DEFINE(HAVE_L2LERN,,qos)
   ENABLE_L2LERN=yes
else
   ENABLE_L2LERN=no
fi

dnl --
dnl HWSTACK
dnl --
if test "$enable_hwstack" = "yes" ; then
   AC_DEFINE(HAVE_HWSTACK,,hwstack)
   ENABLE_HWSTACK=yes
else
   ENABLE_HWSTACK=no
fi

dnl --
dnl MULTI_CONF_SES
dnl --
if test "$enable_multi_conf_ses" = "yes" ; then
   AC_DEFINE(HAVE_MULTI_CONF_SES,,multi_conf_ses)
   ENABLE_MULTI_CONF_SES=yes
else
   ENABLE_MULTI_CONF_SES=no
fi


dnl --
dnl HA checkpoint service support
dnl --
if test "$enable_ha" = "yes" ; then
   AC_DEFINE(HAVE_HA,,ha)
   ENABLE_HA=yes
else
   ENABLE_HA=no
fi

dnl --
dnl PACOS_DEBUG switch - Only for the developer/unit test builds
dnl --
if test "$enable_pacos_debug" = "yes" ; then
   AC_DEFINE(HAVE_PACOS_DEBUG,,ha)
   ENABLE_PACOS_DEBUG=yes
else
   ENABLE_PACOS_DEBUG=no
fi

dnl --
dnl FM simulation switch - Simulation of Fault Management functionality
dnl NOTE: FM alone is enabled by default wherever HA is enebaled
dnl --
if test "$enable_fm_sim" = "yes" ; then
   AC_DEFINE(HAVE_FM_SIM,,ha)
   ENABLE_FM_SIM=yes
else
   ENABLE_FM_SIM=no
fi

dnl -------------------------------
dnl check the size in byte of the C
dnl -------------------------------
dnl AC_CHECK_SIZEOF(char)
dnl AC_CHECK_SIZEOF(int)
dnl AC_CHECK_SIZEOF(short)
dnl AC_CHECK_SIZEOF(long)

dnl ----------------------------
dnl check existance of functions
dnl ----------------------------
if test "$cross_compiling" = "no";then
AC_CHECK_FUNCS(bcopy bzero strerror inet_aton daemon snprintf vsnprintf strlcat strlcpy if_nametoindex if_indextoname getifaddrs strcasecmp strncasecmp)
dnl AC_REPLACE_FUNCS(vsnprintf)
AC_CHECK_FUNCS(setproctitle, ,[AC_CHECK_LIB(util, setproctitle, [LIBS="$LIBS -lutil"; AC_DEFINE(HAVE_SETPROCTITLE)])])
fi

dnl ------------------------------------
dnl Determine routing get and set method
dnl ------------------------------------
AC_MSG_CHECKING(pacos between kernel interface method)
if test x"$opsys" = x"gnu-linux"; then
  if test "${enable_netlink}" = "yes";then
    AC_MSG_RESULT(netlink)
    RT_METHOD=rt_netlink.o
    AC_DEFINE(HAVE_NETLINK,,netlink)
    ENABLE_NETLINK=yes
    netlink=yes
  elif test "${enable_netlink}" = "no"; then
    AC_MSG_RESULT(ioctl)
    RT_METHOD=rt_ioctl.o
    netlink=no
  else
    AC_MSG_RESULT(netlink)
    RT_METHOD=rt_netlink.o
    AC_DEFINE(HAVE_NETLINK,,netlink)
    ENABLE_NETLINK=yes
    netlink=yes
  fi
else
  if test "$opsys" = "solaris";then
    AC_MSG_RESULT(solaris)
    KERNEL_METHOD="kernel_socket.o"
    RT_METHOD="rt_socket.o"
  else
    AC_TRY_RUN([#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

main ()
{
  int ac_sock;

  ac_sock = socket (AF_ROUTE, SOCK_RAW, 0);
  if (ac_sock < 0 && errno == EINVAL)
    exit (1);
  exit (0);
}],
  [KERNEL_METHOD=kernel_socket.o
   RT_METHOD=rt_socket.o
   AC_MSG_RESULT(socket)],
  [RT_METHOD=rt_ioctl.o
   AC_MSG_RESULT(ioctl)],
  [KERNEL_METHOD=kernel_socket.o
   RT_METHOD=rt_socket.o
   AC_MSG_RESULT(socket)])
  fi
fi
AC_SUBST(RT_METHOD)
AC_SUBST(KERNEL_METHOD)
AC_SUBST(OTHER_METHOD)

if test "$opsys" = "gnu-linux" ; then
dnl Check for TCP MD5 support in linux kernel
AC_EGREP_CPP(MD5SIG,[#include <linux/autoconf.h>]
[CONFIG_TCP_MD5SIG],,
[AC_EGREP_CPP(TCP_MD5SIG,[#include <netinet/tcp.h>]
[TCP_MD5SIG],,
[AC_DEFINE(HAVE_TCP_MD5SIG,,TCP MD5SIG)])])

dnl Determine kernel version to select correct L2/MPLS software forwarder
dnl Check to see if linux kernel is 2.4 or 2.6
dnl Code 132122 is 2.4.26
AC_EGREP_CPP(132122,[#include <linux/version.h>
LINUX_VERSION_CODE],[mylv=4],[mylv=6])
if test "$mylv" = "6" ; then
dnl Code 132622 is 2.6.14.x
AC_EGREP_CPP(132622,[#include <linux/version.h>
LINUX_VERSION_CODE],[mylvm=14],[mylvm=27])
AC_DEFINE(LINUX_KERNEL_2_6,,Linux 2.6.x kernel)
fi
AC_MSG_CHECKING(Linux version is)
AC_MSG_RESULT(2.${mylv})
fi
LINUX_VERSION_MAJOR="$mylv"
LINUX_VERSION_MINOR="$mylvm"

dnl See if -D_GNU_SOURCE is required for IPV6
AC_EGREP_CPP(in6_pktinfo,
[#include <netinet/in.h>],,
[CFLAGS="${CFLAGS} -D_GNU_SOURCE"])


dnl ------------------------------
dnl check kernel route read method
dnl ------------------------------
AC_CACHE_CHECK(route read method check, pacos_rtread,
[if test "$netlink" = yes; then
  RTREAD_METHOD="rtread_netlink.o"
  pacos_rtread="netlink"
else
for pacos_rtread in /proc/net/route /dev/ip /dev/null;
do
  test x`ls $pacos_rtread 2>/dev/null` = x"$pacos_rtread" && break
done
case $pacos_rtread in
  "/proc/net/route") RTREAD_METHOD="rtread_proc.o"
                     pacos_rtread="proc";;
  "/dev/ip")         RTREAD_METHOD="rtread_getmsg.o"
                     pacos_rtread="getmsg";;
  *)                 RTREAD_METHOD="rtread_sysctl.o"
                     pacos_rtread="sysctl";;
esac
fi])
AC_SUBST(RTREAD_METHOD)

dnl -----------------------------
dnl check interface lookup method
dnl -----------------------------
AC_MSG_CHECKING(interface looking up method)
if test "$netlink" = yes; then
  AC_MSG_RESULT(netlink)
  IF_METHOD=if_netlink.o
else
  if test "$opsys" = "solaris";then
    AC_MSG_RESULT(solaris)
    IF_METHOD=if_solaris.o
  elif test "$opsys" = "openbsd";then
    AC_MSG_RESULT(openbsd)
    IF_METHOD=if_ioctl.o
  elif test "$opsys" = "netbsd" ; then
    AC_MSG_RESULT(netbsd)
    IF_METHOD=if_ioctl.o
    AC_DEFINE(HAVE_NET_RT_IFLIST,,NET_RT_IFLIST ioctl)
  elif grep NET_RT_IFLIST /usr/include/sys/socket.h >/dev/null 2>&1; then
    AC_MSG_RESULT(sysctl)
    IF_METHOD=if_sysctl.o
    AC_DEFINE(HAVE_NET_RT_IFLIST,,NET_RT_IFLIST ioctl)
  else
    AC_MSG_RESULT(ioctl)
    IF_METHOD=if_ioctl.o
  fi
fi
AC_SUBST(IF_METHOD)

dnl -------------------------------
dnl If it's Sun CC, then set CFLAGS
dnl -------------------------------
if test "$ENABLE_SUN_CC" = "yes";then
CFLAGS=" -g -O "
fi

dnl -----------------------
dnl check proc file system.
dnl -----------------------
if test -r /proc/net/dev; then
  AC_DEFINE(HAVE_PROC_NET_DEV,,/proc/net/dev)
  IF_PROC=if_proc.o
fi

if test -r /proc/net/if_inet6; then
  AC_DEFINE(HAVE_PROC_NET_IF_INET6,,/proc/net/if_inet6)
  IF_PROC=if_proc.o
fi
AC_SUBST(IF_PROC)

dnl -----------------------------
dnl check ipforward detect method
dnl -----------------------------
AC_CACHE_CHECK(ipforward method check, pacos_ipforward_path,
[for pacos_ipforward_path in /proc/net/snmp /dev/ip /dev/null;
do
  test x`ls $pacos_ipforward_path 2>/dev/null` = x"$pacos_ipforward_path" && break
done
case $pacos_ipforward_path in
  "/proc/net/snmp")  IPFORWARD=ipforward_proc.o
                     pacos_ipforward_path="proc";;
  "/dev/ip")
                     case "$host" in
                       *-nec-sysv4*)  IPFORWARD=ipforward_ews.o
                                      pacos_ipforward_path="ews";;
                       *)             IPFORWARD=ipforward_solaris.o
                                      pacos_ipforward_path="solaris";;
                     esac;;
  *)                 IPFORWARD=ipforward_sysctl.o
                     pacos_ipforward_path="sysctl";;
esac])
AC_SUBST(IPFORWARD)

if test "$cross_compiling" = "no";then
AC_CHECK_FUNCS(getaddrinfo, [have_getaddrinfo=yes], [have_getaddrinfo=no])
fi

dnl ----------
dnl IPv6 check
dnl ----------
AC_MSG_CHECKING(whether does this OS have IPv6 stack)
if test "${enable_ipv6}" = "no"; then
  pacos_cv_ipv6=no
  AC_MSG_RESULT(disabled)
else
if test "$cross_compiling" = "yes";then
if test "$pacos_cv_ipnet" = "yes" ;then
   pacos_cv_ipv6=yes
   AC_DEFINE(HAVE_IPV6,,IPv6)
   RIPNGD="ripngd"
   OSPF6D="ospf6d"
   PIM6D="pim6d"
fi
# PNE 2.2 support with vxWorks
if test "$pacos_cv_pne22" = "yes" ; then
   pacos_cv_ipv6=yes
   AC_DEFINE(HAVE_IPV6,,IPv6)
   AC_DEFINE(KAME,,KAME)
   RIPNGD="ripngd"
   OSPFD="ospf6d"
   PIM6D="pim6d"
fi

# PNE 3.3 support with vxWorks
if test "$pacos_cv_pne33" = "yes" ; then
   pacos_cv_ipv6=yes
   AC_DEFINE(HAVE_IPV6,,IPv6)
   RIPNGD="ripngd"
   OSPFD="ospf6d"
   PIM6D="pim6d"
fi

if test x"$opsys" = x"netbsd" ; then
   AC_MSG_RESULT(enabled (KAME))
   pacos_cv_ipv6=yes
   AC_DEFINE(HAVE_IPV6,,IPv6)
   AC_DEFINE(KAME,,KAME)
   RIPND="ripngd"
   OSPFD="ospf6d"
   PIM6D="pim6d"
fi

if test x"$opsys" != x"" ; then
   AC_MSG_RESULT(enabled (cross-compiled))
   pacos_cv_ipv6=yes
   AC_DEFINE(HAVE_IPV6,,IPv6)
   RIPNGD="ripngd"
   OSPF6D="ospf6d"
   PIM6D="pim6d"
fi

else
dnl ----------
dnl INRIA IPv6
dnl ----------
if grep IPV6_INRIA_VERSION /usr/include/netinet/in.h >/dev/null 2>&1; then
   pacos_cv_ipv6=yes
   AC_DEFINE(HAVE_IPV6,,IPv6)
   AC_DEFINE(INRIA_IPV6,,Inria IPv6)
   RIPNGD="ripngd"
   OSPF6D="ospf6d"
   PIM6D="pim6d"
   LIB_IPV6=""
   AC_MSG_RESULT(INRIA IPv6)
fi
dnl ---------
dnl KAME IPv6
dnl ---------
if grep WIDE /usr/include/netinet6/in6.h >/dev/null 2>&1; then
   pacos_cv_ipv6=yes
   AC_DEFINE(HAVE_IPV6,,IPv6)
   AC_DEFINE(KAME,,KAME IPv6)
   RIPNGD="ripngd"
   OSPF6D="ospf6d"
   PIM6D="pim6d"
   if test -d /usr/local/v6/lib -a -f /usr/local/v6/lib/libinet6.a; then
      LIB_IPV6="-L/usr/local/v6/lib -linet6"
   fi
   AC_MSG_RESULT(KAME)

   AC_MSG_CHECKING(whether does this OS have MIP6 stack)
   if test "${enable_mip6}" = no; then
     pacos_cv_mip6=no
     AC_MSG_RESULT(disabled)
   else
     if grep SIOCSMIP6CFG /usr/include/netinet6/mip6.h >/dev/null 2>&1; then
       pacos_cv_mip6=yes
       AC_DEFINE(HAVE_MIP6,,MIPv6 support in router advertisement)
       AC_MSG_RESULT(KAME MIP6)
     else
       pacos_cv_mip6=no
       AC_MSG_RESULT(no)
     fi
   fi
fi
dnl ---------
dnl NRL check
dnl ---------
if grep NRL /usr/include/netinet6/in6.h >/dev/null 2>&1; then
   pacos_cv_ipv6=yes
   AC_DEFINE(HAVE_IPV6,,IPv6)
   AC_DEFINE(NRL,,NRL)
   RIPNGD="ripngd"
   OSPF6D="ospf6d"
   PIM6D="pim6d"
   if test x"$opsys" = x"bsdi";then
      AC_DEFINE(BSDI_NRL,,BSDI)
      AC_MSG_RESULT(BSDI_NRL)
   else
      AC_MSG_RESULT(NRL)
   fi
fi

dnl ----------
dnl Linux IPv6
dnl ----------
if test "${enable_ipv6}" = "yes"; then
   AC_EGREP_CPP(yes, [
   #include <linux/version.h>
   /* 2.1.128 or later */
   #if LINUX_VERSION_CODE >= 0x020180
   yes
   #endif],
   [pacos_cv_ipv6=yes; pacos_cv_linux_ipv6=yes;AC_MSG_RESULT(Linux IPv6)])
else
   if test x`ls /proc/net/ipv6_route 2>/dev/null` = x"/proc/net/ipv6_route"
   then
      pacos_cv_ipv6=yes
      pacos_cv_linux_ipv6=yes
      AC_MSG_RESULT(Linux IPv6)
   fi
fi

if test "$pacos_cv_linux_ipv6" = "yes";then
   AC_DEFINE(HAVE_IPV6)
   AC_MSG_CHECKING(for GNU libc 2.1)
   AC_EGREP_CPP(yes, [
#include <features.h>
#if __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 1
  yes
#endif], [glibc=yes; AC_MSG_RESULT(yes)], AC_MSG_RESULT(no))
   AC_DEFINE(LINUX_IPV6,,Linux IPv6)
   RIPNGD="ripngd"
   OSPF6D="ospf6d"
   PIM6D="pim6d"
   if test "$glibc" != "yes"; then
      INCLUDES="-I/usr/inet6/include"
      if test x`ls /usr/inet6/lib/libinet6.a 2>/dev/null` != x;then
         LIB_IPV6="-L/usr/inet6/lib -linet6"
      fi
   fi
fi
dnl ------------------
dnl Solaris IPv6 check
dnl ------------------
if grep "Sun Microsystems" /usr/include/netinet/ip6.h >/dev/null 2>&1; then
   pacos_cv_ipv6=yes
   AC_DEFINE(HAVE_IPV6,,IPv6)
   AC_DEFINE(SOLARIS_IPV6,,Solaris IPv6)
   RIPNGD="ripngd"
   OSPF6D="ospf6d"
   PIM6D="pim6d"
   AC_MSG_RESULT(Solaris IPv6)
fi

if test "${pacos_cv_ipv6}" != yes; then
  AC_MSG_RESULT(IPv4 only)
fi
fi
fi

dnl -----------------------
dnl Set IPv6 related values
dnl -----------------------
if test "${pacos_cv_ipv6}" = yes; then
  LIBS="$LIB_IPV6 $LIBS"
  AC_SUBST(LIB_IPV6)
  ENABLE_IPV6=yes
else
  ENABLE_IPV6=no
fi

if test "${enable_rtadv}" != no && test "${pacos_cv_ipv6}" = yes; then
  AC_MSG_CHECKING(whether does this OS support IPv6 router advertisement)
  if test "${with_pne22}" = "yes" || test "${with_pne33}" = "yes"; then
        AC_DEFINE(HAVE_RTADV,,IPv6 Router Advertisement)
        ENABLE_RTAV=yes
        AC_MSG_RESULT(yes)
  else
        AC_EGREP_HEADER(nd_router_advert,
        netinet/icmp6.h,
        [pacos_cv_rtadv=yes;AC_MSG_RESULT(yes)
        AC_DEFINE(HAVE_RTADV,,IPv6 Router Advertisement)
        ENABLE_RTADV=yes],
        AC_MSG_RESULT(no)
        ENABLE_RTADV=no)
   fi
else
  ENABLE_RTADV=no
fi

dnl --------------------
dnl Check for ffs
dnl --------------------
if test "$cross_compiling" = "no";then
AC_CHECK_FUNCS(ffs, [have_ffs=yes], [have_ffs=no])
if test "${have_ffs}" = yes; then
  AC_DEFINE(HAVE_FFS)
fi
fi

dnl --------------------
dnl Hostname change
dnl --------------------
if test "${enable_hostname_change}" = "yes"; then
  AC_DEFINE(HAVE_HOSTNAME_CHANGE,,Hostname change)
fi

dnl ---------------------
dnl Developer's test code
dnl ---------------------
if test "${enable_dev_test}" = "yes"; then
  AC_DEFINE(HAVE_DEV_TEST,,Developer's test code)
  ENABLE_DEV_TEST=yes
fi

if test "${enable_efence}" = "yes"; then
  AC_CHECK_LIB(efence, memalign, [EFENCE_LIB="-lefence"])
  dnl if test -f /usr/lib/libefence.a; then
    dnl EFENCE_LIB="/usr/lib/libefence.a"
  dnl fi
  ENABLE_EFENCE=yes
fi
AC_SUBST(EFENCE_LIB)

if test "$cross_compiling" = "yes" ; then
if test x"$opsys" != x"" ; then

AC_CHECK_LIB(c, inet_ntop, [AC_DEFINE(HAVE_INET_NTOP,,inet_ntop)])
AC_CHECK_LIB(c, inet_pton, [AC_DEFINE(HAVE_INET_PTON,,inet_pton)])
AC_CHECK_LIB(crypt, crypt)
AC_CHECK_LIB(resolv, res_init)
AC_CHECK_LIB(m, main)
AC_CHECK_LIB(md, MD5Init)
AC_CHECK_LIB(md5, MD5Init)

fi
fi

if test "$cross_compiling" = "no" -o x"$opsys" != x"" ; then
AC_CHECK_LIB(c, inet_ntop, [AC_DEFINE(HAVE_INET_NTOP,,inet_ntop)])
AC_CHECK_LIB(c, inet_pton, [AC_DEFINE(HAVE_INET_PTON,,inet_pton)])
AC_CHECK_LIB(crypt, crypt)
AC_CHECK_LIB(resolv, res_init)
AC_CHECK_LIB(m, main)
AC_CHECK_LIB(md, MD5Init)
AC_CHECK_LIB(md5, MD5Init)

dnl -------------------------
dnl OpenSSL MD5 library check
dnl -------------------------
if test "${HAVE_SYS_MD5_H}" != "yes"; then
  for ac_ssl_md5 in /usr/ssl/include/openssl/md5.h /usr/local/ssl/include/openssl/md5.h /usr/include/openssl/md5.h /dev/null
  do
    test -f "${ac_ssl_md5}" && break
  done

  AC_CHECK_LIB(crypto, MD5_Init, [HAVE_CRYPTO=yes])

  case ${ac_ssl_md5} in
    /usr/ssl/include/openssl/*)
      if test "${HAVE_CRYPTO}" = "yes"; then
        AC_DEFINE(HAVE_OPENSSL_MD5_H,,OpenSSL MD5)
        LIBS="${LIBS} -lcrypto"
        CFLAGS="${CFLAGS} -I/usr/ssl/include"
      fi
      ;;
    /usr/local/ssl/include/openssl/*)
      if test "${HAVE_CRYPTO}" = "yes"; then
        AC_DEFINE(HAVE_OPENSSL_MD5_H,,OpenSSL MD5)
        LIBS="${LIBS} -lcrypto"
        CFLAGS="${CFLAGS} -I/usr/local/ssl/include"
      fi
      ;;
    /usr/include/openssl/*)
      if test "${HAVE_CRYPTO}" = "yes"; then
        AC_DEFINE(HAVE_OPENSSL_MD5_H,,OpenSSL MD5)
        LIBS="${LIBS} -lcrypto"
      fi
      ;;
    *)
      ac_cv_lib_crypto_MD5_Init="no"
      ;;
  esac
fi


dnl -------------------------
dnl OpenSSL HMAC library check
dnl -------------------------
if test "${HAVE_SYS_HMAC_H}" != "yes"; then
  for ac_ssl_hmac in /usr/ssl/include/openssl/hmac.h /usr/local/ssl/include/openssl/hmac.h /usr/include/openssl/hmac.h /dev/null
  do
    test -f "${ac_ssl_hmac}" && break
  done

  AC_CHECK_LIB(crypto, HMAC_Init, [HAVE_CRYPTO=yes])

  case ${ac_ssl_hmac} in
    /usr/ssl/include/openssl/*)
      if test "${HAVE_CRYPTO}" = "yes"; then
        AC_DEFINE(HAVE_OPENSSL_HMAC_H,,OpenSSL HMAC)
        LIBS="${LIBS} -lcrypto"
        CFLAGS="${CFLAGS} -I/usr/ssl/include"
      fi
      ;;
    /usr/local/ssl/include/openssl/*)
      if test "${HAVE_CRYPTO}" = "yes"; then
        AC_DEFINE(HAVE_OPENSSL_HMAC_H,,OpenSSL HMAC)
        LIBS="${LIBS} -lcrypto"
        CFLAGS="${CFLAGS} -I/usr/local/ssl/include"
      fi
      ;;
    /usr/include/openssl/*)
      if test "${HAVE_CRYPTO}" = "yes"; then
        AC_DEFINE(HAVE_OPENSSL_HMAC_H,,OpenSSL HMAC)
        LIBS="${LIBS} -lcrypto"
      fi
      ;;
    *)
      ac_cv_lib_crypto_HMAC_Init="no"
      ;;
  esac
fi

dnl --
dnl ENEA support - HA checkpoint service support from ENEA
dnl --
if test "$enable_enea_element" = "yes" ; then
   AC_DEFINE(HAVE_ENEA_ELEMENT,,enea_element)
   ENABLE_ENEA_ELEMENT=yes
   LIBS="${LIBS} -lpthread -lexpat"
else
   ENABLE_ENEA_ELEMENT=no
fi

dnl --
dnl OpenSAF support - HA checkpoint service support from OpenSAF
dnl --
if test "$enable_open_saf" = "yes" ; then
   AC_DEFINE(HAVE_OPEN_SAF,,open_saf)
   ENABLE_OPEN_SAF=yes
   LIBS="${LIBS} -lpthread -lexpat"
else
   ENABLE_OPEN_SAF=no
fi

dnl ---------------------------------------------------
dnl BSD/OS 4.1 define inet_XtoY function as __inet_XtoY
dnl ---------------------------------------------------
AC_CHECK_FUNC(__inet_ntop, AC_DEFINE(HAVE_INET_NTOP))
AC_CHECK_FUNC(__inet_pton, AC_DEFINE(HAVE_INET_PTON))
AC_CHECK_FUNC(__inet_aton, AC_DEFINE(HAVE_INET_ATON))

dnl ---------------------------
dnl check system has GNU regexp
dnl ---------------------------
dnl AC_MSG_CHECKING(whether system has GNU regex)
AC_CHECK_LIB(c, regexec,
[AC_DEFINE(HAVE_GNU_REGEX,,Have GNU regexp)
LIB_REGEX=""],
[LIB_REGEX="regex.o"])
AC_SUBST(LIB_REGEX)

dnl AC_MSG_CHECKING(whether system has GNU regex)
dnl if grep RE_NO_GNU_OPS /usr/include/regex.h >/dev/null 2>&1; then
dnl   AC_MSG_RESULT(yes)
dnl   AC_DEFINE(HAVE_GNU_REGEX)
dnl   LIB_REGEX=""
dnl else
dnl   AC_MSG_RESULT(no)
dnl   LIB_REGEX="regex.o"
dnl fi
dnl AC_SUBST(LIB_REGEX)

dnl ----------------------------
dnl check sa_len of sockaddr
dnl ----------------------------
AC_MSG_CHECKING(whether struct sockaddr has a sa_len field)
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>
],[static struct sockaddr ac_i;int ac_j = sizeof (ac_i.sa_len);],
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_SA_LEN,,sa_len)],
 AC_MSG_RESULT(no))

dnl ----------------------------
dnl check sin_len of sockaddr_in
dnl ----------------------------
AC_MSG_CHECKING(whether struct sockaddr_in has a sin_len field)
AC_TRY_COMPILE([#include <sys/types.h>
#include <netinet/in.h>
],[static struct sockaddr_in ac_i;int ac_j = sizeof (ac_i.sin_len);],
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_SIN_LEN,,sin_len)],
 AC_MSG_RESULT(no))

dnl ----------------------------
dnl check sun_len of sockaddr_un
dnl ----------------------------
AC_MSG_CHECKING(whether struct sockaddr_un has a sun_len field)
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/un.h>
],[static struct sockaddr_un ac_i;int ac_j = sizeof (ac_i.sun_len);],
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_SUN_LEN,,sun_len)],
 AC_MSG_RESULT(no))

dnl -----------------------------------
dnl check sin6_scope_id of sockaddr_in6
dnl -----------------------------------
if test "$pacos_cv_ipv6" = yes; then
  AC_MSG_CHECKING(whether struct sockaddr_in6 has a sin6_scope_id field)
  AC_TRY_COMPILE([#include <sys/types.h>
#include <netinet/in.h>
],[static struct sockaddr_in6 ac_i;int ac_j = sizeof (ac_i.sin6_scope_id);],
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_SIN6_SCOPE_ID,,sin6_scope_id)],
 AC_MSG_RESULT(no))
fi

dnl ----------------------------
dnl check socklen_t exist or not
dnl ----------------------------
AC_MSG_CHECKING(whther socklen_t is defined)
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
],[socklen_t ac_x;],
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_SOCKLEN_T,,socklen_t)],
 AC_MSG_RESULT(no))

dnl ------------------------
dnl check struct sockaddr_dl
dnl ------------------------
AC_MSG_CHECKING(whether struct sockaddr_dl exist)
AC_EGREP_HEADER(sockaddr_dl,
net/if_dl.h,
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_SOCKADDR_DL,,sockaddr_dl)],
 AC_MSG_RESULT(no))

dnl --------------------------
dnl check structure ifaliasreq
dnl --------------------------
AC_MSG_CHECKING(whether struct ifaliasreq exist)
AC_EGREP_HEADER(ifaliasreq,
net/if.h,
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_IFALIASREQ,,ifaliasreq)],
 AC_MSG_RESULT(no))

dnl ----------------------------
dnl check structure in6_aliasreq
dnl ----------------------------
AC_MSG_CHECKING(whether struct if6_aliasreq exist)
AC_EGREP_HEADER(in6_aliasreq,
netinet6/in6_var.h,
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_IN6_ALIASREQ,,if6_aliasreq)],
 AC_MSG_RESULT(no))

dnl ---------------------------
dnl check structure rt_addrinfo
dnl ---------------------------
AC_MSG_CHECKING(whether struct rt_addrinfo exist)
AC_EGREP_HEADER(rt_addrinfo,
net/route.h,
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_RT_ADDRINFO,,rt_addrinfo)],
 AC_MSG_RESULT(no))

dnl ----------------------------------
dnl check structure nd_opt_advinterval
dnl ----------------------------------
if test "$pacos_cv_rtadv" = yes; then
  AC_MSG_CHECKING(whether struct nd_opt_advinterval exist)
  AC_EGREP_HEADER(nd_opt_advinterval,
  netinet/icmp6.h,
  [AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_ND_OPT_ADVINTERVAL,,nd_opt_advinterval)],
   AC_MSG_RESULT(no))
fi

dnl -------------------------------------
dnl check structure nd_opt_homeagent_info
dnl -------------------------------------
if test "$pacos_cv_rtadv" = yes; then
  AC_MSG_CHECKING(whether struct nd_opt_homeagent_info exist)
  AC_EGREP_HEADER(nd_opt_homeagent_info,
  netinet/icmp6.h,
  [AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_ND_OPT_HOMEAGENT_INFO,,nd_opt_homeagent_info)],
   AC_MSG_RESULT(no))
fi

dnl -----------------------------
dnl check structure ha_discov_req
dnl -----------------------------
if test "$pacos_cv_rtadv" = yes; then
  AC_MSG_CHECKING(whether struct ha_discov_req exit)
  AC_EGREP_HEADER(ha_discov_req,
  netinet/icmp6.h,
  [AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_HA_DISCOV_REQ,,ha_discov_req)],
   AC_MSG_RESULT(no))
fi

dnl -----------------------------
dnl check structure ha_discov_rep
dnl -----------------------------
if test "$pacos_cv_rtadv" = yes; then
  AC_MSG_CHECKING(whether struct ha_discov_rep exit)
  AC_EGREP_HEADER(ha_discov_rep,
  netinet/icmp6.h,
  [AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_HA_DISCOV_REP,,ha_discov_rep)],
   AC_MSG_RESULT(no))
fi

dnl -----------------------------
dnl check structure in_pktinfo
dnl -----------------------------
if test "$pacos_cv_ipnet" != "yes"; then
  AC_MSG_CHECKING(whether struct in_pktinfo exit)
  AC_TRY_COMPILE([#include <netinet/in.h>
  ],[struct in_pktinfo ac_i;],
  [AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_IN_PKTINFO,,IN PKTINFO)],
   AC_MSG_RESULT(no))
fi

fi

dnl ---------------------------------------------------------------------------
dnl                              Platform checks END
dnl ---------------------------------------------------------------------------

dnl --------------------------------------------------------------------------
dnl                              PacOS checks START
dnl --------------------------------------------------------------------------

dnl ----------------------------------------------
dnl Enable TCP messaging between NSM and protocols
dnl ----------------------------------------------
if test "${enable_tcp_message}" = "yes"; then
  AC_DEFINE(HAVE_TCP_MESSAGE,,Use TCP for daemons communication)
fi

dnl -------------------------------------
dnl Integrated Management Interface (WMI)
dnl -------------------------------------
AC_MSG_CHECKING(for WMI)
if test "${enable_wmi}" = "yes"; then
  AC_MSG_RESULT(enabled)
  AC_DEFINE(HAVE_WMI,,WMI)
  WMI="wmi"
  ENABLE_WMI=yes
fi
AC_SUBST(WMI)

dnl -------------------------------------
dnl Integrated Management Interface (IMI)
dnl -------------------------------------
AC_MSG_CHECKING(for IMI)
if test "${enable_imi}" = "yes" || test "${enable_imish}" = "yes"; then
  AC_MSG_RESULT(enabled)
  AC_DEFINE(HAVE_IMI,,IMI)
  IMILIB=../imi/libimi.a
  IMI="imi"
  ENABLE_IMI=yes

  dnl -----------------------------------
  dnl IMI shell related parameter (IMISH)
  dnl -----------------------------------
  AC_MSG_CHECKING(for IMISH)
  if test "${enable_imish}" = "yes"; then
    AC_MSG_RESULT(enabled)
    AC_DEFINE(HAVE_IMISH,,IMI shell)
    ENABLE_IMISH=yes
    AC_HEADER_DIRENT
    AC_HEADER_STDC
    AC_HEADER_SYS_WAIT
    AC_TYPE_SIZE_T
    AC_FUNC_ALLOCA
    AC_PROG_GCC_TRADITIONAL
    AC_CHECK_HEADERS(limits.h malloc.h sys/ioctl.h unistd.h sys/cdefs.h vis.h)
    AC_CHECK_FUNCS(re_comp regcomp strdup strstr strtol issetugid fgetln getline flockfile)
    AC_CHECK_LIB(termcap, tgetent, [LIBS="-ltermcap ${LIBS}"])
    AC_CHECK_LIB(curses, tgoto, [LIBS="-lcurses ${LIBS}"])
  else
    AC_MSG_RESULT(disabled)
  fi
else
  AC_MSG_RESULT(disabled)
fi
AC_SUBST(IMILIB)
AC_SUBST(IMI)

if test "${enable_config_compare}" = "yes"; then
  AC_DEFINE(HAVE_CONFIG_COMPARE,,CONFIG_COMPARE)
  ENABLE_CONFIG_COMPARE=yes
  CONFIG_COMPARE="config_compare"
fi

dnl ---------------------------
dnl 6PE Routing
dnl ---------------------------
if test "${enable_6pe}" = "yes"; then
  AC_DEFINE(HAVE_6PE,,6pe)
  ENABLE_6PE=yes
fi

dnl ---------------------------
dnl LDP Inter Area LSP
dnl ---------------------------
if test "${enable_ldp_inter_area}" = "yes"; then
  AC_DEFINE(HAVE_LDP_INTER_AREA,,INTER AREA LSP)
  AC_DEFINE(HAVE_LDPD,,ldpd)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_LDP_INTER_AREA=yes
  ENABLE_LDPD=yes
  ENABLE_MPLS=yes
else
  ENABLE_LDP_INTER_AREA=no
fi

dnl ---------------------------
dnl LDP SUPPRESS FEC
dnl ---------------------------
if test "${enable_ldp_suppress_fec}" = "yes"; then
  AC_DEFINE(HAVE_LDP_SUPPRESS_FEC,,SUPPRESS FEC)
  AC_DEFINE(HAVE_LDPD,,ldpd)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_LDP_SUPPRESS_FEC=yes
  ENABLE_LDPD=yes
  ENABLE_MPLS=yes
fi

dnl ---------------------------------------
dnl Bidirectional Forwarding Detection (BFD)
dnl ---------------------------------------
if test "${enable_bfd}" = "no" ; then
  ENABLE_BFD_MAIN=no
  ENABLE_BFD_MONO=no
  ENABLE_BFD_LINE=no
else
if test "${enable_bfd_main}" = "yes"; then
  AC_DEFINE(HAVE_BFD_MAIN,,bfd)
  ENABLE_BFD_MAIN=yes
  ENABLE_BFD_MONO=no
  ENABLE_BFD_LINE=no
else
if test "${enable_bfd_line}" = "yes"; then
  AC_DEFINE(HAVE_BFD_LINE,,bfd)
  ENABLE_BFD_LINE=yes
  ENABLE_BFD_MONO=no
  ENABLE_BFD_MAIN=no
else
  AC_DEFINE(HAVE_BFD_MONO,,bfd)
  ENABLE_BFD_MONO=yes
  ENABLE_BFD_MAIN=no
  ENABLE_BFD_LINE=no
fi
fi
fi

dnl ---------------------------
dnl VPN Routing Forwarding(VRF)
dnl ---------------------------
if test "${enable_vrf}" = "yes"; then
  AC_DEFINE(HAVE_VRF,,VRF)
  AC_DEFINE(MPLS_VPN,,MPLS VPN)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_VRF=yes
  ENABLE_MPLS=yes

  if test "${enable_pece_ospf}" = "yes"; then
    AC_DEFINE(HAVE_VRF_OSPF,,OSPF VRF)
  fi

  if test "${enable_pece_rip}" = "yes"; then
    AC_DEFINE(HAVE_VRF_RIP,,RIP VRF)
  fi

dnl ---------------------------
dnl OSPF6 for PE to CE Routing
dnl ---------------------------
  if test "${enable_pece_ospf6}" = "yes"; then
    AC_DEFINE(HAVE_VRF_OSPF6,,OSPF6 VRF)
  fi

dnl ---------------------------
dnl RIPNG for PE to CE Routing
dnl ---------------------------
  if test "${enable_pece_ripng}" = "yes"; then
    AC_DEFINE(HAVE_VRF_RIPNG,,RIPNG VRF)
  fi

fi

if test "${enable_mpls}" = "yes"; then
  AC_DEFINE(HAVE_MPLS,,MPLS)
  AC_DEFINE(HAVE_PACKET,,Packet)
  ENABLE_MPLS=yes
  ENABLE_PACKET=yes

  if test "${enable_mpls_bk_lsp}" = "yes"; then
    AC_DEFINE(HAVE_MPLS_INSTALL_BK_LSP,,MPLS Install Backup Lsp)
  fi
fi

if test "${enable_pbr}" = "yes"; then
  AC_DEFINE(HAVE_PBR,,PBR)
  ENABLE_PBR=yes
fi

if test "${enable_packet}" = "yes"; then
  AC_DEFINE(HAVE_PACKET,,Packet)
  ENABLE_PACKET=yes
fi

if test "${enable_gels_edge}" = "yes"; then
  AC_DEFINE(HAVE_GELS,,GELS)
  AC_DEFINE(HAVE_PBB_TE,,pbb-te)
  AC_DEFINE(HAVE_I_BEB,,i-pbb)
  AC_DEFINE(HAVE_B_BEB,,b-pbb)
  AC_DEFINE(HAVE_GMPLS,,GMPLS)
  ENABLE_GMPLS=yes
  ENABLE_PBB_TE=yes
  ENABLE_I_BEB=yes
  ENABLE_B_BEB=yes
  PBB_TE="pbb-te"
  I_BEB="i-pbb"
  B_BEB="b-pbb"
else
  ENABLE_PBB_TE=no
  ENABLE_I_BEB=no
  ENABLE_B_BEB=no
  PBB_TE=""
  I_BEB=""
  B_BEB=""
fi
AC_SUBST(I_BEB)
AC_SUBST(B_BEB)

if test "${enable_gels_core}" = "yes"; then
  AC_DEFINE(HAVE_GELS,,GELS)
  AC_DEFINE(HAVE_PBB_TE,,pbb-te)
  AC_DEFINE(HAVE_GMPLS,,GMPLS)
  ENABLE_GMPLS=yes
  ENABLE_PBB_TE=yes
  PBB_TE="pbb-te"
else
  ENABLE_PBB_TE=no
  PBB_TE=""
fi
AC_SUBST(PBB_TE)

if test "${enable_lmpd}" = "yes"; then
  AC_DEFINE(HAVE_LMP,,Link Management Protocol)
  AC_DEFINE(HAVE_GMPLS,,GMPLS)
  ENABLE_LMP=yes
  ENABLE_GMPLS=yes
fi

if test "${enable_mpls_fwd}" = "yes"; then
  AC_DEFINE(HAVE_MPLS_FWD,,MPLS forwarder)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  MPLS="mpls"
  ENABLE_MPLS_FWD=yes
  ENABLE_MPLS=yes
else
    MPLS=""
fi
AC_SUBST(MPLS)

dnl ------------------------------
dnl MPLS Operations and Management
dnl ------------------------------

if test "${enable_mpls_oam}" = "yes"; then
  AC_DEFINE(HAVE_MPLS_OAM,,MPLS oam)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  MPLS="mpls"
  ENABLE_MPLS_OAM=yes
  ENABLE_MPLS=yes
  MPLSONM="mplsonm"
else
    MPLS=""
    MPLSONM=""
fi



dnl ---------------
dnl Virtual Routing
dnl ---------------
AC_MSG_CHECKING(Virtual Router)
if test "${enable_vr}" = "yes"; then
  AC_MSG_RESULT(enabled)
  AC_DEFINE(HAVE_VR,,VR)
  ENABLE_VR=yes
  if test "${enable_vlogd}" = "yes"; then
    AC_MSG_RESULT(enabled)
    AC_DEFINE(HAVE_VLOGD,,VLOGD)
    ENABLE_VLOGD=yes
  else
    AC_MSG_RESULT(disabled)
    ENABLE_VLOGD=no
  fi
else
  AC_MSG_RESULT(disabled)
  ENABLE_VR=no
  ENABLE_VLOGD=no
fi

dnl ---------------------------------------------------
dnl Multiple FIB support in the kernel (linux-vrf, etc)
dnl ---------------------------------------------------
AC_MSG_CHECKING(linux-vrf)
if test "${enable_multiple_fib}" = "yes"; then
  AC_MSG_RESULT(enabled)
  AC_DEFINE(HAVE_MULTIPLE_FIB,,Multiple FIB kernel support)
else
  AC_MSG_RESULT(disabled)
fi

dnl ----------------------------------
dnl Virtual Routing for CheckPoint VSX
dnl ----------------------------------
AC_MSG_CHECKING(Virtual Router for CheckPoint VSX)
if test "${enable_vrx}" = "yes"; then
  AC_MSG_RESULT(enabled)
  AC_DEFINE(HAVE_VRF,,VRF)
  AC_DEFINE(HAVE_VRF_OSPF,,OSPF VRF)
  AC_DEFINE(HAVE_VRF_RIP,,RIP VRF)
  AC_DEFINE(HAVE_VRX,,Virtual Routing for CheckPoint VSX)
  AC_DEFINE(HAVE_VR,,VR)
  AC_DEFINE(HAVE_MULTIPLE_FIB,,Multiple FIB kernel support)
  ENABLE_VR=yes
else
  AC_MSG_RESULT(disabled)
fi

dnl -------------------------
dnl PacOS Clustering solution
dnl -------------------------
AC_MSG_CHECKING(PacOS Clustering solution)
if test "${enable_crx}" = "yes"; then
  AC_MSG_RESULT(enabled)
  AC_DEFINE(HAVE_CRX,,PacOS Clustering solution)
  ENABLE_CRX=yes
else
  AC_MSG_RESULT(disabled)
fi

dnl ---------
dnl RIP ECMP
dnl ---------
if test "${enable_rip_ecmp}" != "no"; then
  AC_DEFINE(HAVE_RIP_ECMP,,RIP ECMP)
fi

dnl ---------
dnl OSPF NSSA
dnl ---------
if test "${enable_nssa}" != "no"; then
  AC_DEFINE(HAVE_NSSA,,OSPF NSSA)
fi

dnl ----------------------
dnl OSPF Database Overflow
dnl ----------------------
if test "${enable_ospf_db_overflow}" != "no"; then
  AC_DEFINE(HAVE_OSPF_DB_OVERFLOW,,OSPF database overflow)
fi

dnl ---------------------------------------
dnl OSPF Multiple Areas on Single Interface
dnl ---------------------------------------
if test "${enable_ospf_multi_area}" = "yes"; then
  AC_DEFINE(HAVE_OSPF_MULTI_AREA,,OSPF Multi Area)
  ENABLE_OSPF_MULTI_AREA=yes
fi

dnl --------------------------------------------
dnl OSPF Multiple Instances on Single Interafce
dnl --------------------------------------------
if test "${enable_ospf_multi_inst}" = "yes"; then
  AC_DEFINE(HAVE_OSPF_MULTI_INST,,OSPF Multi Instance)
  ENABLE_OSPF_MULTI_INST=yes
fi

dnl ---------------
dnl OSPF Opaque LSA
dnl ---------------
if test "${enable_opaque_lsa}" != "no"; then
  AC_DEFINE(HAVE_OPAQUE_LSA,,OSPF Opaque LSA)
fi

dnl ----------------
dnl All in one PacOS
dnl ----------------
if test "${enable_all_in_one}" = "yes"; then
  AC_DEFINE(HAVE_ALL_IN_ONE,,All in one configuration -- not yet supported)
  AIO="aio"
  ENABLE_AIO=yes
fi

dnl ---------
dnl OSPF CSPF
dnl ---------
if test "${enable_ospf_cspf}" = "yes"; then
  AC_DEFINE(HAVE_OSPF_CSPF,,OSPF CSPF)
  AC_DEFINE(HAVE_OSPF_TE,,OSPF TE)
  AC_DEFINE(HAVE_OPAQUE_LSA,,OSPF Opaque LSA)
  ENABLE_OSPF_CSPF=yes
  ENABLE_CSPF=yes
fi

dnl ---------
dnl OSPF6 CSPF
dnl ---------
if test "${enable_ospf6_cspf}" = "yes"; then
  AC_DEFINE(HAVE_OSPF6_CSPF,,OSPF6 CSPF)
  AC_DEFINE(HAVE_OSPF_TE,,OSPF TE)
  ENABLE_OSPF6_CSPF=yes
  ENABLE_CSPF=yes
fi

dnl ---------
dnl ISIS CSPF
dnl ---------
if test "${enable_isis_cspf}" = "yes"; then
  AC_DEFINE(HAVE_ISIS_CSPF,,ISIS CSPF)
  AC_DEFINE(HAVE_ISIS_TE,,ISIS TE)
  ENABLE_ISIS_CSPF=yes
  ENABLE_CSPF=yes
fi

dnl ----
dnl CSPF
dnl ----
if test "$ENABLE_CSPF" = "yes"; then
  AC_DEFINE(HAVE_CSPF,,CSPF)
  AC_DEFINE(HAVE_TE,,Traffic Engineering)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_MPLS=yes
fi

dnl -----------------
dnl MPLS Fast Reroute
dnl -----------------
if test "${enable_mpls_frr}" = "yes"; then
  AC_DEFINE(HAVE_MPLS_FRR,,MPLS Fast Reroute)
  AC_DEFINE(HAVE_CSPF,,CSPF)
  AC_DEFINE(HAVE_OSPF_CSPF,,OSPF CSPF)
  if test "${enable_ospf6_cspf}" = "yes"; then
    AC_DEFINE(HAVE_OSPF6_CSPF,,OSPF6 CSPF)
  fi
  AC_DEFINE(HAVE_OSPF_TE,,OSPF TE)
  AC_DEFINE(HAVE_OPAQUE_LSA,,OSPF Opaque LSA)
  AC_DEFINE(HAVE_TE,,Traffic Engineering)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_MPLS_FRR=yes
  ENABLE_CSPF=yes
  ENABLE_OSPF_CSPF=yes
  if test "${enable_ospf6_cspf}" = "yes"; then
    ENABLE_OSPF6_CSPF=yes
  fi
  ENABLE_MPLS=yes
fi

dnl ------
dnl CR-LDP
dnl ------
if test "${enable_cr_ldp}" = "yes"; then
  CRLDPLIB="../ldpd/crldp/libcrldp.a"
  CRLDPINCLUDE="-I../ldpd/crldp/"
  CRLDP="crldp"
  AC_DEFINE(HAVE_CR_LDP,,CR-LDP)
  AC_DEFINE(HAVE_TE,,Traffic Engineering)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_CR_LDP=yes
  ENABLE_MPLS=yes
else
  CRLDPLIB=""
  CRLDPINCLUDE=""
  CRLDP=""
fi

AC_SUBST(CRLDPLIB)
AC_SUBST(CRLDPINCLUDE)
AC_SUBST(CRLDP)

dnl --------------------
dnl MPLS Virtual Circuit
dnl --------------------
if test "${enable_mpls_vc}" = "yes"; then
  AC_DEFINE(HAVE_MPLS_VC,,MPLS VC)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  VC="vc"
  ENABLE_MPLS_VC=yes
  ENABLE_MPLS=yes
else
  VC=""
fi

dnl -----------------------------------------
dnl Virtual Circuit Connectivity Verification
dnl -----------------------------------------
if test "${enable_vccv}" = "yes"; then
  AC_DEFINE(HAVE_VCCV,,VCCV)
  AC_DEFINE(HAVE_MPLS_OAM,,MPLS oam)
  AC_DEFINE(HAVE_MPLS_VC,,MPLS VC)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_VCCV=yes
  ENABLE_MPLS_OAM=yes
  ENABLE_MPLS_VC=yes
  ENABLE_MPLS=yes
fi

dnl ----------------------------------
dnl Multi-segment Pseudowires
dnl ----------------------------------
if test "${enable_ms_pw}" = "yes"; then
  AC_DEFINE(HAVE_MS_PW,,MS PW)
  AC_DEFINE(HAVE_MPLS_VC,,MPLS VC)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_MS_PW=yes
  ENABLE_MPLS_VC=yes
  ENABLE_MPLS=yes
fi

dnl ----------------------------------
dnl Psuedo Wires with FEC 129
dnl ----------------------------------
if test "${enable_fec129}" = "yes"; then
  AC_DEFINE(HAVE_FEC129,,FEC 129)
  AC_DEFINE(HAVE_MPLS_VC,,MPLS VC)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_FEC129=yes
  ENABLE_MPLS_VC=yes
  ENABLE_MPLS=yes
fi

dnl ----------------------------------
dnl Virtual Private LAN Service (VPLS)
dnl ----------------------------------
if test "${enable_vpls}" = "yes"; then
  AC_DEFINE(HAVE_VPLS,,VPLS)
  AC_DEFINE(HAVE_MPLS_VC,,MPLS VC)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_VPLS=yes
  ENABLE_MPLS_VC=yes
  ENABLE_MPLS=yes
fi

dnl ----------------------------------
dnl MPLS TDM Virtual Circuit
dnl ----------------------------------
  if test "${enable_mpls_tdm}" = "yes"; then
  AC_DEFINE(HAVE_TDM_VC,,MPLS TDM)
  AC_DEFINE(HAVE_MPLS_VC,,MPLS VC)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_MPLS_TDM_VC=yes
  ENABLE_MPLS_VC=yes
  ENABLE_MPLS=yes
fi

dnl ----------------------------------
dnl MPLS ATM Virtual Circuit
dnl ----------------------------------
  if test "${enable_mpls_atm}" = "yes"; then
  AC_DEFINE(HAVE_ATM_VC,,MPLS ATM)
  AC_DEFINE(HAVE_MPLS_VC,,MPLS VC)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_MPLS_ATM_VC=yes
  ENABLE_MPLS_VC=yes
  ENABLE_MPLS=yes
fi

dnl -----------------------
dnl Enable Graceful restart
dnl -----------------------
if test "${enable_restart}" = "yes"; then
  AC_DEFINE(HAVE_RESTART,,Restart capability)
  ENABLE_RESTART=yes
fi

dnl -----------------------
dnl Enable RSVP Graceful Restart
dnl -----------------------
if test "${enable_rsvp_graceful}" = "yes"; then
  AC_DEFINE(HAVE_RSVP_GRST,,RSVP Graceful Restart capability)
  ENABLE_RSVP_GRACEFUL=yes
fi

dnl ----------------------
dnl Enable License Manager
dnl ----------------------
if test "${enable_licmgr}" = "yes"; then
  AC_DEFINE(HAVE_LICENSE_MGR,,License Manager)
  ENABLE_LICMGR=yes
else
    LICMGRLIB=""
    LICMGRINCLUDE=""
fi

dnl --------
dnl CUSTOM 1
dnl --------
if test "${enable_custom1}" = "yes"; then
  AC_CHECK_HEADER(scl.h,,AC_MSG_ERROR([scl.h is need for CUSTOM1 feature.]))
  AC_CHECK_LIB(scl,scl_init,[LIBS="$LIBS -lscl"],AC_MSG_ERROR([libscl is need need for CUSTOM1 feature.]))
  AC_DEFINE(HAVE_CUSTOM1,,CUSTOM1)
  AC_DEFINE(HAVE_INTERFACE_NAME_MAPPING,,Interface name mapping)
  AC_DEFINE(HAVE_NO_LOCAL_FILESYSTEM,,No local file system)
  ENABLE_CUSTOM1=yes
fi

dnl ----------------
dnl Checkpoint SPLAT
dnl ----------------
if test "${enable_splat}" = "yes"; then
    AC_DEFINE(HAVE_SPLAT,,Enable secure platform on linux)
    ENABLE_SPLAT=yes
  if test "${enable_splat_test}" = "yes"; then
    AC_DEFINE(HAVE_SPLAT_TEST,,Enable secure platform testing)
  fi
fi

dnl --------------
dnl Memory Manager
dnl --------------
if test "${enable_memmgr}" = "no"; then
  ENABLE_MEMMGR=no
else
  ENABLE_MEMMGR=yes
  AC_DEFINE(MEMMGR,,Memory manager enabled)
fi

dnl --------------
dnl IPv4 Multicast
dnl --------------
if test "${enable_mcast_ipv4}" = "yes"; then
    AC_DEFINE(HAVE_MCAST_IPV4,,IPv4 MRIB)
    ENABLE_MCAST_IPV4=yes
else
    ENABLE_MCAST_IPV4=no
fi

dnl -------------------
dnl Check if have VRRP.
dnl -------------------
AC_MSG_CHECKING(VRRP)

if test "${enable_vrrp}" = "yes"; then
  AC_MSG_RESULT(enabled)
  AC_DEFINE(HAVE_VRRP,,VRRP)
  VRRPLIB=../vrrp/libvrrp.a
  VRRP="vrrp"
  ENABLE_VRRP=yes

  if test "${enable_vrrp_link_addr}" = "yes"; then
    AC_DEFINE(HAVE_VRRP_LINK_ADDR,,VRRP Link address)
  fi

else
  AC_MSG_RESULT(disabled)
  VRRPLIB=""
  VRRP=""
fi
AC_SUBST(VRRPLIB)
AC_SUBST(VRRP)

dnl ------------------
dnl Check if have RMM.
dnl ------------------
AC_MSG_CHECKING(RMM)

if test "${enable_rmm}" = "yes"; then
  AC_MSG_RESULT(enabled)
  AC_DEFINE(HAVE_RMM,,RMM)
  RMM="rmm"
  ENABLE_RMM=yes

  if test "${enable_rmm_testprimary}" = "yes"; then
    AC_DEFINE(HAVE_RMM_TEST,,RMM test)
    AC_DEFINE(HAVE_RMM_TEST_SERV,,RMM test server)
    ENABLE_RMM_TESTPRIMARY=yes
  else
    if test "${enable_rmm_testbackup}" = "yes"; then
      AC_DEFINE(HAVE_RMM_TEST,,RMM test)
      AC_DEFINE(HAVE_RMM_TEST_CLIENT,,RMM test client)
      ENABLE_RMM_TESTBACKUP=yes
    fi
  fi
else
  AC_MSG_RESULT(disabled)
  RMM=""
fi
AC_SUBST(RMM)

dnl --------------
dnl IPv4 tunneling
dnl --------------
if test "${enable_ipv4_tunnel}" = yes; then
  ENABLE_TUNNEL=yes
  AC_DEFINE(HAVE_TUNNEL,,IP tunneling)
  AC_DEFINE(HAVE_TUNNEL_IPV4,,IPv4 tunneling)
fi

dnl ---------------
dnl IPv6 transition
dnl ---------------
if test "${enable_ipv6_transit}" = yes && test "${pacos_cv_ipv6}" = yes; then
  ENABLE_TUNNEL=yes
  AC_DEFINE(HAVE_TUNNEL,,IP tunneling)
  AC_DEFINE(HAVE_TUNNEL_IPV6,,IPv6 transition)
fi

dnl ---------------------------------------
dnl Basic Access - DNS, DHCP, NTP, NAT, ACL
dnl ---------------------------------------
AC_MSG_CHECKING(for Basic Access Services)
if test "${enable_basic_access}" = "yes" || test "${enable_sysconf}" = "yes"
then
  AC_MSG_RESULT(enabled)
  ENABLE_BASIC_ACCESS=yes

  AC_MSG_CHECKING(for DNS Client Service)
  if test "${enable_dns_client}" = "no"; then
    AC_MSG_RESULT(disabled)
  else
    AC_MSG_RESULT(enabled)
    AC_DEFINE(HAVE_DNS,,DNS Client Service)
    ENABLE_DNS_CLIENT=yes
  fi

  AC_MSG_CHECKING(for DHCP Client Service)
  if test "${enable_dhcp_client}" = "no"; then
    AC_MSG_RESULT(disabled)
  else
    AC_MSG_RESULT(enabled)
    AC_DEFINE(HAVE_DHCP_CLIENT,,DHCP Client)
    ENABLE_DHCP_CLIENT=yes
  fi

  AC_MSG_CHECKING(for DHCP Server Service)
  if test "${enable_dhcp_server}" = "no"; then
    AC_MSG_RESULT(disabled)
  else
    AC_MSG_RESULT(enabled)
    AC_DEFINE(HAVE_DHCP_SERVER,,DHCP Server)
    ENABLE_DHCP_SERVER=yes

    AC_MSG_CHECKING(for DHCP Server Configuration Update)
    if test "${enable_dhcp_update_style}" = "no"; then
      AC_MSG_RESULT(disabled)
    else
      AC_MSG_RESULT(enabled)
      AC_DEFINE(HAVE_DHCP_UPDATE_STYLE,,DHCP Server Update)
    fi
  fi

  AC_MSG_CHECKING(for PPPoE Client Service)
  if test "${enable_pppoe_client}" = "no"; then
    AC_MSG_RESULT(disabled)
  else
    AC_MSG_RESULT(enabled)
    AC_DEFINE(HAVE_PPPOE,,PPPoE Client)
    ENABLE_PPPOE_CLIENT=yes
  fi

  AC_MSG_CHECKING(for NAT Service)
  if test "${enable_nat}" = "no"; then
    AC_MSG_RESULT(disabled)
  else
    AC_MSG_RESULT(enabled)
    AC_DEFINE(HAVE_NAT,,Network Address Translation)
    ENABLE_NAT=yes
  fi

  AC_MSG_CHECKING(for ACL Service)
  if test "${enable_acl}" = "no"; then
    AC_MSG_RESULT(disabled)
  else
    AC_MSG_RESULT(enabled)
    AC_DEFINE(HAVE_ACL,,Access List)
    ENABLE_ACL=yes
  fi

  AC_MSG_CHECKING(for NTP Service)
  if test "${enable_ntp}" = "no"; then
    AC_MSG_RESULT(disabled)
  else
    AC_MSG_RESULT(enabled)
    AC_DEFINE(HAVE_NTP,,NTP)
    ENABLE_NTP=yes
  fi
else
  AC_MSG_RESULT(disabled)
fi

dnl -----------
dnl Mobile IPv6
dnl -----------
if test "${enable_mip6}" != no && test "${pacos_cv_mip6}" = yes; then
  ENABLE_MIP6=yes
else
  ENABLE_MIP6=no
fi

dnl ------------------
dnl check SNMP library
dnl ------------------
if test "${enable_snmp}" != "no";then
  AC_DEFINE(HAVE_SNMP,,SNMP)
  ENABLE_SNMP="yes"
else
    ENABLE_SNMP="no"
fi

dnl ------------------
dnl check AGENTX library
dnl ------------------
if test "${enable_agentx}" = "yes";then
  AC_DEFINE(HAVE_AGENTX,,AGENTX)
  ENABLE_AGENTX="yes"
else
  ENABLE_AGENTX="no"
fi

dnl ---------------------
dnl Figure out build date
dnl ---------------------
BUILDDATE="`date +%x` `date | awk '{print $4}'`"
AC_DEFINE_UNQUOTED(BUILDDATE, "${BUILDDATE}",Build Number)

dnl -------------
dnl Platform data
dnl -------------
if test "${PACOS_PLATFORM}" != ""; then
  AC_DEFINE_UNQUOTED(PACOS_PLATFORM, "${PACOS_PLATFORM}", Platform)
fi
if test "${PACOS_PLATFORM_ACRONYM}" != ""; then
  AC_DEFINE_UNQUOTED(PACOS_PLATFORM_ACRONYM, "${PACOS_PLATFORM_ACRONYM}", Platform Acronym)
fi
if test "${PACOS_CURR_RELEASE}" != ""; then
  AC_DEFINE_UNQUOTED(PACOS_CURR_RELEASE, "${PACOS_CURR_RELEASE}", Curr Release)
fi

dnl ---------------------------------------------------------
dnl Use static crypto library when license manager is enabled
dnl ---------------------------------------------------------
if test "${ENABLE_LICMGR}" = "yes";then
  if test -f /usr/lib/libcrypto.a; then
     LIBCRYPTO=/usr/lib/libcrypto.a
  elif test -f /usr/local/lib/libcrypto.a; then
     LIBCRYPTO=/usr/local/lib/libcrypto.a
  elif test -f /usr/local/ssl/lib/libcrypto.a; then
     LIBCRYPTO=/usr/local/ssl/lib/libcrypto.a
  fi
  if test "${enable_sun_cc_64_bit}" = "yes"; then
      LIBST=`echo ${LIBS} | sed 's/-lcrypto//g'`
      LIBS="${LIBST} -L/usr/local/ssl/lib/64 -lcrypto"
  fi
  if test -n "${LIBCRYPTO}"; then
      LIBST=`echo ${LIBS} | sed 's/-lcrypto//g'`
      LIBS="${LIBST} ${LIBCRYPTO}"
  fi
fi

dnl -------------
dnl Enable static
dnl -------------
if test "${enable_static}" = "yes"; then
  ENABLE_STATIC=yes
  LIBS="${LIBS} ${LDL}"
fi

dnl --------------------
dnl Daemon disable check
dnl --------------------
if test "${enable_nsm}" = "no";then
  PACOS=""
  ENABLE_NSM=no
else
  PACOS="nsm"
  ENABLE_NSM=yes
  AC_DEFINE(HAVE_NSM,,NSM)

  dnl ------------------------
  dnl NSM Unnumbered Interface
  dnl ------------------------
  AC_MSG_CHECKING(for NSM Unnumbered Interfaces)
  if test "${enable_if_unnumbered}" = "yes"; then
    AC_MSG_RESULT(enabled)
    AC_DEFINE(HAVE_NSM_IF_UNNUMBERED,,NSM Unnumbered Interface)
    ENABLE_NSM_IF_UNNUMBERED=yes
  else
    AC_MSG_RESULT(disabled)
    ENABLE_NSM_IF_UNNUMBERED=no
  fi

  dnl ---------------------
  dnl NSM Interface Arbiter
  dnl ---------------------
  AC_MSG_CHECKING(for NSM Interface Arbiter)
  if test "${enable_if_arbiter}" = "yes"; then
    AC_MSG_RESULT(enabled)
    AC_DEFINE(HAVE_NSM_IF_ARBITER,,NSM Interface Arbiter)
    ENABLE_NSM_IF_ARBITER=yes
  else
    AC_MSG_RESULT(disabled)
    ENABLE_NSM_IF_ARBITER=no
  fi
fi

if test "${enable_bgpd}" = "no";then
  BGPD=""
  ENABLE_BGPD=no
  ENABLE_BGPD_EXTASN=no
  ENABLE_ORDER_NEXTHOP=no
else
  BGPD="bgpd"
  ENABLE_BGPD=yes
  AC_DEFINE(HAVE_BGPD,,bgpd)
dnl check for --disable-bgpd-extasn
dnl turn on by default if not explictly disabled
  if test "${enable_bgpd_extasn}" != "no" ; then
     ENABLE_BGPD_EXTASN=yes
     AC_DEFINE(HAVE_EXT_CAP_ASN)
  fi
dnl check for --disable-order-nexthop
dnl turn on by default if not explicitly disabled
  if test "${enable_order_nexthop}" != "no" ; then
     ENABLE_ORDER_NEXTHOP=yes
     AC_DEFINE(HAVE_ORDER_NEXTHOP,,BGP Order NextHop)
  fi
fi

dnl----------------------------
dnl BGP Extended ASN capability
dnl-----------------------------
if test "${enable_bgpd_extasn}" = "yes";then
    ENABLE_BGPD=yes
    ENABLE_BGPD_EXTASN=yes
    AC_DEFINE(HAVE_BGPD,,bgpd)
    AC_DEFINE(HAVE_EXT_CAP_ASN,,BGP extasn)
fi

AC_SUBST(BGPD_EXTASN)

dnl------------------
dnl BGP Order NextHop
dnl------------------
if test "${enable_order_nexthop}" = "yes" ; then
   ENABLE_BGPD=yes
   ENABLE_ORDER_NEXTHOP=yes
   AC_DEFINE(HAVE_BGPD,,bgpd)
   AC_DEFINE(HAVE_ORDER_NEXTHOP,,BGP Order NextHop)
fi
if test "${enable_order_nexthop}" = "no" ; then
   ENABLE_ORDER_NEXTHOP=no
fi

AC_SUBST(ORDER_NEXTHOP)

dnl ----------------------------
ENABLE_ONMD=no
ENABLE_CFM=no
ENABLE_LLDP=no
ENABLE_EFM=no
ENABLE_G8031=no
ENABLE_G8032=no
ONMD=""

if test "${enable_onmd}" = "yes"; then
   ENABLE_ONMD=yes
   AC_DEFINE(HAVE_ONMD,,onmd)
   ONMD="onmd"
fi

if test "${enable_cfm}" = "yes"; then
   ENABLE_ONMD=yes
   ENABLE_CFM=yes
   ENABLE_LLDP=yes
   AC_DEFINE(HAVE_ONMD,,onmd)
   AC_DEFINE(HAVE_CFM,,cfm)
   AC_DEFINE(HAVE_LLDP,,lldp)
   ONMD="onmd"
fi

if test "${enable_lldp}" = "yes"; then
   ENABLE_ONMD=yes
   ENABLE_LLDP=yes
   AC_DEFINE(HAVE_ONMD,,onmd)
   AC_DEFINE(HAVE_LLDP,,lldp)
   ONMD="onmd"
fi

if test "${enable_efm}" = "yes"; then
   ENABLE_ONMD=yes
   ENABLE_EFM=yes
   AC_DEFINE(HAVE_ONMD,,onmd)
   AC_DEFINE(HAVE_EFM,,cfm)
   ONMD="onmd"
fi

if test "${enable_g8031}" = "yes"; then
   ENABLE_ONMD=yes
   ENABLE_CFM=yes
   ENABLE_G8031=yes
   AC_DEFINE(HAVE_ONMD,,onmd)
   AC_DEFINE(HAVE_CFM,,cfm)
   AC_DEFINE(HAVE_G8031,,g8031)
   ONMD="onmd"
fi

if test "${enable_g8032}" = "yes"; then
   ENABLE_ONMD=yes
   ENABLE_CFM=yes
   ENABLE_G8032=yes
   AC_DEFINE(HAVE_ONMD,,onmd)
   AC_DEFINE(HAVE_CFM,,cfm)
   AC_DEFINE(HAVE_G8032,,g8032)
   ONMD="onmd"
fi

if test "${ENABLE_CFM}" = "yes" ; then
  if test "${enable_cfm_Y1731}" = "yes"; then
   AC_DEFINE(HAVE_CFM_Y1731,,cfm)
   ENABLE_CFM_Y1731=yes
  fi
fi
AC_SUBST(ONMD)


if test "${enable_ripd}" = "no";then
  RIPD=""
  ENABLE_RIPD=no
else
  RIPD="ripd"
  ENABLE_RIPD=yes
  AC_DEFINE(HAVE_RIPD,,ripd)
fi

if test "${enable_ospfd}" = "no";then
  OSPFD=""
  ENABLE_OSPFD=no
else
  OSPFD="ospfd"
  ENABLE_OSPFD=yes
  AC_DEFINE(HAVE_OSPFD,,ospfd)
fi

if test "${enable_bfd}" = "no" && test "${enable_mpls_oam}" != "yes";then
  BFD=""
else
  BFD="oamd"
fi

if test "${enable_bfd}" = "no"; then
  ENABLE_BFD=no
else 
  ENABLE_BFD=yes
if test "${enable_bfd_mono}" = "yes" ; then
  ENABLE_BFD_MONO=yes
  AC_DEFINE(HAVE_BFD_MONO,,bfd)
else
  ENABLE_BFD_MONO=no
fi
  AC_DEFINE(HAVE_BFD,,bfd)
fi
if test "${enable_bfd_main}" = "yes" ; then
  ENABLE_BFD_MAIN=yes
  AC_DEFINE(HAVE_BFD_MAIN,,bfd)
fi
if test "${enable_bfd_line}" = "yes" ; then
  ENABLE_BFD_LINE=yes
  AC_DEFINE(HAVE_BFD_LINE,,bfd)
fi

if test "${enable_mpls_oam}" = "yes"; then
  ENABLE_MPLS_OAM=yes
  AC_DEFINE(HAVE_MPLS_OAM,,mpls-oam)
else
  ENABLE_MPLS_OAM=no
fi

if test "${enable_stpd}" = "yes";then
  ENABLE_STPD=yes
  AC_DEFINE(HAVE_STPD,,stpd)
  STPD="stpd"
else
  ENABLE_STPD=no
  STPD=""
fi
AC_SUBST(STPD)

if test "${enable_rstpd}" = "yes";then
  ENABLE_RSTPD=yes
  AC_DEFINE(HAVE_RSTPD,,rstpd)
  RSTPD="rstpd"
else
  ENABLE_RSTPD=no
  RSTPD=""
fi
AC_SUBST(RSTPD)

if test "${enable_authd}" = "yes";then
  ENABLE_AUTHD=yes
  AC_DEFINE(HAVE_AUTHD,,authd)
  AUTHD="auth"
else
  ENABLE_AUTHD=no
  AUTHD=""
fi
AC_SUBST(AUTHD)

if test "${enable_lacpd}" = "yes";then
  ENABLE_LACPD=yes
  AC_DEFINE(HAVE_LACPD,,lacpd)
  LACPD="lacpd"
else
  ENABLE_LACPD=no
  LACPD=""
fi
AC_SUBST(LACPD)


if test "${enable_intervlan_routing}" = "yes";then
  AC_DEFINE(HAVE_INTERVLAN_ROUTING,,intervlan_routing)
fi

if test "${enable_vlan}" = "yes";then
  ENABLE_VLAN=yes
  VLAN="vlan"
  AC_DEFINE(HAVE_VLAN,,vlan)
else
  ENABLE_VLAN=no
  VLAN=""
fi
AC_SUBST(VLAN)

if test "${enable_pvlan}" = "yes";then
  ENABLE_PVLAN=yes
  ENABLE_VLAN=yes
  PVLAN="pvlan"
  VLAN="vlan"
  AC_DEFINE(HAVE_PVLAN,,pvlan)
  AC_DEFINE(HAVE_VLAN,,vlan)
else
  ENABLE_PVLAN=no
  PVLAN=""
fi
AC_SUBST(PVLAN)

if test "${enable_ipsec}" = "yes";then
  ENABLE_IPSEC=yes
  IPSEC="ipsec"
  AC_DEFINE(HAVE_IPSEC,,ipsec)
else
  ENABLE_IPSEC=no
  IPSEC=""
fi
AC_SUBST(IPSEC)

if test "${enable_firewall}" = "yes";then
  ENABLE_FIREWALL=yes
  FIREWALL="firewall"
  AC_DEFINE(HAVE_FIREWALL,,firewall)
else
  ENABLE_FIREWALL=no
  FIREWALL=""
fi
AC_SUBST(FIREWALL)

if test "${enable_vlan_class}" = "yes";then
  ENABLE_VLAN_CLASS=yes
  ENABLE_VLAN=yes
  VLAN_CLASS="vlan_class"
  VLAN="vlan"
  AC_DEFINE(HAVE_VLAN_CLASS,,vlan_class)
  AC_DEFINE(HAVE_VLAN,,vlan)
else
  ENABLE_VLAN_CLASS=no
  VLAN_CLASS=""
fi
AC_SUBST(VLAN_CLASS)

if test "${enable_vlan_stack}" = "yes";then
  ENABLE_VLAN_STACK=yes
  ENABLE_VLAN=yes
  VLAN_STACK="vlan_stack"
  VLAN="vlan"
  AC_DEFINE(HAVE_VLAN_STACK,,vlan_stack)
  AC_DEFINE(HAVE_VLAN,,vlan)
else
  ENABLE_VLAN_STACK=no
  VLAN_STACK=""
fi
AC_SUBST(VLAN_STACK)

if test "${enable_rate_limit}" = "yes";then
  ENABLE_RATE_LIMIT=yes
  RATE_LIMIT="rate_limit"
  AC_DEFINE(HAVE_RATE_LIMIT,,rate_limit)
else
  ENABLE_RATE_LIMIT=no
  RATE_LIMIT=""
fi
AC_SUBST(RATE_LIMIT)

if test "${enable_igmp_snoop}" = "yes";then
  ENABLE_IGMP_SNOOP=yes
  IGMP_SNOOP="igmp_snoop"
  AC_DEFINE(HAVE_IGMP_SNOOP,,igmp_snoop)
else
  ENABLE_IGMP_SNOOP=no
  IGMP_SNOOP=""
fi
AC_SUBST(IGMP_SNOOP)

if test "${enable_mld_snoop}" = "yes";then
  ENABLE_MLD_SNOOP=yes
  MLD_SNOOP="mld_snoop"
  AC_DEFINE(HAVE_MLD_SNOOP,,mld_snoop)
else
  ENABLE_MLD_SNOOP=no
  MLD_SNOOP=""
fi
AC_SUBST(MLD_SNOOP)

if test "${enable_mld_default_no_snoop}" = "yes";then
  DISABLE_DEFAULT_MLD_SNOOP=yes
  MLD_DEAFULT_SNOOP="mld_default_snoop"
  AC_DEFINE(HAVE_DISABLE_MLD_SNOOP,,mld_default_snoop)
else
  DISABLE_DEFAULT_MLD_SNOOP=no
  MLD_DEAFULT_SNOOP=""
fi
AC_SUBST(DISABLE_DEFAULT_IGMP_SNOOP)

if test "${enable_dcb}" = "yes";then
  ENABLE_DCB=yes
  DCB="dcb"
  AC_DEFINE(HAVE_DCB,,dcb)
else
  ENABLE_DCB=no
  DCB=""
fi
AC_SUBST(DCB)

if test "${enable_mac_auth}" = "yes";then
  ENABLE_MAC_AUTH=yes
  MAC_AUTH="mac_auth"
  AC_DEFINE(HAVE_MAC_AUTH,,mac_auth)
else
  ENABLE_MAC_AUTH=no
  MAC_AUTH=""
fi
AC_SUBST(MAC_AUTH)

if test "${enable_igmp_v3}" = "no";then
  IGMP_V3=""
  ENABLE_IGMP_V3=no
else
  IGMP_V3="igmp_v3"
  ENABLE_IGMP_V3=yes
  AC_DEFINE(HAVE_IGMP_V3,,igmp_v3)
fi

if test "${enable_smi}" = "yes";then
  ENABLE_SMI=yes
  AC_DEFINE(HAVE_SMI,,smi)
  LIBS="${LIBS} -lpthread"
else
  ENABLE_SMI=no
fi

if test "${with_pne37}" = "yes";then
  ENABLE_PNE_3_7=yes
  PNE_3_7="PNE_3_7"
else
  ENABLE_PNE_3_7=no
  PNE_3_7=""
fi
AC_SUBST(PNE_3_7)


dnl --------------------
dnl Check if target has disc/compact flash
dnl --------------------
if test "${enable_storage_dev}" = "no";then
  ENABLE_STORAGE_DEV=no
  STORAGE_DEV=""
  AC_DEFINE(HAVE_NO_STORAGE_DEV,,storage-dev)
else
  STORAGE_DEV="STORAGE_DEV"
  ENABLE_STORAGE_DEV=yes
fi
AC_SUBST(STORAGE_DEV)


if test "${enable_hal}" = "yes";then
  ENABLE_HAL=yes
  HAL="HAL"
  AC_DEFINE(HAVE_HAL,,hal)
else
  ENABLE_HAL=no
  HAL=""
fi
AC_SUBST(HAL)

if test "${enable_axelx}" = "yes" ; then
   ENABLE_AXELX=yes
   AXELX="AXELX"
   AC_DEFINE(HAVE_AXELX,,axelx)
else
   ENABLE_AXELX=no
   AXELX=""
fi
AC_SUBST(AXELX)

if test "${enable_user_hsl}" = "yes" ; then
   ENABLE_USER_HSL=yes
   ENABLE_USER_HSL=yes
   HAVE_USER_HSL="HAVE_USER_HSL"
   AC_DEFINE(HAVE_USER_HSL,,user-hsl)
else
   USER_HSL=no
   USER_HSL=""
fi
AC_SUBST(USER_HSL)

dnl -- enable broadcom 
dnl -- allow user to override board default 
dnl -- GTO (8548 cpu) boards default for 2.6.27 kernel
dnl -- BMW (824x cpu) boards default for 2.6.14 kernel

case "${enable_broadcom}" in
dnl -- configure for broadcom
     yes|bmw|gto)
	ENABLE_BROADCOM=yes
	BROADCOM="BROADCOM"
	AC_DEFINE(HAVE_BROADCOM,,broadcom)
	dnl -- See if picked specific board
	case "${enable_broadcom}" in
	     dnl -- picked GTO
	     gto)
		AC_DEFINE(HAVE_GTO,,Broadcom GTO board)
		;;
	     dnl -- Check kernel version to select board
	     dnl -- Select GTO if kernel is 2.6.27
	     yes)
		AC_MSG_CHECKING(Kernel version is)
		if test "$LINUX_VERSION_MINOR" = "27"
		then
			AC_MSG_RESULT(2.6.$LINUX_VERSION_MINOR)
			AC_DEFINE(HAVE_GTO,,Broadcom GTO board)
		fi
		;;
	esac	
	;;

dnl -- do not configure broadcom
     no)
	ENABLE_BORADCOM=no
	BROADCOM=""
	;;
esac
AC_SUBST(BROADCOM)

if test "${enable_marvell}" = "yes";then
  ENABLE_MARVELL=yes
  MARVELL="MARVELL"
  AC_DEFINE(HAVE_MARVELL,,marvell)
else
  ENABLE_MARVELL=no
  MARVELL=""
fi
AC_SUBST(MARVELL)

if test "${enable_marvell_ls}" = "yes";then
  ENABLE_MARVELL_LS=yes
  MARVELL_LS="MARVELL_LS"
  AC_DEFINE(HAVE_PPC_MARVELL,,ppc_marvell)
else
  ENABLE_MARVELL_LS=no
  MARVELL_LS=""
fi
AC_SUBST(MARVELL_LS)

case "${enable_intel}" in
  yes)
    echo "Please specify CP-PDK path to --enable-intel=ARG"
    exit 1
    ;;
  "")
    ENABLE_INTEL=no
    ;;
  *)
    ENABLE_INTEL=yes
    INTEL_CPPDK_PATH="${enable_intel}"
    AC_DEFINE(HAVE_INTEL,,intel)

    INTEL_PDK_LIB_PIL=no
    AC_CHECK_LIB(pil, PIL_Init, [INTEL_PDK_LIB_PIL=yes])
    if test "$INTEL_PDK_LIB_PIL" = "no";then
      AC_MSG_ERROR([Could not find libpil.so in the LD path. This shared library is typically present in $INTEL_CPPDK_PATH/common/pil/lib/ia_linux. One solution is to copy the .so to /usr/lib. Please correct this and try again.])
    fi
    LIBS="-lpil $LIBS"
esac

if test "${enable_default_bridge}" = "yes";then
  ENABLE_DEFAULT_BRIDGE=yes
  DEFAULT_BRIDGE="default-bridge"
  AC_DEFINE(HAVE_DEFAULT_BRIDGE,,default-bridge)
else
  ENABLE_DEFAULT_BRIDGE=no
  DEFAULT_BRIDGE=""
fi
AC_SUBST(DEFAULT_BRIDGE)

if test "${enable_provider_bridge}" = "yes";then
  ENABLE_PROVIDER_BRIDGE=yes
  PROVIDER_BRIDGE="provider-bridge"
  AC_DEFINE(HAVE_PROVIDER_BRIDGE,,provider-bridge)
else
  ENABLE_PROVIDER_BRIDGE=no
  PROVIDER_BRIDGE=""
fi
AC_SUBST(PROVIDER_BRIDGE)

if test "${enable_elmid}" = "yes"; then
  ENABLE_ELMID=yes
  AC_DEFINE(HAVE_ELMID,,elmid)
  ELMID="elmid"
else
  ENABLE_ELMID=no
  ELMID=""
fi
AC_SUBST(ELMID)

if test "${enable_i_pbb}" = "yes";then
  ENABLE_I_BEB=yes
  I_BEB="i-pbb"
  AC_DEFINE(HAVE_I_BEB,,i-pbb)
else
  ENABLE_I_BEB=no
  I_BEB=""
fi
AC_SUBST(I_BEB)

if test "${enable_b_pbb}" = "yes";then
  ENABLE_B_BEB=yes
  B_BEB="b-pbb"
  AC_DEFINE(HAVE_B_BEB,,b-pbb)
else
  ENABLE_B_BEB=no
  B_BEB=""
fi
AC_SUBST(B_BEB)

if test "${enable_pbb_te}" = "yes";then
  ENABLE_PBB_TE=yes
  PBB_TE="pbb-te"
  AC_DEFINE(HAVE_PBB_TE,,pbb-te)
else
  ENABLE_PBB_TE=no
  PBB_TE=""
fi
AC_SUBST(PBB_TE)

if test "${enable_gmrp}" = "yes";then
  ENABLE_GMRP=yes
  GMRP="gmrp"
  AC_DEFINE(HAVE_GMRP,,gmrp)
else
  ENABLE_GMRP=no
  GMRP=""
fi
AC_SUBST(GMRP)

if test "${enable_mmrp}" = "yes";then
  ENABLE_MMRP=yes
  MMRP="mmrp"
  AC_DEFINE(HAVE_MMRP,,mmrp)
else
  ENABLE_MMRP=no
  MMRP=""
fi
AC_SUBST(MMRP)

if test "${enable_gvrp}" = "yes";then
  ENABLE_GVRP=yes
  GVRP="gvrp"
  AC_DEFINE(HAVE_GVRP,,gvrp)
else
  ENABLE_GVRP=no
  GVRP=""
fi
AC_SUBST(GVRP)

if test "${enable_mvrp}" = "yes";then
  ENABLE_MVRP=yes
  MVRP="mvrp"
  AC_DEFINE(HAVE_MVRP,,mmrp)
else
  ENABLE_MVRP=no
  MVRP=""
fi
AC_SUBST(MVRP)

if test "${enable_mstpd}" = "yes";then
  ENABLE_MSTPD=yes
  AC_DEFINE(HAVE_MSTPD,,mstpd)
  MSTPD="mstpd"
else
  ENABLE_MSTPD=no
  MSTPD=""
fi
AC_SUBST(MSTPD)

if test "${enable_rpvst_plus}" = "yes";then
  ENABLE_RPVST_PLUS=yes
  AC_DEFINE(HAVE_RPVST_PLUS,,rpvst-plus)
  RPVST_PLUS="rpvst_plus"
else
  ENABLE_RPVST_PLUS=no
  RPVST_PLUS=""
fi
AC_SUBST(RPVST_PLUS)

if test "${enable_swfwdr}" = "yes";then
  ENABLE_SWFWDR=yes
  AC_DEFINE(HAVE_SWFWDR,,l2swfwdr)
  SWFWDR="l2swfwdr"
else
  ENABLE_SWFWDR=no
fi
AC_SUBST(SWFWDR)

if test "${enable_rmond}" = "yes";then
  ENABLE_RMOND=yes
  AC_DEFINE(HAVE_RMOND,,rmond)
  RMOND="rmond"
else
  ENABLE_RMOND=no
  RMOND=""
fi
AC_SUBST(RMOND)

if test "${enable_ldpd}" = "no";then
  LDPD=""
  ENABLE_LDPD=no
else
  LDPD="ldpd"
  ENABLE_LDPD=yes
  ENABLE_MPLS=yes
  AC_DEFINE(HAVE_LDPD,,ldpd)
  AC_DEFINE(HAVE_MPLS,,MPLS)
fi

if test "${enable_rsvpd}" = "no";then
  RSVPD=""
  ENABLE_RSVPD=no
else
  RSVPD="rsvpd"
  ENABLE_RSVPD=yes
  ENABLE_MPLS=yes
  AC_DEFINE(HAVE_RSVPD,,rsvpd)
  AC_DEFINE(HAVE_MPLS,,MPLS)
fi

if test "${enable_isisd}" = "no";then
  ISISD=""
  ENABLE_ISISD=no
else
  ISISD="isisd"
  ENABLE_ISISD=yes
  AC_DEFINE(HAVE_ISISD,,isisd)
  if test "${enable_multi_topology}" = yes && test "${pacos_cv_ipv6}" = yes
  then
    AC_DEFINE(HAVE_MULTI_TOPOLOGY,,IS-IS Multi topology)
    ENABLE_MULTI_TOPOLOGY=yes
  fi
fi

if test "${enable_pdmd}" = "no";then
  PDMD=""
  ENABLE_PDMD=no
else
  PDMD="pdmd"
  ENABLE_PDMD=yes
  AC_DEFINE(HAVE_PDMD,,pdmd)
fi

if test "${enable_pimd}" = "no";then
  PIMD=""
  ENABLE_PIMD=no
else
  PIMD="pimd"
  ENABLE_PIMD=yes
  AC_DEFINE(HAVE_PIMD,,pimd)
fi

if test "${enable_pim_ssm}" = "no";then
  PIM_SSM=""
  ENABLE_PIM_SSM=no
else
  PIM_SSM="pim_ssm"
  ENABLE_PIM_SSM=yes
  AC_DEFINE(HAVE_PIM_SSM,,pim_ssm)
fi

if test "${enable_pimpktgend}" = "yes";then
  ENABLE_PIMPKTGEND=yes
else
  ENABLE_PIMPKTGEND=no
fi

if test "${enable_dvmrpd}" = "no";then
  DVMRPD=""
  ENABLE_DVMRPD=no
else
  DVMRPD="dvmrpd"
  ENABLE_DVMRPD=yes
  AC_DEFINE(HAVE_DVMRPD,,dvmrpd)
fi

if test "${enable_dvmrp_ssm}" = "no";then
  DVMRP_SSM=""
  ENABLE_DVMRP_SSM=no
else
  DVMRP_SSM="dvmrp_ssm"
  ENABLE_DVMRP_SSM=yes
  AC_DEFINE(HAVE_DVMRP_SSM,,dvmrp_ssm)
fi

if test "${enable_te}" = "yes";then
  ENABLE_MPLS=yes
  AC_DEFINE(HAVE_TE,,Traffic Engineering)
  AC_DEFINE(HAVE_OSPF_TE,,OSPF TE)
  if test "${enable_opaque_lsa}" = "no"; then
    AC_MSG_ERROR([Opaque-lsa should be enabled if you want to use OSPF-TE])
  fi
  AC_DEFINE(HAVE_OPAQUE_LSA,,OSPF Opaque LSA)
  AC_DEFINE(HAVE_OSPF6_TE,,OSPF6 TE)
  AC_DEFINE(HAVE_ISIS_TE,,ISIS TE)
  AC_DEFINE(HAVE_MPLS,,MPLS)
fi

if test "${enable_gmpls}" = "yes";then
  AC_DEFINE(HAVE_GMPLS,,GMPLS)
  AC_DEFINE(HAVE_TE,,Traffic Engineering)
  AC_DEFINE(HAVE_OSPF_TE,,OSPF TE)
  AC_DEFINE(HAVE_PACKET,,Packet)
  if test "${enable_opaque_lsa}" = "no"; then
    AC_MSG_ERROR([Opaque-lsa should be enabled if you want to use OSPF-TE])
  fi
  AC_DEFINE(HAVE_OPAQUE_LSA,,OSPF Opaque LSA)
  AC_DEFINE(HAVE_ISIS_TE,,ISIS TE)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_GMPLS=yes
  ENABLE_MPLS=yes
  ENABLE_PACKET=yes
fi

if test "${enable_diffserv}" = "yes"; then
  AC_DEFINE(HAVE_DIFFSERV,,Diffserv)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  ENABLE_DIFFSERV=yes
  ENABLE_MPLS=yes
fi

if test "${enable_dste}" = "yes"; then
  AC_DEFINE(HAVE_DSTE,,DSTE)
  AC_DEFINE(HAVE_MPLS,,MPLS)
  AC_DEFINE(HAVE_TE,,TE)
  ENABLE_DSTE=yes
  ENABLE_MPLS=yes
  ENABLE_TE=yes
fi

if test "${enable_pim6d}" = "no" || test "${pacos_cv_ipv6}" != "yes";then
  PIM6D=""
  ENABLE_PIM6D=no
else
  PIM6D="pim6d"
  ENABLE_PIM6D=yes
  AC_DEFINE(HAVE_PIM6D,,pim6d)
fi

if test "${enable_pim6d_ssm}" = "no" || test "${pacos_cv_ipv6}" != "yes";then
  PIM6_SSM=""
  ENABLE_PIM6_SSM=no
else
  PIM6_SSM="pim6_ssm"
  ENABLE_PIM6_SSM=yes
  AC_DEFINE(HAVE_PIM6_SSM,,pim6_ssm)
fi

if test "${enable_mcast_ipv6}" != "yes" || test "${pacos_cv_ipv6}" != "yes";then
  ENABLE_MCAST_IPV6=no
else
  ENABLE_MCAST_IPV6=yes
  AC_DEFINE(HAVE_MCAST_IPV6,,IPv6 MRIB)
fi

if test "${enable_mcast4_intf_limit}" = "no";then
  MCAST4_LIMIT=""
else
  MCAST4_LIMIT="IPv4 Multicast Interface Limit"
  AC_DEFINE(HAVE_IPV4_MCAST_INTF_LIMIT,,MCAST4_LIMIT)
fi

if test "${enable_mcast4_intf_limit_ignore}" = "yes";then
  MCAST4_LIMIT_IGNORE="Ignore IPv4 Multicast Interface Limit"
  AC_DEFINE(IGNORE_IPV4_MCAST_INTF_LIMIT,,MCAST4_LIMIT_IGNORE)
else
  MCAST4_LIMIT_IGNORE=""
fi

if test "${enable_mcast6_intf_limit}" = "no";then
  MCAST6_LIMIT=""
else
  MCAST6_LIMIT="IPv6 Multicast Interface Limit"
  AC_DEFINE(HAVE_IPV6_MCAST_INTF_LIMIT,,MCAST6_LIMIT)
fi

if test "${enable_mcast6_intf_limit_ignore}" = "yes";then
  MCAST6_LIMIT_IGNORE="Ignore IPv6 Multicast Interface Limit"
  AC_DEFINE(IGNORE_IPV6_MCAST_INTF_LIMIT,,MCAST6_LIMIT_IGNORE)
else
  MCAST4_LIMIT_IGNORE=""
fi

if test "${enable_ripngd}" = "no" || test "${pacos_cv_ipv6}" != "yes";then
  RIPNGD=""
  ENABLE_RIPNGD=no
else
  RIPNGD="ripngd"
  ENABLE_RIPNGD=yes
  AC_DEFINE(HAVE_RIPNGD,,ripngd)
fi

if test "${enable_ospf6d}" = "no" || test "${pacos_cv_ipv6}" != "yes";then
  OSPF6D=""
  ENABLE_OSPF6D=no
else
  OSPF6D="ospf6d"
  ENABLE_OSPF6D=yes
  AC_DEFINE(HAVE_OSPF6D,,ospf6d)
fi

if test "${ENABLE_OSPFD}" != "yes" || test "${ENABLE_OSPF6D}" != "yes";then
  ENABLE_OSPF_COMMON=no
else
  ENABLE_OSPF_COMMON=yes
  AC_DEFINE(HAVE_OSPF_COMMON,,OSPF common library)
fi

if test "${enable_bgp_rt2fib}" = "no";then
  AC_DEFINE(DISABLE_BGP_RT2FIB,,Do not install BGP routes in NSM)
fi

if test "${enable_bgp_dump}" = "yes"; then
  AC_DEFINE(HAVE_BGP_DUMP,,BGP Dump)
fi

if test "${enable_agentx_unix_domain}" = "yes";then
  ENABLE_AGENTX_UNIX_DOMAIN=yes
  AC_DEFINE(HAVE_AGENTX_UNIX_DOMAIN,,Agentx Unix Domain)
else
  ENABLE_AGENTX_UNIX_DOMAIN=no
fi

AC_SUBST(VRRP)
AC_SUBST(PACOS)
AC_SUBST(BGPD)
AC_SUBST(ONMD)
AC_SUBST(ELMID)
AC_SUBST(RIPD)
AC_SUBST(RIPNGD)
AC_SUBST(OSPFD)
AC_SUBST(OSPF6D)
AC_SUBST(INCLUDES)
AC_SUBST(CURSES)
AC_SUBST(MPLS)
AC_SUBST(PBR)
AC_SUBST(LDPD)
AC_SUBST(RSVPD)
AC_SUBST(ISISD)
AC_SUBST(PDMD)
AC_SUBST(PIMD)
AC_SUBST(PIM6D)
AC_SUBST(DVMRPD)
AC_SUBST(AIO)
AC_SUBST(STPD)
AC_SUBST(RSTPD)
AC_SUBST(AUTHD)
AC_SUBST(LACPD)
AC_SUBST(MSTPD)
AC_SUBST(SWFWDR)
AC_SUBST(RMOND)
AC_SUBST(MPLSONM)

dnl -------------
dnl check version
dnl -------------
file="../src/lib/version.h"
VERSION=`sed -ne 's/^#.*PACOS_VERSION.*\"\([^\"]*\)\"$/\1/p' $file`
AC_SUBST(VERSION)

dnl ------------------------------
dnl set paths for process id files
dnl ------------------------------
AC_CACHE_CHECK(pid file directory,ac_piddir,
[for PACOS_PID_DIR in /var/run  dnl
                   /var/adm     dnl
                   /etc         dnl
                   /dev/null;
do
  test -d $PACOS_PID_DIR && break
done
ac_piddir=$PACOS_PID_DIR

if test $PACOS_PID_DIR = "/dev/null"; then
  echo "PID DIRECTORY NOT FOUND!"
fi])
AC_DEFINE(HAVE_PID,,PID)
AC_DEFINE_UNQUOTED(PATH_STPD_PID, "$ac_piddir/stp.pid",STPD PID)
AC_DEFINE_UNQUOTED(PATH_RSTPD_PID, "$ac_piddir/rstp.pid",RSTPD PID)
AC_DEFINE_UNQUOTED(PATH_MSTPD_PID, "$ac_piddir/mstp.pid",MSTPD PID)
AC_DEFINE_UNQUOTED(PATH_AUTHD_PID, "$ac_piddir/auth.pid",AUTHD PID)
AC_DEFINE_UNQUOTED(PATH_LACPD_PID, "$ac_piddir/lacp.pid",LACPD PID)
AC_DEFINE_UNQUOTED(PATH_ELMID_PID, "$ac_piddir/elmi.pid",ELMID PID)
AC_DEFINE_UNQUOTED(PATH_NSM_PID, "$ac_piddir/nsm.pid",NSM PID)
AC_DEFINE_UNQUOTED(PATH_ONMD_PID, "$ac_piddir/onm.pid",ONMD PID)
AC_DEFINE_UNQUOTED(PATH_RIPD_PID, "$ac_piddir/ripd.pid",ripd PID)
AC_DEFINE_UNQUOTED(PATH_RIPNGD_PID, "$ac_piddir/ripngd.pid",ripngd PID)
AC_DEFINE_UNQUOTED(PATH_BGPD_PID, "$ac_piddir/bgpd.pid",bgpd PID)
AC_DEFINE_UNQUOTED(PATH_OSPFD_PID, "$ac_piddir/ospfd.pid",ospfd PID)
AC_DEFINE_UNQUOTED(PATH_OSPF6D_PID, "$ac_piddir/ospf6d.pid",ospf6d PID)
AC_DEFINE_UNQUOTED(PATH_ISISD_PID, "$ac_piddir/isisd.pid",isisd PID)
AC_DEFINE_UNQUOTED(PATH_PDMD_PID, "$ac_piddir/pdmd.pid",pdmd PID)
AC_DEFINE_UNQUOTED(PATH_PIMD_PID, "$ac_piddir/pimd.pid",pimd PID)
AC_DEFINE_UNQUOTED(PATH_DVMRPD_PID, "$ac_piddir/dvmrpd.pid",dvmrpd PID)
AC_DEFINE_UNQUOTED(PATH_PIM6D_PID, "$ac_piddir/pim6d.pid",pim6d PID)
AC_DEFINE_UNQUOTED(PATH_LDPD_PID, "$ac_piddir/ldpd.pid",ldpd PID)
AC_DEFINE_UNQUOTED(PATH_RSVPD_PID, "$ac_piddir/rsvpd.pid",rsvpd PID)
AC_DEFINE_UNQUOTED(PATH_IMI_PID, "$ac_piddir/imi.pid",imi PID)
AC_DEFINE_UNQUOTED(PATH_RMOND_PID, "$ac_piddir/rmon.pid",RMOND PID)
AC_DEFINE_UNQUOTED(PATH_MPLSONM_PID, "$ac_piddir/mplsonm.pid",MPLSONM PID)
AC_DEFINE_UNQUOTED(PATH_BFD_PID, "$ac_piddir/oam.pid",BFD PID)
AC_DEFINE_UNQUOTED(PATH_VLOGD_PID, "$ac_piddir/vlogd.pid",VLOGD PID)
AC_DEFINE_UNQUOTED(PATH_LMPD_PID, "$ac_piddir/lmpd.pid",LMPD PID)

dnl ---------------------------
dnl Check htonl works correctly
dnl ---------------------------
AC_MSG_CHECKING(for working htonl)
AC_CACHE_VAL(ac_cv_htonl_works, [
AC_TRY_LINK([#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif],
[htonl (0);],
ac_cv_htonl_works=yes,
ac_cv_htonl_works=no)])
AC_MSG_RESULT($ac_cv_htonl_works)



dnl ---------------------------------
dnl Creating options in Rules.options
dnl ---------------------------------

echo "creating platform options file"

if test "${IAPN_PACOS_PLATFORM}" = "dummy";then
echo "LIBS=" >> $OPTIONS_FILE
echo "EXTRA_CFLAGS=" >> $OPTIONS_FILE
else
echo "LIBS=${LIBS}" >> $OPTIONS_FILE
echo "EXTRA_CFLAGS=${CFLAGS}" >> $OPTIONS_FILE
fi

if test "$ENABLE_EFENCE" = "yes";then
  echo "EFENCE_LIB=${EFENCE_LIB}" >> $OPTIONS_FILE
fi

if test "${prefix}" != "NONE" ; then
  echo "PREFIX=${prefix}" >> $OPTIONS_FILE
  AC_DEFINE_UNQUOTED(PATH_PREFIX, "${prefix}", "")
else
  echo "PREFIX=/usr/local" >> $OPTIONS_FILE
  AC_DEFINE_UNQUOTED(PATH_PREFIX, "/usr/local", "")
fi

if test "${sysconfdir}" != "NONE" ; then
  if test "${sysconfdir}" = \$\{prefix\}/etc ; then
    if test "${prefix}" != "NONE" ; then
      echo "SYSCONFDIR=${prefix}/etc" >> $OPTIONS_FILE
      AC_DEFINE_UNQUOTED(PATH_SYSCONFDIR, "${prefix}/etc", "")
    else
      echo "SYSCONFDIR=/usr/local/etc" >> $OPTIONS_FILE
      AC_DEFINE_UNQUOTED(PATH_SYSCONFDIR, "/usr/local/etc", "")
    fi
  else
    echo "SYSCONFDIR=${sysconfdir}" >> $OPTIONS_FILE
    AC_DEFINE_UNQUOTED(PATH_SYSCONFDIR, "${sysconfdir}", "")
  fi
else
  echo "SYSCONFDIR=/usr/local/etc" >> $OPTIONS_FILE
  AC_DEFINE_UNQUOTED(PATH_SYSCONFDIR, "/usr/local/etc", "")
fi

if test "$COMPILER_DIR" != "" ;  then
   echo "COMPILER_DIR=$COMPILER_DIR" >> $OPTIONS_FILE
fi

if test "$COMPILER_PREFIX" != "" ;  then
   echo "COMPILER_PREFIX=$COMPILER_PREFIX" >> $OPTIONS_FILE
fi

if test "$KERNEL_SOURCE" != "" ;  then
   echo "KERNEL_SOURCE=$KERNEL_SOURCE" >> $OPTIONS_FILE
fi

dnl ----------------------------------------
dnl Set L2 if anyone of L2 protocols enabled
dnl ----------------------------------------
if test "${ENABLE_STPD}" = "yes" || test "${ENABLE_RSTPD}" = "yes" || test "${ENABLE_MSTPD}" = "yes" || test "${ENABLE_LACPD}" = "yes" || test "${ENABLE_AUTHD}" = "yes" || test "${ENABLE_IGMP_SNOOP}" = "yes" ||  test "${ENABLE_RPVST_PLUS}" = "yes" ||  test "${ENABLE_ELMID}" = "yes" || test "${ENABLE_DCB}" = "yes"; then
 ENABLE_L2=yes
 echo "ENABLE_L2=yes" >> $OPTIONS_FILE
 AC_DEFINE(HAVE_L2,,Layer 2)
else
 ENABLE_L2=no
 echo "ENABLE_L2=no" >> $OPTIONS_FILE
fi

dnl ----------------------------------------
dnl Set L3 if anyone of L3 protocols enabled
dnl ----------------------------------------
if test "${ENABLE_BFD}" = "yes" || test "${ENABLE_RIPD}" = "yes" || test "${ENABLE_OSPFD}" = "yes" || test "${ENABLE_RIPNGD}" = "yes" || test "${ENABLE_OSPF6D}" = "yes" || test "${ENABLE_LDPD}" = "yes" || test "${ENABLE_RSVPD}" = "yes" || test "${ENABLE_PIMD}" = "yes" || test "${ENABLE_PIM6D}" = "yes" || test "${ENABLE_BGPD}" = "yes" || test "${ENABLE_PDMD}" = "yes" || test "${ENABLE_ISISD}" = "yes" || test "${ENABLE_VRRP}" = "yes" || test "${ENABLE_DVMRPD}" = "yes" || test "${ENABLE_MCAST_IPV4}" = "yes" || test "${ENABLE_MCAST_IPV6}" = "yes" ;then
 ENABLE_L3=yes
 echo "ENABLE_L3=yes" >> $OPTIONS_FILE
 AC_DEFINE(HAVE_L3,,Layer 3)
else
 ENABLE_L3=no
 echo "ENABLE_L3=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_IMI}" = "yes";then
   echo "ENABLE_IMI=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_IMI=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_IMISH}" = "yes";then
   echo "ENABLE_IMISH=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_IMISH=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_SPLAT" = "yes"; then
   echo "ENABLE_SPLAT=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_SPLAT=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_BASIC_ACCESS}" = "yes"; then
   echo "ENABLE_BASIC_ACCESS=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_BASIC_ACCESS=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_DNS_CLIENT}" = "yes"; then
   echo "ENABLE_DNS_CLIENT=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_DNS_CLIENT=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_DHCP_CLIENT}" = "yes"; then
   echo "ENABLE_DHCP_CLIENT=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_DHCP_CLIENT=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_DHCP_SERVER}" = "yes"; then
   echo "ENABLE_DHCP_SERVER=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_DHCP_SERVER=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_PPPOE_CLIENT}" = "yes"; then
   echo "ENABLE_PPPOE_CLIENT=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PPPOE_CLIENT=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_NAT}" = "yes"; then
   echo "ENABLE_NAT=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_NAT=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_ACL}" = "yes"; then
   echo "ENABLE_ACL=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_ACL=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_NTP}" = "yes"; then
   echo "ENABLE_NTP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_NTP=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_IPV6}" = "yes";then
   echo "ENABLE_IPV6=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_IPV6=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_RTADV}" = "yes";then
   echo "ENABLE_RTADV=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_RTADV=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_MIP6}" = "yes";then
   echo "ENABLE_MIP6=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MIP6=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_NSM}" = "yes";then
   echo "ENABLE_NSM=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_NSM=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_NSM_IF_UNNUMBERED}" = "yes"; then
   echo "ENABLE_NSM_IF_UNNUMBERED=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_NSM_IF_UNNUMBERED=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_NSM_IF_ARBITER}" = "yes"; then
   echo "ENABLE_NSM_IF_ARBITER=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_NSM_IF_ARBITER=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_BFD}" = "yes";then
   echo "ENABLE_BFD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_BFD=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_BFD_MONO}" = "yes";then
   echo "ENABLE_BFD_MONO=yes" >> $OPTIONS_FILE
fi

if test "${ENABLE_BFD_MAIN}" = "yes" ; then
   echo "ENABLE_BFD_MAIN=yes" >> $OPTIONS_FILE
fi

if test "${ENABLE_BFD_LINE}" = "yes" ; then
   echo "ENABLE_BFD_LINE=yes" >> $OPTIONS_FILE
fi

if test "${ENABLE_BGPD}" = "yes";then
   echo "ENABLE_BGPD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_BGPD=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_ORDER_NEXTHOP}" = "yes" ; then
   echo "ENABLE_ORDER_NEXTHOP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_ORDER_NEXTHOP=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_BGPD_EXTASN}" = "yes";then
   echo "ENABLE_BGPD_EXTASN=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_BGPD_EXTASN=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_RIPD}" = "yes";then
   echo "ENABLE_RIPD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_RIPD=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_RIPNGD}" = "yes";then
   echo "ENABLE_RIPNGD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_RIPNGD=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_OSPFD}" = "yes";then
   echo "ENABLE_OSPFD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_OSPFD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_OSPF6D" = "yes";then
   echo "ENABLE_OSPF6D=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_OSPF6D=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_OSPF6_CSPF" = "yes";then
   echo "ENABLE_OSPF6_CSPF=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_OSPF6_CSPF=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_STPD" = "yes";then
   echo "ENABLE_STPD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_STPD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_RSTPD" = "yes";then
   echo "ENABLE_RSTPD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_RSTPD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_AUTHD" = "yes";then
   echo "ENABLE_AUTHD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_AUTHD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_LACPD" = "yes";then
   echo "ENABLE_LACPD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_LACPD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_ELMID" = "yes";then
   echo "ENABLE_ELMID=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_ELMID=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_VLAN" = "yes";then
   echo "ENABLE_VLAN=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_VLAN=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_PVLAN" = "yes";then
   echo "ENABLE_PVLAN=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PVLAN=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_RMOND" = "yes";then
   echo "ENABLE_RMOND=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_RMOND=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_IPSEC" = "yes";then
   echo "ENABLE_IPSEC=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_IPSEC=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_FIREWALL" = "yes";then
   echo "ENABLE_FIREWALL=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_FIREWALL=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_VLAN_CLASS" = "yes";then
   echo "ENABLE_VLAN_CLASS=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_VLAN_CLASS=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_VLAN_STACK" = "yes";then
   echo "ENABLE_VLAN_STACK=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_VLAN_STACK=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_RATE_LIMIT" = "yes";then
   echo "ENABLE_RATE_LIMIT=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_RATE_LIMIT=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MSTPD" = "yes";then
   echo "ENABLE_MSTPD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MSTPD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_RPVST_PLUS" = "yes";then
   echo "ENABLE_RPVST_PLUS=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_RPVST_PLUS=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_SWFWDR" = "yes";then
   echo "ENABLE_SWFWDR=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_SWFWDR=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_IGMP_SNOOP" = "yes";then
   echo "ENABLE_IGMP_SNOOP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_IGMP_SNOOP=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MLD_SNOOP" = "yes";then
   echo "ENABLE_MLD_SNOOP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MLD_SNOOP=no" >> $OPTIONS_FILE
fi

if test "$DISABLE_DEFAULT_MLD_SNOOP" = "yes";then
   echo "DISABLE_DEFAULT_MLD_SNOOP=yes" >> $OPTIONS_FILE
else
   echo "DISABLE_DEFAULT_MLD_SNOOP=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_DCB" = "yes";then
   echo "ENABLE_DCB=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_DCB=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_IGMP_V3" = "yes";then
   echo "ENABLE_IGMP_V3=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_IGMP_V3=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_DEFAULT_BRIDGE" = "yes";then
   echo "ENABLE_DEFAULT_BRIDGE=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_DEFAULT_BRIDGE=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_PROVIDER_BRIDGE" = "yes";then
   echo "ENABLE_PROVIDER_BRIDGE=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PROVIDER_BRIDGE=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_I_BEB" = "yes";then
   echo "ENABLE_I_BEB=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_I_BEB=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_B_BEB" = "yes";then
   echo "ENABLE_B_BEB=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_B_BEB=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_PBB_TE" = "yes";then
   echo "ENABLE_PBB_TE=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PBB_TE=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_L2GP" = "yes";then
   echo "ENABLE_L2GP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_L2GP=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_GMRP" = "yes";then
   echo "ENABLE_GMRP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_GMRP=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MMRP" = "yes";then
   echo "ENABLE_MMRP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MMRP=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_GVRP" = "yes";then
   echo "ENABLE_GVRP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_GVRP=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MVRP" = "yes";then
   echo "ENABLE_MVRP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MVRP=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_NETLINK" = "yes";then
   echo "ENABLE_NETLINK=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_NETLINK=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_LDPD" = "yes";then
   echo "ENABLE_LDPD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_LDPD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_RSVPD" = "yes";then
   echo "ENABLE_RSVPD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_RSVPD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_ISISD" = "yes";then
   echo "ENABLE_ISISD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_ISISD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MULTI_TOPOLOGY" = "yes";then
   echo "ENABLE_MULTI_TOPOLOGY=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MULTI_TOPOLOGY=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_ONMD" = "yes";then
   echo "ENABLE_ONMD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_ONMD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_CFM" = "yes";then
   echo "ENABLE_CFM=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_CFM=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_CFM_Y1731" = "yes";then
  echo "ENABLE_CFM_Y1731=yes" >> $OPTIONS_FILE
else
  echo "ENABLE_CFM_Y1731=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_LLDP" = "yes";then
   echo "ENABLE_LLDP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_LLDP=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_EFM" = "yes";then
   echo "ENABLE_EFM=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_EFM=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_G8031" = "yes";then
  echo "ENABLE_G8031=yes" >> $OPTIONS_FILE
else
  echo "ENABLE_G8031=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_G8032" = "yes";then
  echo "ENABLE_G8032=yes" >> $OPTIONS_FILE
else
  echo "ENABLE_G8032=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_PDMD" = "yes";then
   echo "ENABLE_PDMD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PDMD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_PIMD" = "yes";then
   echo "ENABLE_PIMD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PIMD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_PIM_SSM" = "yes";then
   echo "ENABLE_PIM_SSM=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PIM_SSM=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_PIM6D" = "yes";then
   echo "ENABLE_PIM6D=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PIM6D=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_PIM6_SSM" = "yes";then
   echo "ENABLE_PIM6_SSM=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PIM6_SSM=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_PIMPKTGEND" = "yes";then
   echo "ENABLE_PIMPKTGEND=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PIMPKTGEND=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_DVMRPD" = "yes";then
   echo "ENABLE_DVMRPD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_DVMRPD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_DVMRP_SSM" = "yes";then
   echo "ENABLE_DVMRP_SSM=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_DVMRP_SSM=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MCAST_IPV4" = "yes";then
   echo "ENABLE_MCAST_IPV4=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MCAST_IPV4=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MCAST_IPV6" = "yes";then
   echo "ENABLE_MCAST_IPV6=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MCAST_IPV6=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_L3_NPF" = "yes";then
   echo "ENABLE_L3_NPF=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_L3_NPF=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_L2_NPF" = "yes";then
   echo "ENABLE_L2_NPF=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_L2_NPF=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MPLS_FWD" = "yes";then
   echo "ENABLE_MPLS_FWD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MPLS_FWD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_VRRP" = "yes";then
   echo "ENABLE_VRRP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_VRRP=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_RMM" = "yes";then
   echo "ENABLE_RMM=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_RMM=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_RMM_TESTPRIMARY" = "yes";then
   echo "ENABLE_RMM_TESTPRIMARY=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_RMM_TESTPRIMARY=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_RMM_TESTBACKUP" = "yes";then
   echo "ENABLE_RMM_TESTBACKUP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_RMM_TESTBACKUP=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_VRF" = "yes";then
   echo "ENABLE_VRF=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_VRF=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_CR_LDP" = "yes";then
   echo "ENABLE_CR_LDP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_CR_LDP=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_OSPF_MULTI_AREA" = "yes";then
   echo "ENABLE_OSPF_MULTI_AREA=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_OSPF_MULTI_AREA=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_OSPF_MULTI_INST" = "yes";then
   echo "ENABLE_OSPF_MULTI_INST=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_OSPF_MULTI_INST=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_CSPF" = "yes";then
   echo "ENABLE_CSPF=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_CSPF=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_OSPF_CSPF" = "yes";then
   echo "ENABLE_OSPF_CSPF=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_OSPF_CSPF=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_ISIS_CSPF" = "yes";then
   echo "ENABLE_ISIS_CSPF=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_ISIS_CSPF=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MPLS_FRR" = "yes";then
   echo "ENABLE_MPLS_FRR=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MPLS_FRR=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_VR" = "yes";then
   echo "ENABLE_VR=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_VR=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_VLOGD" = "yes";then
   echo "ENABLE_VLOGD=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_VLOGD=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_AIO" = "yes";then
   echo "ENABLE_AIO=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_AIO=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_DEV_TEST" = "yes";then
   echo "ENABLE_DEV_TEST=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_DEV_TEST=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MPLS_VC" = "yes";then
   echo "ENABLE_MPLS_VC=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MPLS_VC=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_VCCV" = "yes";then
   echo "ENABLE_VCCV=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_VCCV=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MS_PW" = "yes";then
   echo "ENABLE_MS_PW=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MS_PW=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_FEC129" = "yes";then
   echo "ENABLE_FEC129=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_FEC129=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_VPLS" = "yes";then
   echo "ENABLE_VPLS=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_VPLS=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MPLS_TDM_VC" = "yes";then
   echo "ENABLE_MPLS_TDM_VC=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MPLS_TDM_VC=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MPLS_ATM_VC" = "yes";then
   echo "ENABLE_MPLS_ATM_VC=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MPLS_ATM_VC=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_CRX" = "yes";then
   echo "ENABLE_CRX=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_CRX=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_RESTART" = "yes";then
   echo "ENABLE_RESTART=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_RESTART=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_RSVP_GRACEFUL" = "yes";then
   echo "ENABLE_RSVP_GRACEFUL=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_RSVP_GRACEFUL=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_SUN_CC" = "yes";then
   echo "ENABLE_SUN_CC=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_SUN_CC=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_SUN_CC_64_BIT" = "yes";then
   echo "ENABLE_SUN_CC_64_BIT=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_SUN_CC_64_BIT=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MEMMGR" = "yes";then
   echo "ENABLE_MEMMGR=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MEMMGR=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_LICMGR" = "yes";then
   echo "ENABLE_LICMGR=yes" >> $OPTIONS_FILE
   echo "LICMGRINCLUDE=../lmlicmgr/include" >> $OPTIONS_FILE
   if test "$ENABLE_SUN_CC_64_BIT" = "yes";then
     echo "LICMGRLIB=../../lmlicmgr/lib/lm_lib64.a" >> $OPTIONS_FILE
   else
     echo "LICMGRLIB=../../lmlicmgr/lib/lm_lib.a" >> $OPTIONS_FILE
   fi
else
   echo "ENABLE_LICMGR=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_CUSTOM1" = "yes"; then
   echo "ENABLE_CUSTOM1=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_CUSTOM1=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_SNMP" = "yes";then
   echo "ENABLE_SNMP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_SNMP=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_AGENTX" = "yes";then
   echo "ENABLE_AGENTX=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_AGENTX=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_STATIC" = "yes";then
   dnl echo "LDFLAGS=-static"  >> $OPTIONS_FILE
   echo "ENABLE_STATIC=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_STATIC=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_GMPLS" = "yes";then
   echo "ENABLE_GMPLS=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_GMPLS=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_PACKET" = "yes";then
   echo "ENABLE_PACKET=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PACKET=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_LMP" = "yes";then
   echo "ENABLE_LMP=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_LMP=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MAC_AUTH" = "yes";then
   echo "ENABLE_MAC_AUTH=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MAC_AUTH=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_DIFFSERV" = "yes";then
   echo "ENABLE_DIFFSERV=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_DIFFSERV=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_DSTE" = "yes";then
   echo "ENABLE_DSTE=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_DSTE=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_TUNNEL}" = "yes";then
   echo "ENABLE_TUNNEL=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_TUNNEL=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_STORAGE_DEV}" = "yes";then
   echo "ENABLE_STORAGE_DEV=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_STORAGE_DEV=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_HAL}" = "yes";then
   echo "ENABLE_HAL=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_HAL=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_AXELX}" = "yes";then
   echo "ENABLE_AXELX=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_AXELX=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_USER_HSL}" = "yes";then
   echo "ENABLE_USER_HSL=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_USER_HSL=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_BROADCOM}" = "yes";then
   echo "ENABLE_BROADCOM=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_BROADCOM=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_PNE_3_7}" = "yes";then
   echo "ENABLE_PNE_3_7=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PNE_3_7=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_MARVELL}" = "yes";then
   echo "ENABLE_MARVELL=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MARVELL=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_MARVELL_LS}" = "yes";then
   echo "ENABLE_MARVELL_LS=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MARVELL_LS=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_INTEL}" = "yes";then
   echo "ENABLE_INTEL=yes" >> $OPTIONS_FILE
   echo "INTEL_CPPDK_PATH=${INTEL_CPPDK_PATH}" >> $OPTIONS_FILE
else
   echo "ENABLE_INTEL=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_IPNET}" = "yes";then
   echo "ENABLE_IPNET=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_IPNET=no" >> $OPTIONS_FILE
fi
if test "${ENABLE_QOS}" = "yes" ; then
   echo "ENABLE_QOS=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_QOS=no" >> $OPTIONS_FILE
fi
if test "$ENABLE_CONFIG_COMPARE" = "yes"; then
  echo "ENABLE_CONFIG_COMPARE=yes" >> $OPTIONS_FILE
else
  echo "ENABLE_CONFIG_COMPARE=no" >> $OPTIONS_FILE
fi
if test "${ENABLE_L2LERN}" = "yes" ; then
   echo "ENABLE_L2LERN=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_L2LERN=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_HWSTACK}" = "yes" ; then
   echo "ENABLE_HWSTACK=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_HWSTACK=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_MULTI_CONF_SES}" = "yes" ; then
   echo "ENABLE_MULTI_CONF_SES=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MULTI_CONF_SES=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_ENEA_ELEMENT}" = "yes" ; then
   echo "ENABLE_ENEA_ELEMENT=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_ENEA_ELEMENT=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_OPEN_SAF}" = "yes" ; then
   echo "ENABLE_OPEN_SAF=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_OPEN_SAF=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_HA}" = "yes" ; then
   echo "ENABLE_HA=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_HA=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_PACOS_DEBUG}" = "yes" ; then
   echo "ENABLE_PACOS_DEBUG=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PACOS_DEBUG=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_FM_SIM}" = "yes" ; then
   echo "ENABLE_FM_SIM=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_FM_SIM=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_PBR}" = "yes";then
   echo "ENABLE_PBR=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_PBR=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MPLS" = "yes";then
   echo "ENABLE_MPLS=yes" >> $OPTIONS_FILE
   # For MPLS Client.
   MPLS_CLIENT="mpls_client"
   MPLS_CLIENT_LIB="../lib/mpls_client/libmpls_client.a"
   MPLS_CLIENT_INCLUDE="-I../lib/mpls_client"
   AC_SUBST(MPLS_CLIENT)
   AC_SUBST(MPLS_CLIENT_LIB)
   AC_SUBST(MPLS_CLIENT_INCLUDE)
else
   echo "ENABLE_MPLS=no" >> $OPTIONS_FILE
fi

if test "$ENABLE_MPLS_OAM" = "yes"; then
   echo "ENABLE_MPLS_OAM=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_MPLS_OAM=no" >> $OPTIONS_FILE
fi



if test "${ENABLE_BIGENDIAN}" = "yes" ; then
   echo "ENABLE_BIGENDIAN=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_BIGENDIAN=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_SMI}" = "yes";then
   echo "ENABLE_SMI=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_SMI=no" >> $OPTIONS_FILE
fi

echo "ENABLE_6PE=${ENABLE_6PE}" >> $OPTIONS_FILE

if test "${ENABLE_LDP_INTER_AREA}" = "yes";then
   echo "ENABLE_LDP_INTER_AREA=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_LDP_INTER_AREA=no" >> $OPTIONS_FILE
fi

if test "${ENABLE_LDP_SUPPRESS_FEC}" = "yes";then
   echo "ENABLE_LDP_SUPPRESS_FEC=yes" >> $OPTIONS_FILE
else
   echo "ENABLE_LDP_SUPPRESS_FEC=no" >> $OPTIONS_FILE
fi

dnl Compiler option(s)

if test "$nopointersign" = "yes" ; then
   echo "COPT_NO_POINTER_SIGN=yes" >> $OPTIONS_FILE
fi

if test "$noworkingdirectory" = "yes" ; then
   echo "COPT_NO_WORKING_DIRECTORY=yes" >> $OPTIONS_FILE
fi

dnl Linux kernel version info
echo "LINUX_VERSION_MAJOR=$LINUX_VERSION_MAJOR" >> $OPTIONS_FILE
if test "$LINUX_VERSION_MINOR" != "" ;  then
   echo "LINUX_VERSION_MINOR=$LINUX_VERSION_MINOR" >> $OPTIONS_FILE
fi


AC_OUTPUT(Makefile)

if test "$cross_compiling" = "yes";then
if test x"$opsys" = x"" || test "$with_ipnet2" = "yes"; then
echo "#include \"pacos_config.h\"" >> $OPSYSDIR/config.h
fi
fi

if test "$with_ipnet2" = "yes"; then
echo "#include \"pacos_config.h\"" >> $OPSYSDIR/config.h
fi

dnl ---------------------------------------------------------------------------
dnl                              PacOS checks END
dnl ---------------------------------------------------------------------------

echo "
PacOS configuration
-------------------
PacOS version           : ${VERSION}
Build date              : ${BUILDDATE}
host operating system   : ${host_os}
source code location    : ${srcdir}
compiler                : ${CC}
compiler flags          : ${CFLAGS}
directory for pid files : ${ac_piddir}
"
