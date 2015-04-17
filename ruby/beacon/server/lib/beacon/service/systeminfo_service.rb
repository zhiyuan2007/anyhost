require 'beacon'

MY_IP = Beacon::SystemInfo.get_netcard_ip(Beacon::Config.get("nic_name"))
module Beacon
module Service 
    class SystemInfoService

        def initialize
            @interface = Beacon::Interface.new('SystemInfo')
            @interface.register_command("systeminfo", method(:sysinfo))
            Beacon::SystemInfo.start_background_info_collection
        end

        def sysinfo
            { 
                :cpu_usage    => Beacon::SystemInfo.get_cpu_useage,
                :memory_usage => Beacon::SystemInfo.get_mem_useage,
                :disk_usage   => Beacon::SystemInfo.get_disk_useage,
                :cpu_temp     => Beacon::SystemInfo.get_cpu_temp,
            }
        end

        def run
            @interface.run_command
        end

        def self.run
            service = SystemInfoService.new
            service.run
        end
    end
end
end
