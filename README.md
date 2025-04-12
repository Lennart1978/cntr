# cntr
cntr: A CLI command that prints a file or data from a pipe centered to stdout. Written in C
## example:
```bash
./cntr text.txt
cat text.txt | ./cntr
```
## Compile:
```bash
gcc -s -O3 -lm cntr.c -o cntr
```
### Install:
```bash
sudo cp cntr /usr/bin
```
### A (senseless) example:
```bash
ifconfig | cntr
          enp3s0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
           inet 192.168.2.100  netmask 255.255.255.0  broadcast 192.168.2.255
        inet6 2003:db:370c:8200:c640:5ce7:5aff:7728  prefixlen 64  scopeid 0x0<global>
        inet6 fde2:8acd:e9d3:0:f4e5:1702:e03a:7f92  prefixlen 64  scopeid 0x0<global>
            inet6 fe80::226d:6e25:92c:f10  prefixlen 64  scopeid 0x20<link>
                  ether a8:a1:59:3b:4b:5b  txqueuelen 1000  (Ethernet)
                     RX packets 257343  bytes 351672699 (335.3 MiB)
                      RX errors 0  dropped 0  overruns 0  frame 0
                      TX packets 107295  bytes 11908385 (11.3 MiB)
               TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

                  lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
                           inet 127.0.0.1  netmask 255.0.0.0
                      inet6 ::1  prefixlen 128  scopeid 0x10<host>
                        loop  txqueuelen 1000  (Local Loopback)
                           RX packets 2  bytes 160 (160.0 B)
                      RX errors 0  dropped 0  overruns 0  frame 0
                           TX packets 2  bytes 160 (160.0 B)
               TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```
