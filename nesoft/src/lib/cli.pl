#! /usr/bin/perl
##
## CLI command extractor.
## Copyright (C) 2002-2003
##

$show_flag = 0;

$ignore{'"interface IFNAME"'} = "ignore";
$ignore{'"interface LINE"'} = "ignore";
$ignore{'"interface tunnel <0-2147483647>"'} = "ignore";
$ignore{'"interface vlan <1-4094>"'} = "ignore";
$ignore{'"interface loopback"'} = "ignore";
$ignore{'"interface manage"'} = "ignore";

$ignore{'"interface fastEthernet RANGEXFPORT"'} = "ignore";
$ignore{'"interface gigabitEthernet RANGEXGPORT"'} = "ignore";
$ignore{'"interface extended-module RANGEXEPORT"'} = "ignore";
$ignore{'"interface range RANGEXAPORT"'} = "ignore";

$ignore{'"show interface (IFNAME|)"'} = "ignore";
$ignore{'"ip vrf WORD"'} = "ignore";
$ignore{'"ip vrf NAME"'} = "ignore";
$ignore{'"mpls vpls NAME <1-4294967295>"'} = "ignore";
$ignore{'"mpls vpls NAME"'} = "ignore";

$ignore{'"hostname WORD"'} = "ignore";
$ignore{'"no hostname (WORD|)"'} = "ignore";
$ignore{'"service advanced-vty"'} = "ignore";
$ignore{'"no service advanced-vty"'} = "ignore";

$ignore{'"router rip"'} = "ignore";
$ignore{'"router ipv6 rip"'} = "ignore";
$ignore{'"router ipv6 rip WORD"'} = "ignore";
$ignore{'"router ospf"'} = "ignore";
$ignore{'"router ospf <1-65535>"'} = "ignore";
$ignore{'"router ospf <1-65535> WORD"'} = "ignore";
$ignore{'"router ipv6 ospf"'} = "ignore";
$ignore{'"router ipv6 ospf WORD"'} = "ignore";
$ignore{'"router ipv6 vrf ospf WORD"'} = "ignore";
$ignore{'"router isis (WORD|)"'} = "ignore";
#ifndef HAVE_EXT_CAP_ASN
$ignore{'"router bgp <1-65535>"'} = "ignore";
$ignore{'"router bgp <1-65535> view WORD"'} = "ignore";
#else
$ignore{'"router bgp <1-4294967295>"'} = "ignore";
$ignore{'"router bgp <1-4294967295> view WORD"'} = "ignore";
#endif
$ignore{'"router vrrp <1-255> IFNAME"'} = "ignore";
$ignore{'"router ipv6 vrrp <1-255> IFNAME"'} = "ignore";
$ignore{'"router vrrp <1-255> vlan <1-4094"'} = "ignore";
$ignore{'"router ipv6 vrrp <1-255> vlan <1-4094"'} = "ignore";
$ignore{'"address-family ipv4"'} = "ignore";
$ignore{'"address-family ipv4 (unicast|multicast)"'} = "ignore";
$ignore{'"address-family ipv6 (unicast|)"'} = "ignore";
$ignore{'"address-family ipv6 labeled-unicast"'} = "ignore";
$ignore{'"address-family vpnv4"'} = "ignore";
$ignore{'"address-family vpnv4 unicast"'} = "ignore";
$ignore{'"address-family ipv4 vrf NAME"'} = "ignore";
$ignore{'"address-family vpnv6"'} = "ignore";
$ignore{'"address-family vpnv6 unicast"'} = "ignore";
$ignore{'"address-family ipv6 vrf NAME"'} = "ignore";
$ignore{'"exit-address-family"'} = "ignore";
$ignore{'"router ldp"'} = "ignore";
$ignore{'"targeted-peer ipv4 A.B.C.D"'} = "ignore";
$ignore{'"exit-targeted-peer-mode"'} = "ignore";
$ignore{'"ldp-path PATHNAME"'} = "ignore";
$ignore{'"ldp-trunk TRUNKNAME"'} = "ignore";
$ignore{'"router rsvp"'} = "ignore";
#ifdef HAVE_GMPLS
$ignore{'"rsvp-trunk TRUNKNAME (ipv4|ipv6|gmpls|)"'} = "ignore";
#else
$ignore{'"rsvp-trunk TRUNKNAME (ipv4|ipv6|)"'} = "ignore";
#endif
$ignore{'"rsvp-trunk-exit"'} = "ignore";
#ifdef HAVE_GMPLS
$ignore{'"rsvp-path PATHNAME (mpls|gmpls|)"'} = "ignore";
#else
$ignore{'"rsvp-path PATHNAME"'} = "ignore";
#endif
$ignore{'"rsvp-path-exit"'} = "ignore";
$ignore{'"rsvp-bypass BYPASSNAME"'} = "ignore";
$ignore{'"rsvp-bypass-exit"'} = "ignore";
$ignore{'"key chain WORD"'} = "ignore";
$ignore{'"no key chain WORD"'} = "ignore";
$ignore{'"key <0-2147483647>"'} = "ignore";
$ignore{'"no key <0-2147483647>"'} = "ignore";
$ignore{'"route-map WORD (deny|permit) <1-65535>"'} = "ignore";
$ignore{'"no route-map WORD ((deny|permit) <1-65535>|)"'} = "ignore";
$ignore{'"exit"'} = "ignore";
$ignore{'"end"'} = "ignore";
$ignore{'"ip dhcp pool WORD"'} = "ignore";
$ignore{'"service pppoe"'} = "ignore";
$ignore{'"show running-config"'} = "ignore";
$ignore{'"line console <0-0>"'} = "ignore";
$ignore{'"line aux <0-0>"'} = "ignore";
$ignore{'"line vty <0-871> (<0-871>|)"'} = "ignore";
$ignore{'"configure terminal"'} = "ignore";
$ignore{'"enable"'} = "ignore";
$ignore{'"disable"'} = "ignore";
$ignore{'"password (8|) LINE"'} = "ignore";
$ignore{'"no password"'} = "ignore";
$ignore{'"pw <1-2000>"'} = "ignore";
$ignore{'"vpn <1-2000>"'} = "ignore";
$ignore{'"fe <1-24>"'} = "ignore";
$ignore{'"ge <1-4>"'} = "ignore";
$ignore{'"lsp <1-2000>"'} = "ignore";
$ignore{'"lsp-protect-group <0-63>"'} = "ignore";
$ignore{'"lsp-oam <0-63>"'} = "ignore";
$ignore{'"ac <1-2000>"'} = "ignore";
$ignore{'"tun <1-2000>"'} = "ignore";
$ignore{'"meg INDEX"'} = "ignore";
$ignore{'"section ID"'} = "ignore";
$ignore{'"mpls-tp oam {y1731-mode |bfd-mode}"'} = "ignore";
$ignore{'"ptp"'} = "ignore";
$ignore{'"port <0-19>"'} = "ignore";

$ignore{'"write (file|miniconfig)"'} = "ignore";
$ignore{'"write memory"'} = "ignore";
$ignore{'"copy running-config startup-config"'} = "ignore";

$ignore{'"virtual-router WORD"'} = "ignore";
$ignore{'"virtual-router <1-512>"'} = "ignore";
$ignore{'"configure virtual-router WORD"'} = "ignore";
$ignore{'"configure virtual-router <1-512>"'} = "ignore";
$ignore{'"exit virtual-router"'} = "ignore";
$ignore{'"load (ospf|bgp|rip|isis)"'} = "ignore";
$ignore{'"load ipv6 (ospf|rip)"'} = "ignore";
$ignore{'"no load (ospf|bgp|rip|isis)"'} = "ignore";
$ignore{'"no load ipv6 (ospf|rip)"'} = "ignore";
$ignore{'"enable-vr"'} = "ignore";
$ignore{'"disable-vr"'} = "ignore";

$ignore{'"spanning-tree mst configuration"'} = "ignore";
$ignore{'"spanning-tree rpvst+ configuration"'} = "ignore";
$ignore{'"spanning-tree te-msti configuration"'} = "ignore";

$ignore{'"vlan database"'} = "ignore";
$ignore{'"cvlan registration table WORD"'} = "ignore";
$ignore{'"cvlan registration table WORD bridge <1-32>"'} = "ignore";
$ignore{'"vlan access-map WORD <1-65535>"'} = "ignore";
$ignore{'"no vlan access-map WORD <1-65535>"'} = "ignore";
$ignore{'"reload"'} = "ignore";
$ignore{'"class-map NAME"'} = "ignore";
$ignore{'"policy-map NAME"'} = "ignore";
$ignore{'"class NAME"'} = "ignore";

$ignore{'"service instance INSTANCE_ID evc-id EVC_ID"'} = "ignore";
$ignore{'"exit-service-instance-mode"'} = "ignore";

$ignore{'"ethernet cfm domain-name type ((no-name name NO_NAME)| (dns-based  name DOMAIN_NAME) | (character-string name DOMAIN_NAME)) level LEVEL mip-creation (none|default|explicit)(bridge <1-32>|)"'} = "ignore";
$ignore{'"ethernet cfm domain-name type mac name DOMAIN_NAME level LEVEL mip-creation (none|default|explicit)(bridge <1-32>|)"'} = "ignore";
$ignore{'"ethernet cfm domain-name type itu-t name DOMAIN_NAME level LEVEL mip-creation (none|default|explicit)(bridge <1-32>|)"'} = "ignore";
$ignore{'"ethernet cfm pbb domain-name type ((no-name name NO_NAME)| (dns-based  name DOMAIN_NAME) | (character-string name DOMAIN_NAME)) pbb-domain-type (svid|service|bvlan|link) level LEVEL (mip-creation (none|default|explicit)|) (bridge <1-32> | backbone) ((p-enni | h-enni)|)"'} = "ignore";
$ignore{'"ethernet cfm pbb domain-name type mac name DOMAIN_NAME pbb-domain-type (svid|service|bvlan|link) level LEVEL mip-creation (none|default|explicit) (bridge <1-32> | backbone) ((p-enni | h-enni)|)"'} = "ignore";
$ignore{'"ethernet cfm pbb domain-name type itut-t name DOMAIN_NAME pbb-domain-type (svid|service|bvlan|link) level LEVEL mip-creation (none|default|explicit)(bridge <1-32> | backbone) ((p-enni | h-enni)|)"'} = "ignore";

$ignore{'"ethernet cfm configure vlan PRIMARY_VID ((bridge <1-32>| backbone)|)"'} = "ignore";
$ignore{'"g8031 configure switching eps-id EPS_ID bridge <1-32>"'} = "ignore";
$ignore{'"g8031 configure switching eps-id EPS_ID bridge (<1-32> | backbone)"'} = "ignore";
$ignore{'"g8031 configure switching eps-id EPS_ID bridge backbone"'} = "ignore";
$ignore{'"g8031 configure switching"'} = "ignore";
$ignore{'"g8031 configure vlan eps-id EPS-ID bridge (<1-32> | backbone)"'} = "ignore";
$ignore{'"g8031 configure vlan eps-id EPS-ID bridge <1-32>"'} = "ignore";
$ignore{'"g8031 configure vlan eps-id EPS-ID bridge backbone"'} = "ignore";
$ignore{'"pbb isid list"'} = "ignore";
$ignore{'"pbb-te configure tesid TESID (name TRUNK_NAME|)"'} = "ignore";
$ignore{'"pbb-te configure esp tesi TESID"'} = "ignore";
$ignore{'"exit-pbb-te-esp-mode"'} = "ignore";


$ignore{'"g8032 configure switching ring-id RINGID bridge (<1-32> | backbone)"'} = "ignore";
$ignore{'"g8032 configure switching ring-id RINGID bridge <1-32>"'} = "ignore";
$ignore{'"g8032 configure vlan ring-id RINGID bridge (<1-32> | backbone)"'} = "ignore";
$ignore{'"g8032 configure vlan ring-id RINGID bridge <1-32>"'} = "ignore";

#ifdef HAVE_LMP
$ignore{'"lmp-configuration"'} = "ignore";
#endif /* HAVE_LMP */

#ifdef HAVE_GMPLS
$ignore{'"te-link TLNAME local-link-id A.B.C.D (numbered |)"'} = "ignore";
$ignore{'"te-link TLNAME"'} = "ignore";
$ignore{'"control-channel CCNAME cc-id <1-4294967295> local-address A.B.C.D peer-address A.B.C.D"'} = "ignore";
$ignore{'"control-channel CCNAME"'} = "ignore";
$ignore{'"control-adjacency CADJNAME peer-address A.B.C.D (static |using-lmp |)"'} = "ignore";
$ignore{'"control-adjacency CADJNAME"'} = "ignore";
#endif
#ifdef HAVE_PBB_TE
#if defined HAVE_I_BEB && defined HAVE_B_BEB
$ignore{'"ethernet cfm pbb-te domain-name type ((no-name name NO_NAME)| (dns-based  name DOMAIN_NAME) | (character-string name  DOMAIN_NAME) | mac name DOMAIN_NAME) level LEVEL mip-creation (none|default|explicit) bridge backbone"'} = "ignore";
$ignore{'"ethernet cfm pbb-te domain-name type itu-t name DOMAIN_NAME level LEVEL mip-creation (none|default|explicit) bridge backbone"'} = "ignore";
#else
$ignore{'"ethernet cfm pbb-te domain-name type ((no-name name NO_NAME)| (dns-based  name DOMAIN_NAME) | (character-string name  DOMAIN_NAME) | mac name DOMAIN_NAME) level LEVEL mip-creation (none|default|explicit) bridge <1-32>"'} = "ignore";
$ignore{'"ethernet cfm pbb-te domain-name type itu-t name DOMAIN_NAME level LEVEL mip-creation (none|default|explicit) bridge <1-32>"'} = "ignore";
#endif /* HAVE_I_BEB && HAVE_B_BEB */
#endif /* HAVE_PBB_TE */
$ignore{'"pbb-te configure switching cbp IFNAME aps-group APS-ID"'} = "ignore";
$ignore{'"exit-pbb-te-switching-mode"'} = "ignore";

$ignore{'"crypto ipsec transform-set NAME esp-auth (None|esp-md5|esp-sha1) esp-enc (esp-null|esp-des|esp-3des|esp-aes|esp-blf|esp-cast)"'} = "ignore";
$ignore{'"crypto ipsec transform-set NAME ah (None|ah-md5|ah-sha1)"'} = "ignore";
$ignore{'"crypto map MAP-NAME SEQ-NUM ipsec-manual"'} = "ignore";
$ignore{'"crypto map MAP-NAME SEQ-NUM ipsec-isakmp"'} = "ignore";
$ignore{'"crypto isakmp policy PRIORITY"'} = "ignore";
$ignore{'"firewall group <1-30>"'} = "ignore";

$BLD_NAME = shift @ARGV;
$INC_DIR = shift @ARGV;

if ($ARGV[0] eq "-show") {
    $show_flag = 1;
    shift @ARGV;
}

print <<EOF;
#include "pal.h"
#include "lib.h"
#include "cli.h"

EOF

if ($show_flag) {
    print "int generic_show_func (struct cli *, int, char **);\n\n";
}

foreach (@ARGV) {
    $file = $_;

    open (FH, "$INC_DIR $file |");

    local $/; undef $/;
    $line = <FH>;
    close (FH);

    @defun = ($line =~ /(?:CLI|ALI)\s*\((.+?)\);?\s?\s?\n/sg);
    @install = ($line =~ /cli_install\S*\s*\([^;]+?;/sg);

    # Protocol flag.
    $protocol = "PM_EMPTY";

  if ($file =~ /lib/) {
        if ($file =~ /line.c/) {
            $protocol = "PM_EMPTY";
        }
        if ($file =~ /igmp\/igmp_cli.c/) {
            $protocol = "PM_IGMP";
        }
        if ($file =~ /igmp\/igmp_show.c/) {
            $protocol = "PM_IGMP";
        }
        if ($file =~ /l2lib/) {
            $protocol = "PM_STP|PM_RSTP|PM_MSTP";
        }
        if ($file =~ /mld\/mld_cli.c/) {
            $protocol = "PM_MLD";
        }
        if ($file =~ /mld\/mld_show.c/) {
            $protocol = "PM_MLD";
        }
        if ($file =~ /fm\/lib_fm_cli.c/) {
            $protocol = "PM_FM";
        }
        if ($file =~ /distribute.c/) {
            $protocol = "PM_DISTRIBUTE";
        }
        if ($file =~ /if_rmap.c/) {
            $protocol = "PM_IFRMAP";
        }
        if ($file =~ /routemap.c/) {
            $protocol = "PM_RMAP";
        }
        if ($file =~ /filter.c/) {
            $protocol = "PM_ACCESS";
        }
        if ($file =~ /plist.c/) {
            $protocol = "PM_PREFIX";
        }
        if ($file =~ /log.c/) {
            $protocol = "PM_LOG";
        }
        if ($file =~ /keychain.c/) {
            $protocol = "PM_KEYCHAIN";
        }
    } elsif ($file =~ /cal/) {
       if ($file =~ /fm\/sim\/sim_fm_cli.c/) {
           $protocol = "PM_FM";
       } else {
          $protocol = "PM_CAL";
       }
    } elsif ($file =~ /swp/) {
        $protocol = "PM_NSM";
    } elsif ($file =~ /hal/) {
        $protocol = "PM_NSM";
    } elsif ($file =~ /pac/) {
        $protocol = "PM_NSM";
	} else {
        ($protocol) = ($file =~ /\/([a-z0-9]+)/);
        $protocol =~ s/d$//;
        $protocol = "PM_" . uc $protocol;
    }

    # DEFUN process
    foreach (@defun) {
        my (@defun_array);
        @defun_array = split (/,\s*\n/);

        $str = "$defun_array[2]";
        $str =~ s/^\s+//g;
        $str =~ s/\s+$//g;
        $str =~ s/\t//g;

        # When this command string in ignore list skip it.
        next if defined ($ignore{$str});

        # Add IMI string
        $defun_array[1] .= "_imi";

        # Replace _cli with _imish.
        $defun_array[1] =~ s/_cli/_imish/;

        # Show command
        if ($defun_array[2] =~ /^\s*\"show/
            || $defun_array[1] =~ /write_terminal/) {
            if ($show_flag) {
                $defun_array[0] = "generic_show_func";
            } else {
                next;
            }
        } else {
            $defun_array[0] = NULL;
        }

        $proto = $protocol;
        if ($defun_array[2] =~ /^\s*\"show/) {
          if ($file =~ /routemap.c/
              || ($file =~ /lib/
                  && $file =~ /filter.c/)
              || $file =~ /plist.c/) {
            $proto = "PM_EMPTY";
          }
        }

        $defun_body = join (",\n", @defun_array);

        $cli = $defun_array[1];
        $cli =~ s/^\s+//g;
        $cli =~ s/\s+$//g;
        $cli =~ s/\t//g;

        $cli2str{$cli} = $str;
        $cli2defun{$cli} = $defun_body;
        $cli2proto{$cli} = $proto;
    }

    # cli_install() process

    foreach (@install) {
        my (@install_array) = split (/,/);
        my $func = pop @install_array;
        my $flags = "0";
        my $priv;
        my $mode;

        if ($install_array[0] =~ /gen/) {
            $flags = $install_array[3];
            $priv = $install_array[2];
            $mode = $install_array[1];
        } elsif ($install_array[0] =~ /imi/) {
            $flags = $install_array[4];
            $priv = $install_array[3];
            $mode = $install_array[1];
        } else {
            if ($install_array[0] =~ /hidden/) {
                $flags = "CLI_FLAG_HIDDEN";
            }
            if ($install_array[1] =~ /EXEC_PRIV_MODE/) {
                $mode = "EXEC_MODE";
                $priv = "PRIVILEGE_MAX";
            } else {
                $mode = $install_array[1];
                $priv = "PRIVILEGE_NORMAL";
            }
        }

        ($cli) = ($func =~ /&([^\)]+)/);
        $cli =~ s/^\s+//g;
        $cli =~ s/\s+$//g;
        $cli =~ s/_cli/_imish/;

        # Add IMI string
        $cli .= "_imi";

        $mode =~ s/^\s+//g;
        $mode =~ s/\s+$//g;

        if (defined ($cli2defun{$cli})) {
            my ($key) = $cli2str{$cli} . "," . $mode;

            $cli2install{$key} = [ $cli, $mode, $priv, $flags ];

            push (@{$cli2pm{$key}}, $cli2proto{$cli});
        }
    }
}

foreach (keys %cli2defun) {
    printf ("IMI_ALI (%s);\n\n", $cli2defun{$_});
}

printf ("\nvoid\nimi_extracted_cmd_init (struct cli_tree *ctree)\n{\n");
foreach (keys %cli2install) {
    my $proto_str;
    $count = 0;
    printf ("  SET_FLAG (%s.flags, CLI_FLAG_SHOW);\n",
            $cli2install{$_}[0]) if $_ =~ /write terminal/;

    next if $cli2mode{$_} eq "mode";

    {
      $count = $#{$cli2pm{$_}} ;
      $proto_str = join (", &", @{$cli2pm{$_}});
    }

    if ($count == 0) {
    printf ("  cli_install_imi (ctree, %s, %s, %s, %s, &%s);\n",
            $cli2install{$_}[1], $proto_str, $cli2install{$_}[2],
            $cli2install{$_}[3], $cli2install{$_}[0]);
    } else {
        printf ("  cli_install_imi (ctree, %s, modbmap_vor (%s, &%s), %s, %s, &%s);\n",
                $cli2install{$_}[1], ($count+1), $proto_str, $cli2install{$_}[2],
                $cli2install{$_}[3], $cli2install{$_}[0]);
    }
}
printf ("}\n");

