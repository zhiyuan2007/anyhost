require 'beacon'
require 'json'
require 'test/unit'
require 'roadbike'

nic_name = "eth1"
LOCAL_IP = Beacon::SystemInfo.get_netcard_ip(nic_name)
SECOND_IP = '202.173.9.29'
puts "-------------------"
puts "test on nic #{nic_name} with ip #{LOCAL_IP}"
puts "-------------------"

LOC_INDEX = LOCAL_IP < SECOND_IP ? 0 : 1
SEC_INDEX = LOCAL_IP > SECOND_IP ? 0 : 1

TEST_NUM = 10
class GroupTest < Test::Unit::TestCase
    extend RB::UnitTestHelper

    def setup
        @tt = Beacon::MiddleRest.new
    end

    def teardown
        puts "one case over"
    end

    def test_group_create_and_delete
        TEST_NUM.times {|i|
            group = "testgroup" + i.to_s
            @tt.add_group(group)
            result = JSON.load(@tt.get_nodes)
            puts result
            assert_equal(2, result.length)
            assert_raise Beacon::MiddleRestException do
                @tt.add_group(group)
            end

            @tt.del_group!(group)
            result = JSON.load(@tt.get_nodes)
            assert_equal(1, result.length)
            assert_equal("default", result[0]["id"])

            assert_raise Beacon::MiddleRestException do
                @tt.del_group!(group)
            end
        }
    end

    def test_group_move
        TEST_NUM.times {|i|
            group = "testgroup" + i.to_s
            @tt.add_group(group)
            result = JSON.load(@tt.get_nodes)
            assert_equal(2, result.length)

            result = @tt.get_nodes(true)
            assert(result.include?("cpu_usage"))
            result = @tt.get_nodes(false)
            assert(!result.include?("cpu_usage"))

            assert_raise Beacon::MiddleRestException do
               @tt.add_group(group)
            end

            @tt.add_node(group, LOCAL_IP)
            result = JSON.load(@tt.get_nodes)
            assert_equal(2, result.length)
            assert_equal("default", result[0]["id"])
            assert_equal(group, result[1]["id"])
            assert_equal(LOCAL_IP, result[1]["members"][LOC_INDEX]["id"])

            @tt.add_node(group, SECOND_IP)
            result = JSON.load(@tt.get_nodes)
            assert_equal(2, result.length)
            assert_equal("default", result[0]["id"])
            assert_equal(group, result[1]["id"])
            assert_equal(LOCAL_IP, result[1]["members"][LOC_INDEX]["id"])
            assert_equal(SECOND_IP, result[1]["members"][SEC_INDEX]["id"])

            assert_raise Beacon::MiddleRestException do
                @tt.add_node(group, LOCAL_IP)
            end

            assert_raise Beacon::MiddleRestException do
                @tt.add_node(group, SECOND_IP)
            end

            result = @tt.send_cmd_to_node("max", [10, 20], group, LOCAL_IP, "Dumb") #info was function result 
            result = JSON.load(result)
            assert_equal("20", result["info"].to_s)
            assert_equal(result["_ip"], LOCAL_IP)

            json =  [{"foo"=>"bar", "zdns" => ["maple", "zlope"]}, {"hello" => "world", "knet" => ["gtld"]}]
            r = @tt.send_cmd_to_node("pj", json, group, LOCAL_IP, "Dumb")
            response = JSON.load(r)
            puts "json !!#{response}"
            assert_equal(response["info"].class, Hash)
            assert_equal(response["info"]["foo"], "bar")
            assert_equal(response["info"]["zdns"], ["maple", "zlope"])
            assert_equal(response["info"]["hello"], "world")
            assert_equal(response["info"]["knet"], ["gtld"])

            r = @tt.send_cmd_to_node("hw", [], group, LOCAL_IP, 'Dumb')
            response = JSON.load(r)
            puts "hello !!#{response}"
            assert_equal(response["info"], "hello world")

            para = ["this is a echo information, thanks!!!"]
            r = @tt.send_cmd_to_node("echo", para, group, LOCAL_IP, "Dumb")
            response = JSON.load(r)
            puts "echo !!#{response}"
            assert_equal(response["info"], para[0])


            result = @tt.send_cmd_to_node("max", [10, 20], group, "*", "Dumb") #info was function result 
            result = JSON.load(result)
            assert_equal("20", result[0]["info"].to_s)
            assert_equal("20", result[1]["info"].to_s)

            outs = `rabbitmqctl list_exchanges`
            puts outs
            puts "current group: #{group}"
            assert(outs.include?(group))

            @tt.del_group(group)
            result = JSON.load(@tt.get_nodes)
            assert_equal(1, result.length)
            assert_equal("default", result[0]["id"])
            assert_equal(LOCAL_IP, result[0]["members"][LOC_INDEX]["id"])
            assert_equal(SECOND_IP, result[0]["members"][SEC_INDEX]["id"])

        }
    end
    def test_group_rename
        @asyn_uuids = []
        group = "testgroup0"
        @tt.add_group(group)
        result = JSON.load(@tt.get_nodes)
        assert_equal(2, result.length)

        @tt.add_node(group, LOCAL_IP)
        result = JSON.load(@tt.get_nodes)
        assert_equal(2, result.length)
        assert_equal("default", result[0]["id"])
        assert_equal(group, result[1]["id"])

        @tt.add_node(group, SECOND_IP)
        result = JSON.load(@tt.get_nodes)
        assert_equal(2, result.length)
        assert_equal("default", result[0]["id"])
        assert_equal(group, result[1]["id"])

        assert_raise Beacon::MiddleRestException do
            @tt.add_node(group, LOCAL_IP)
        end
        assert_raise Beacon::MiddleRestException do
            @tt.add_node(group, SECOND_IP)
        end

        TEST_NUM.times {|i|
            old_group = "testgroup" + i.to_s
            new_group = "testgroup" + (i+1).to_s
            @tt.rename_group(old_group, new_group)
            result = JSON.load(@tt.get_nodes)
            puts "________#{result}"
            assert_equal(2, result.length)
            assert_equal("default", result[0]["id"])
            assert_equal(new_group, result[1]["id"])
            assert_equal(LOCAL_IP, result[1]["members"][LOC_INDEX]["id"])

            result = @tt.send_cmd_to_node("max", [10, 20], new_group, LOCAL_IP, "Dumb") #info was function result 
            result = JSON.load(result)
            assert_equal("20", result["info"].to_s)

            result = @tt.send_cmd_to_node("max", [10, 20], new_group, SECOND_IP, "Dumb") #info was function result 
            result = JSON.load(result)
            assert_equal("20", result["info"].to_s)

            result = @tt.send_cmd_to_group("max", [10, 20], new_group, "Dumb", 0) #asyn
            result = JSON.load(result)
            @asyn_uuids << result["id"]

            json =  [{"foo"=>"bar", "zdns" => ["maple", "zlope"]}, {"hello" => "world", "knet" => ["gtld"]}]
            r = @tt.send_cmd_to_node("pj", json, new_group, SECOND_IP, "Dumb")
            response = JSON.load(r)
            puts "json !!#{response}"
            assert_equal(response["info"].class, Hash)
            assert_equal(response["info"]["foo"], "bar")
            assert_equal(response["info"]["zdns"], ["maple", "zlope"])
            assert_equal(response["info"]["hello"], "world")
            assert_equal(response["info"]["knet"], ["gtld"])
            assert_equal(response["_ip"], SECOND_IP)

            r = @tt.send_cmd_to_node("hw", [], new_group, SECOND_IP, 'Dumb')
            response = JSON.load(r)
            puts "hello !!#{response}"
            assert_equal(response["info"], "hello world")

            para = ["this is a echo information, thanks!!!"]
            r = @tt.send_cmd_to_node("echo", para, new_group, SECOND_IP, "Dumb")
            response = JSON.load(r)
            puts "echo !!#{response}"
            assert_equal(response["info"], para[0])

            result = @tt.send_cmd_to_node("max", [10, 20], new_group, "*", "Dumb") #info was function result 
            result = JSON.load(result)
            assert_equal("20", result[0]["info"].to_s)
            assert_equal("20", result[1]["info"].to_s)

            outs = `rabbitmqctl list_exchanges`
            assert(outs.include?(new_group))
            group = new_group
        }
        @tt.del_group(group)
        result = JSON.load(@tt.get_nodes)
        assert_equal(1, result.length)
        assert_equal("default", result[0]["id"])
        assert_equal(LOCAL_IP, result[0]["members"][LOC_INDEX]["id"])

        sleep 1
        @asyn_uuids.each {|id|
          response =  JSON.load(@tt.get_asyn_result(id))
          puts "#{response}"
          assert_equal("20", response[0]["info"].to_s)
          assert_equal("20", response[1]["info"].to_s)
        }
    end

    def test_update_group_member
        new_member_id = "update_member"
        new_group_id = "group_for_update"

        loc_index = new_member_id < SECOND_IP ? 0 : 1

        TEST_NUM.times {|i|
            @tt.update_group_member("default", LOCAL_IP,  "default", new_member_id)
            result = JSON.load(@tt.get_nodes)
            assert_equal(1, result.length)
            assert_equal(result[0]["members"][loc_index]["id"], new_member_id)
            assert_raise Beacon::MiddleRestException do
               @tt.update_group_member("default", LOCAL_IP,  "default", new_member_id)
            end

            @tt.add_group(new_group_id)
            result = JSON.load(@tt.get_nodes)
            assert_equal(2, result.length)
            assert_raise Beacon::MiddleRestException do
                @tt.add_group(new_group_id)
            end

            @tt.update_group_member("default",  new_member_id, new_group_id, LOCAL_IP)
            result = JSON.load(@tt.get_nodes)
            puts "after update groups: #{result[1]["id"]}"
            assert_equal(2, result.length)
            assert_equal(result[1]["members"][LOC_INDEX]["id"], LOCAL_IP)

            assert_raise Beacon::MiddleRestException do
               @tt.update_group_member("default",  new_member_id, new_group_id, LOCAL_IP)
            end

            @tt.del_group(new_group_id)
        }
    end

    def test_delete_group_member
        TEST_NUM.times {|i|
            @tt.delete_group_member("default",  LOCAL_IP)
            result = JSON.load(@tt.get_nodes)
            assert_equal(1, result[0]["members"].length)
            assert_raise Beacon::MiddleRestException do
                @tt.delete_group_member("default",  LOCAL_IP)
            end

            @tt.insert_group_member("default", LOCAL_IP, ["Dumb","SystemInfo"])
            result = JSON.load(@tt.get_nodes)
            assert_equal(2, result[0]["members"].length)
            assert_equal(LOCAL_IP, result[0]["members"][LOC_INDEX]['id'])
            assert_equal(2, result[0]["members"][0]["services"].length)
            assert_raise Beacon::MiddleRestException do
                @tt.insert_group_member("default", LOCAL_IP, ["Dumb","SystemInfo"])
            end

            @tt.delete_group_member("default",  SECOND_IP)
            result = JSON.load(@tt.get_nodes)
            assert_equal(1, result[0]["members"].length)
            assert_raise Beacon::MiddleRestException do
                @tt.delete_group_member("default",  SECOND_IP)
            end
            @tt.insert_group_member("default", SECOND_IP, ["Dumb","SystemInfo"])
            result = JSON.load(@tt.get_nodes)
            puts result
            puts "sec: #{SEC_INDEX}"
            assert_equal(2, result[0]["members"].length)
            assert_equal(SECOND_IP, result[0]["members"][SEC_INDEX]['id'])
            assert_raise Beacon::MiddleRestException do
                @tt.insert_group_member("default", SECOND_IP, ["Dumb","SystemInfo"])
            end


            @tt.delete_group_member("default",  LOCAL_IP)
            result = JSON.load(@tt.get_nodes)
            assert_equal(1, result[0]["members"].length)

            @tt.delete_group_member("default",  SECOND_IP)
            result = JSON.load(@tt.get_nodes)
            assert_equal(0, result[0]["members"].length)

            @tt.insert_group_member("default", LOCAL_IP, ["Dumb","SystemInfo"])
            @tt.insert_group_member("default", SECOND_IP, ["Dumb","SystemInfo"])
            result = JSON.load(@tt.get_nodes)
            assert_equal(2, result[0]["members"].length)
            assert_equal(LOCAL_IP, result[0]["members"][LOC_INDEX]['id'])
            assert_equal(SECOND_IP, result[0]["members"][SEC_INDEX]['id'])
        }
    end

    def test_group_system_info
        result = @tt.get_nodes(true)
        assert(result.include?("cpu_usage"))
        assert(result.include?("memory_usage"))
        assert(result.include?("disk_usage"))
        assert(result.include?("cpu_temp"))

        result = @tt.get_nodes(false)
        assert(!result.include?("cpu_usage"))

        result = @tt.get_group_nodes("default", true)
        assert(result.include?("cpu_usage"))
        assert(result.include?("memory_usage"))
        assert(result.include?("disk_usage"))
        assert(result.include?("cpu_temp"))

        result = @tt.get_group_nodes("default", false)
        assert(!result.include?("cpu_usage"))

        result = @tt.get_group_node("default", LOCAL_IP, true)
        assert(result.include?("cpu_usage"))
        assert(result.include?("memory_usage"))
        assert(result.include?("disk_usage"))
        assert(result.include?("cpu_temp"))

        result = @tt.get_group_node("default", LOCAL_IP, false)
        assert(!result.include?("cpu_usage"))

        result = @tt.get_group_node("default", SECOND_IP, true)
        assert(result.include?("cpu_usage"))
        assert(result.include?("memory_usage"))
        assert(result.include?("disk_usage"))
        assert(result.include?("cpu_temp"))

        result = @tt.get_group_node("default", SECOND_IP, false)
        assert(!result.include?("cpu_usage"))

    end

    #def test_group_del
    #    group = "testgroup0"
    #    @tt.del_group(group)
    #end

    #run only specified test
    #run_tests([:test_group_create_and_delete])
end
