#!/bin/bash

if [ $# == 0 ] ; then
   echo "default install"
   package="beacon"
else
   echo "intall "$1
   package=$1
fi

init_path=`pwd`
echo "install expect"
cd $init_path/repo
rpm -ivh tcl-8.5.7-6.el6.x86_64.rpm
rpm -ivh expect-5.44.1.15-5.el6_4.x86_64.rpm
cd ..

./gitclone 3rd-party
echo "install 3rd party"
cd 3rd-party

# install ruby
echo "install ruby1.9.3..."
tar -jxvf ruby-1.9.3-p0.tar.bz2 >>$init_path/install.log 2>&1
cd ruby-1.9.3-p0
./configure --prefix=/usr/local --enable-shared --disable-install-doc --with-opt-dir=/usr/local/lib >>$init_path/install.log 2>&1
make >>$init_path/install.log 2>&1
make install >>$init_path/install.log 2>&1
cd ..
rm -fr ruby-1.9.3-p0

cd gems
./install_gems >>$init_path/install.log 2>&1
cd ..

if [ $package == 'master' ] ; then
echo "install erlang..."
tar -zxvf otp_src_R16B01.tar.gz >>$init_path/install.log 2>&1
cd otp_src_R16B01 
./configure >>$init_path/install.log 2>&1
make >>$init_path/install.log 2>&1
make install >>$init_path/install.log 2>&1
cd ..
rm -rf otp_src_R16B01

echo "install rabbitmq..."
tar zxvf rabbitmq-server-generic-unix-3.1.3.tar.gz 
mv rabbitmq_server-3.1.3 /opt/
echo "export PATH=/opt/rabbitmq_server-3.1.3/sbin:\$PATH"  >> ~/.bashrc
. ~/.bashrc
rabbitmq-plugins enable  rabbitmq_management
rabbitmq-server -detached
cd ..
fi

cd ..
./gitclone baselib
cd baselib/ruby
./rebuild_and_install_gem
