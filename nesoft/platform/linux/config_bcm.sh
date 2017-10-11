# Please add these optional environment variables if needed:
#
# COMPILER_DIR    : path of compiler
#   export COMPILER_DIR=/opt/twblinux/bin
# COMPILER_PREFIX : prefix of compiler
#   export COMPILER_PREFIX=cris-muffin-linux-
# KERNEL_SOURCE   : path to linux kernel source
# CROSSOPTS       : values for --build,--host, and --target when cross compiling
#   export CROSSOPTS= --host=i686-linux --build=cris-muffin-linux --target=cris-muffin- linux 
# When cross compiling, specify CC on the command line
#   CC=$COMPILER_DIR/${COMPILER_PREFIX}gcc ./configure $CROSSOPTS ...
#
#CC=gcc
#This config.sh is the configuration script used to build this software and is intended to be used as an example. Please refer to the documentation for descriptions of all the configuation options to help configure the code to your specifications.

export KERNEL_SOURCE=/workspace/bcm53344/iproc/kernel/linux-3.6.5
export COMPILER_DIR=/workspace/bcm53344/iproc/buildroot/host/usr/bin
export COMPILER_PREFIX=arm-broadcom-linux-uclibcgnueabi-
export CROSSOPTS="--host=arm-broadcom-linux-uclibcgnueabi --build=i686-pc-linux-gnu --target=arm-broadcom-linux-uclibcgnueabi"

cd $(dirname $0)
make clean

cd ../../script
CC=$COMPILER_DIR/${COMPILER_PREFIX}gcc ./configure $CROSSOPTS\
    --enable-broadcom\
    --disable-swfwdr\
    --enable-hal\
    --enable-imi\
    --disable-imish\
    --enable-if-arbiter\
    --enable-basic-access\
    --enable-rate-limit\
    --enable-restart\
    --enable-netlink\
    --enable-l2lern\
    --enable-mac-auth\
    --disable-multi-conf-ses\
    --disable-mcst-ipv4\
    --enable-rtadv\
    --enable-vlan\
    --enable-intervlan-routing\
    --enable-default-bridge\
    --enable-stpd\
    --enable-rstpd\
    --enable-mstpd\
    --enable-rpvst-plus\
    --enable-vlogd\
    --enable-vlan-class\
    --enable-qos\
    --enable-gmrp\
    --enable-mmrp\
    --enable-gvrp\
    --enable-mvrp\
    --disable-elmid\
    --disable-pvlan\
    --disable-provider-bridge\
    --disable-onmd\
    --disable-efm\
    --disable-cfm\
    --disable-cfm-Y1731\
    --disable-lldp\
    --disable-bfd\
    --disable-mpls-vc\
    --disable-ms-pw\
    --disable-vpls\
    --disable-ldpd\
    --disable-rsvpd\
    --disable-rsvp-graceful\
    --disable-igmp-v3\
    --disable-igmp-snoop\
    --disable-lacpd \
    --disable-agentx\
    --disable-g8031\
    --disable-g8032\
    --disable-authd\
    --disable-rmond\
    --disable-pdmd\
    --disable-vr\
    --disable-dste\
    --disable-gmpls\
    --disable-mpls-frr\
    --disable-mpls-fwd\
    --disable-diffserv\
    --disable-ospf-cspf\
    --disable-te\
    --disable-mpls-oam\
    --disable-pece-ospf\
    --disable-ospfd\
    --disable-ospf-multi-area\
    --disable-ospf-multi-inst\
    --disable-pece-rip\
    --disable-ripd\
    --disable-mpls-tdm \
    --disable-smi\
    --disable-fm-sim\
    --disable-pece-ospf6\
    --disable-pece-ripng\
    --disable-6pe\
    --disable-i-pbb\
    --disable-b-pbb\
    --disable-pbb-te\
    --disable-pimd\
    --disable-pim-ssm\
    --disable-pim6-ssm\
    --disable-dvmrp-ssm\
    --disable-ipv6\
    --disable-isisd\
    --disable-dvmrpd\
    --disable-ripngd\
    --disable-ospf6d\
    --disable-pim6d\
    --disable-wmi\
    --disable-memmgr\
    --disable-vrf\
    --disable-vrrp\
    --disable-dcb\
    --disable-bgpd\
    --disable-pbr\
    --disable-lmpd
