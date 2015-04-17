require 'beacon'
require 'json'
require 'test/unit'
require 'roadbike'

class GroupTest < Test::Unit::TestCase
    extend RB::UnitTestHelper

    def setup
        @tt = Beacon::MiddleRest.new
    end

    def teardown
        puts "one case over"
    end

    def test_group_del
        group = ARGV[0] || "testgroup0"
        @tt.del_group(group)
    end
    #run only specified test
    run_tests([:test_group_del])
end
