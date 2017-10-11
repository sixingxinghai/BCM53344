#!/bin/bash

echo "#!/bin/bash"  > /etc/init.d/mpls_mod
echo "/sbin/rmmod mpls_module" >> /etc/init.d/mpls_mod
chmod +x /etc/init.d/mpls_mod
ln -s /etc/initd.d/mplsmod /etc/rc.d/rc0.d/K99mplsmod
ln -s /etc/initd.d/mplsmod /etc/rc.d/rc6.d/K99mplsmod
