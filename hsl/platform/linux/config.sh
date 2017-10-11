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

export KERNEL_SOURCE=/home/iapn8500v100r001/plateform/linux-2.6.29.6
export COMPILER_DIR=/home/iapn8500v100r001/plateform/tools/powerpc-e300c3-linux-gnu/bin
#export BROADCOM_SDK=/home/iapn8500v100r001/plateform/sdk-all-5.8.0
export BROADCOM_SDK=/mnt/hgfs/code/sdk-all-5.8.0
export COMPILER_PREFIX=powerpc-e300c3-linux-gnu-
export CROSSOPTS="--host=powerpc-e300c3-linux --build=i686-linux --target=powerpc-linux"

#export KERNEL_SOURCE=/home/iapn8500v100r001/plateform/linux-2.6.29.6
#export COMPILER_DIR=/home/iapn8500v100r001/plateform/tools/powerpc-e300c3-linux-gnu/bin
#export BROADCOM_SDK=/home/iapn8500v100r001/plateform/sdk-all-5.8.0/sdk-all-5.8.0

cd $(dirname $0)
make clean

cd ../../script
CC=$COMPILER_DIR/${COMPILER_PREFIX}gcc ./configure $CROSSOPTS\
    --enable-broadcom\
    --disable-swfwdr\
    --enable-hal\
    --enable-imi\
    --enable-imish\
    --enable-if-arbiter\
    --disable-basic-access\
    --enable-rate-limit\
    --enable-restart\
    --enable-netlink\
    --enable-l2lern\
    --enable-mac-auth\
    --disable-multi-conf-ses\
    --disable-mcast-ipv4\
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
    --disable-lmpd\
    --disable-mpls
