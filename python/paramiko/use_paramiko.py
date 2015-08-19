#!/usr/bin/env python
import sys
import threading
import socket
import paramiko
#=================================
# Class: PySSH
#=================================
class PySSH(object):
   
   
    def __init__ (self):
        self.ssh = None
        self.transport = None 
 
    def disconnect (self):
        if self.transport is not None:
           self.transport.close()
        if self.ssh is not None:
           self.ssh.close()
 
    def connect(self,hostname,username,password,port=22):
        self.hostname = hostname
        self.username = username
        self.password = password
 
        self.ssh = paramiko.SSHClient()
        #Don't use host key auto add policy for production servers
        self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.ssh.load_system_host_keys()
        try:
            self.ssh.connect(hostname,port,username,password)
            self.transport=self.ssh.get_transport()
        except (socket.error,paramiko.AuthenticationException) as message:
            print "ERROR: SSH connection to "+self.hostname+" failed: " +str(message)
            sys.exit(1)
        return  self.transport is not None
 
    def runcmd(self,cmd,sudoenabled=False):
        if sudoenabled:
            fullcmd="echo " + self.password + " |   sudo -S -p '' " + cmd
        else:
            fullcmd=cmd
        if self.transport is None:
            return "ERROR: connection was not established"
        session=self.transport.open_session()
        session.set_combine_stderr(True)
        #print "fullcmd ==== "+fullcmd
        if sudoenabled:
            session.get_pty()
        session.exec_command(fullcmd)
        stdout = session.makefile('rb', -1)
        #print stdout.read()
        output=stdout.read()
        session.close()
        return output
    def upload_file_and_exec(self, local_path, remote_path, filename):
       sftp = self.ssh.open_sftp()
       try:
           sftp.chdir(remote_path)  # Test if remote_path exists
       except IOError:
           sftp.mkdir(remote_path)  # Create remote_path
           sftp.chdir(remote_path)
       sftp.put(local_path + "/" + filename, remote_path + "/" + filename)
       sftp.close()

 
def get_nodes(filename):
   fp = open(filename, "r")
   nodes = []
   for line in fp.readlines():
       if line.startswith("#"):continue
       info = line.strip().split()
       nodes.append({'name':info[0], 'role':info[1]})
   fp.close()
   return nodes


#localpath="/home/q/system/"
remotepath="/home/liuguirong"
localpath="/usr/local/inerdns/tag"
filename="inerdns-all-0.0.0-29.tar.bz2"

#===========================================
# MAIN
#===========================================        

if __name__ == '__main__':
   nodes = get_nodes("./dns_node.conf")
   ths = []
   for node in nodes:
       print node
       hostname = node["name"]
       role = node["role"]
       username=''
       password=''
       #if role == "master":
       #   continue 

       ssh = PySSH()
       ssh.connect(hostname,username,password)
       #output=ssh.runcmd('date')
       #print output
       #ssh.upload_file_and_exec(localpath, remotepath, filename)
       #output=ssh.runcmd('tar jxPf /home/liuguirong/%s' % filename, True)
       ssh.disconnect()
