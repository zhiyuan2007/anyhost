h1. this tool is used to control remote machine

h1. required packages

> python 2.6 or higher
> paramkio (yum install python-dev && easy_install paramiko)

h1 .the usage as follows:

<pre>

Usage: rpctool [options]

Options:
  -h, --help            show this help message and exit
  -H HOST, --host=HOST  remote host, domain or ip is ok, can be have many -H
                        option (required unless given -f option)
  -p PASSWD, --passwd=PASSWD
                        password (required)
  -u USER, --user=USER  username (required)
  -m CMD, --cmd=CMD     list of commands will be run in remote host, can be
                        have many -m option (required)
  -s SRC, --src=SRC     source file, absolute path
  -t TRANSFER, --transfer=TRANSFER
                        transfer mode, upload or download
  -d DEST, --dest=DEST  destination filename, absolute path
  -S, --sudo            whether run cmd as root
  -T, --thread          multi-thread mode
  -f HOSTFILE, --hostfile=HOSTFILE
                        host file, used when there is many host need run
                        command
  -w WRITEFILE, --write=WRITEFILE
                        store result of command executed, valid if using -T
                        option
  -i TIMEOUT, --timeout=TIMEOUT
                        timeout of each thread if using -T option

</pre>

<pre>

example:
1. upload file dns_node.conf 
./rpctool -H w-f01.tool.zzct.test.net -s /home/liuguirong/wire_selfcdn/dns_node.conf -d /home/liuguirong/dns_node.conf -t upload -u xxxxxx -p xxxxxxx
2. execute command
./rpctool -H w-f01.tool.zzct.test.net -u xxxxxx -p xxxxxxx -m "df -m"
3. execute many commands once
./rpctool -H w-f01.tool.zzct.test.net -u xxxxxx -p xxxxxxx -m "df -m" -m "ifconfig" -m "ls -l"
4. execute many commands once as sudo user
./rpctool -H w-f01.tool.zzct.test.net -u xxxxxx -p xxxxxxx -m "df -m" -m "ifconfig" -m "ls -l" -S
5. execute commands on many hosts on sequence
./rpctool -f node.conf -u xxxxxx -p xxxxxxx -m "df -m"
    node.conf content is:
    hostname1
    hostname2 
6. execute commands on many hosts at same time
./rpctool -f node.conf -u xxxxxx -p xxxxxxx -m "df -m" -T
    node.conf content is:
    hostname1
    hostname2 

</pre>
