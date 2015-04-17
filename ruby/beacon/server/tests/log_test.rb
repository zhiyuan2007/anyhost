require "#{File.dirname(__FILE__)}/helper"
FileUtils.rm_rf("*.db")
Beacon::DB.init("members.db")

module Beacon
    class T1
        include Beacon::Log
        set_log_category "good"

        def do_log
            debug("this is debug from t1")
            info("this is info from t1")
        end
    end

    class T2
        include Beacon::Log
        set_log_category "good"


        def do_log
            debug("xxxxxxxxxxxx t2")
            info("xxxxxxxxxxxx t2")
        end
    end
end


module Beacon
module MockConfig
    def self.get(key)
        if key == "log_dir" 
            File.expand_path(File.dirname(__FILE__))
        elsif key == "log_to_stdout"
            false
        end
    end
end
end



class TestLog < Test::Unit::TestCase

    def setup
        Beacon::Log.init(Beacon::MockConfig)
    end

    def teardown
        FileUtils.rm_rf("good000001")
    end

    def test_log_into_same_file
        thread1 = Thread.new do
            4.times do
                Beacon::T2.new.do_log
            end
        end

        thread2 = Thread.new do
            4.times do
                Beacon::T1.new.do_log
            end
        end
        thread1.join
        thread2.join

        log_data = File.open("good000001", "r").read
        assert_equal(log_data.split("\n").select{|l| l.include?("t1")}.length, 8)
        assert_equal(log_data.split("\n").select{|l| l.include?("t2")}.length, 8)
    end
end
        
