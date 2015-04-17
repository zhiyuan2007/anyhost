require 'curb'
require 'json'
require 'beacon'
require 'test/unit'
class CallBackTest < Test::Unit::TestCase
    def setup
       @my = Beacon::MYCURL.new("127.0.0.1")
       @group = "default"
       @member = '*'
       @ip = "202.173.9.28"
       @service = "Dumb"
       @params = {"parameter"=>[10, 20]}
    end

    def teardown
        puts ">>>notice: user interface test pass..."
    end

    def tests
       r = @my.group_send_cmd(@group, @ip, @service, "max", @params)
       response = JSON.load(r.to_json)
       puts "max !!#{response}"
       assert_equal(response["info"].to_s, "20")
       assert_equal(response["_ip"], @ip)

       json =  {'parameter' => [{"foo"=>"bar", "zdns" => ["maple", "zlope"]}, {"hello" => "world", "knet" => ["gtld"]}] }
       r = @my.group_send_cmd(@group, @ip, @service, "pj", json)
       response = JSON.load(r.to_json)
       puts "json !!#{response}"
       assert_equal(response["info"].class, Hash)
       assert_equal(response["info"]["foo"], "bar")
       assert_equal(response["info"]["zdns"], ["maple", "zlope"])
       assert_equal(response["info"]["hello"], "world")
       assert_equal(response["info"]["knet"], ["gtld"])

       para =  {'parameter' => []}
       r = @my.group_send_cmd(@group, @ip, @service, "hw", para)
       response = JSON.load(r.to_json)
       puts "hello !!#{response}"
       assert_equal(response["info"], "hello world")

       para =  {'parameter' => ["this is a echo information, thanks!!!"]}
       r = @my.group_send_cmd(@group, @ip, @service, "echo", para)
       response = JSON.load(r.to_json)
       puts "echo !!#{response}"
       assert_equal(response["info"], para['parameter'][0])

       r = @my.group_send_cmd(@group, @member, @service, "min", @params)
       puts "body str: #{r.to_json}"
       response = JSON.load(r.to_json)
       puts "min !!#{response}, #{response[0].class}"
       assert_equal(response[0]["info"].to_s, "10")

       r = @my.group_send_cmd(@group, @member, @service, "min", @params.merge({"syn"=>"no"}))
       puts "body str: #{r.to_json} #{r.class}"
       response = JSON.load(r.to_json)
       puts "asyn  result id: #{response["id"]}"

       r = @my.group_send_cmd(@group, @member, @service, "min", @params.merge({"syn"=>"no"}))
       response1 = JSON.load(r.to_json)
       puts "asyn  result id: #{response1["id"]}"
       r = @my.group_send_cmd(@group, @member, @service, "min", @params.merge({"syn"=>"no"}))
       response2 = JSON.load(r.to_json)
       puts "asyn  result id: #{response2["id"]}"
       sleep 2
       r = @my.get_result(response["id"])
       response = JSON.load(r.to_json)
       puts "actual result: #{response}"
       assert_equal(response[0]["info"], "10")

       r = @my.get_result(response1["id"])
       response = JSON.load(r.to_json)
       puts "actual result: #{response}"
       assert_equal(response[0]["info"], "10")

       r = @my.get_result(response2["id"])
       response = JSON.load(r.to_json)
       puts "actual result: #{response}"
       assert_equal(response[0]["info"], "10")

       r = @my.group_send_cmd(@group, @member, @service, "min", {"a"=>10})
       response = JSON.load(r.to_json)
       assert_equal(response[0]['result'], 'failed')

       r = @my.get_groups
       response = JSON.load(r.to_json)
       puts "groups: #{response}, class: #{response.class}"
       assert_equal(response[0]['id'], @group)

       json =  {'parameter' => [{"foo"=>"bar"*10000, "zdns" => ["maple", "zlope"]}, {"hello" => "world"*10000, "knet" => ["gtld"]}] }
       r = @my.group_send_cmd(@group, @ip, @service, "pj", json)
       response = JSON.load(r.to_json)
       puts "json !!#{response}"
    end
end
