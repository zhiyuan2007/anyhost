require 'json'
require 'beacon'
class TT
    def test_it
       @tt = Beacon::MiddleRest.new
       @group = "default"
       @ip = "202.173.9.28"
       @service = "Dumb"
       #r = @tt.send_cmd_to_node("systeminfo", {}, @group, @ip, "SystemInfo", 0)
       #puts "systeminfo!!#{r}"
       r = @tt.send_cmd_to_node("max", [10, 20], @group, @ip, "Dumb", 1)
       puts "max !!#{r}"
    end
end

TT.new.test_it
