require 'rubygems'
require 'test/unit'
require 'fileutils'
require 'roadbike'

lib_dir = File.join(File.expand_path(File.dirname(__FILE__)), "../lib")
$:.unshift lib_dir
require "#{lib_dir}/beacon.rb"
#Beacon::Log.forground!
