require 'socket'

module Beacon
    module SystemInfo
        REFRESH_INTERVAL = 3

        def self.start_background_info_collection
            @backgroud_thread ||= Thread.new {
                while true 
                    @cpu = self._get_cpu_useage
                    @mem = self._get_mem_useage
                    @temp = self._get_cpu_temp 
                    @disk_usage = self._get_disk_useage
                    sleep REFRESH_INTERVAL
                end
            }
        end

        def self.local_public_ips
            Socket.ip_address_list.select do |intf| 
                intf.ipv4? and !intf.ipv4_multicast? and !intf.ipv4_loopback? 
            end.map{|addr_info| addr_info.ip_address}
        end

        def self.get_host_name
            @hostname ||= `hostname`.strip
        end

        def self.get_cpu_useage
            @cpu || 0
        end

        def self._get_cpu_useage
            cpu_idle = `vmstat 1 2 | tail -n 1 | awk '{print $15}'`
            cpu_usage = (cpu_idle.nil? || cpu_idle.strip.empty?) ? 0 : (100 - cpu_idle.to_i)
            format("%.2f", cpu_usage.to_f / 100.to_f)
        end
  
        def self.get_mem_useage
            @mem || 0 
        end

        def self._get_mem_useage
            cmd_ret = IO.popen("free") {|stdin| stdin.readlines}
            total_mem_info = cmd_ret[1].split(" ").select{|num| num.length > 0}  
            total_mem = total_mem_info[1].to_i
            buffer_mem_info = cmd_ret[2].split(" ").select{|num| num.length > 0}  
            free_mem = buffer_mem_info[3].to_i
            use_mem = (total_mem - free_mem).to_f / total_mem
            format("%.2f", use_mem)
        end

        def self.get_disk_useage(content=0)
            @disk_usage || 0
        end

        def self._get_disk_useage(content=0)
            sh_rs = `df -l `.to_s
            sh_rs.split("\n").each do |line|
                arr = line.split("\s")
                content = arr[-2].to_i if "/" == arr[-1]
            end
            format("%.2f", content / 100.0)
        end


        def self.get_cpu_temp
            @temp || 0
        end

        def self._get_cpu_temp
            `sensors 2>/dev/null` =~ /Core\s0:\s+\+(\d+)/
            $1.to_i
        end

        def self.get_netcard_ip(ethn)
            os = RUBY_PLATFORM.split("-").last
            if os.include?("linux")
                `ifconfig #{ethn}|grep "inet addr"|awk '{print $2}'|awk -F ':' '{print $2}'`[0..-2]
            elsif os.include?("darwin")
                `ifconfig #{ethn}|grep -w "inet"|awk '{print $2}'`.chomp
            end
        end

    end

end

