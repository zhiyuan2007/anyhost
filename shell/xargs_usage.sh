awk '/^[^#]/ {print $1}' dns_node.conf |xargs -I file dig @file -p 54 www.zhuget.com.edge.360gtm.com. +client=114.80.208.176 +short
awk '/^[^#]/ {print $1}' dns_node.conf |xargs -I file dig @file -p 54 www.zhuget.com.parent.360gtm.com. +client=114.80.208.176 +short
