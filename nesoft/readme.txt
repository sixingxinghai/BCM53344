[*编译说明*]
进入platform/linux目录下
>> 编译单板版本
 >>> 设置 ./config_bcm.sh (仅在第一次编译单板版本时需要)
     export KERNEL_SOURCE=
     export COMPILER_DIR=
	 enable/disable 各编译开关
     执行  ./config_bcm.sh 
 >>> 执行  make all

>> 编译pc版本
 >>> 设置 ./config_pc.sh (仅在第一次编译pc版本时需要)
     enable/disable 各编译开关
     执行  ./config_pc.sh 
 >>> 执行  make all

[*编译开关*]
# mpls 可用  enable-mpls enable-mpls-vc enable-ms-pw enable-vpls enable-mpls-frr enable-mpls-fwd enable-mpls-tdm enable-vrf
# vlog 可用  enable-vr enable-vlogd
# 其它可用   enable-dste enable-pbr
#
# enable-smi enable-mpls-oam enable-swfwdr enable-elmid 不可用
# imimore vtysh目录没有用到