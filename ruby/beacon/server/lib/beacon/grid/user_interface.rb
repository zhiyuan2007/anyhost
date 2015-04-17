module Beacon
class UserInterfaceException < RuntimeError; end
class Interface
    INIT_QUEUE = "beacon_init_queue"

    include RB::Log
    set_log_category "beacon_user_interface"

    def initialize(service_name='default')
        @registerd_command = {}
        @s_name = service_name
        @conn = Beacon::Connection.new(Beacon::Config.get("rabbitmq_server"))
        @default_x = @conn.default_exchange 
        @my_ip = Beacon::SystemInfo.get_netcard_ip(Beacon::Config.get("nic_name"))
        raise UserInterfaceException.new("get local ip failed") if @my_ip.empty?

        register_result = _connect_to_beacon_server
        @callback_queue = @conn.queue(register_result['info']['queue'])
        @heartbeat_queue = @conn.queue(register_result['info']['heartbeat_queue'])
    end

    def _connect_to_beacon_server
        syn_sender = Beacon::SynchronizeClient.new(@conn, "#{@s_name}.registerd.queue") 
        msg = {:command_name=>'init', :command_args=>{'server' => @my_ip, 'service' => @s_name,\
               :user=>Beacon::Config.get("user"), :password=>Beacon::Config.get("password")}}
        info("service: #{@s_name} registered msg: #{msg}")
        result = JSON.load(syn_sender.send_and_wait(@default_x, INIT_QUEUE, msg))
        syn_sender.delete_queue
        info("service: #{@s_name} get response: #{result}")
        raise UserInterfaceException.new("register to master failed") if result["result"] == "failed"
        result
    end

    def register_command(command_name, function)
        @registerd_command[command_name] = function
        info("#{@s_name}|#{function} has been registerd, parameters #{function.parameters.to_json}")
    end

    def start_heartbeat_queue
        @heartbeat_queue.subscribe(:block => false) do |delivery_info, prop, payload|
            msg = JSON.load(payload)
            if msg["command_name"] == "heartbeat"
                res = JSON.generate({:result => "succeed", :_ip => @my_ip })
                @default_x.publish(res, :routing_key=>prop.reply_to, :correlation_id=>prop.correlation_id)
            end
        end
    end

    def run_command (block=true)
        while true
        begin
            start_heartbeat_queue
            @callback_queue.subscribe(:block => block, :ack => true) do |delivery_info, prop, payload|
                info("<<<service: #{@s_name} get command: #{payload}")
                msg = JSON.load(payload)
                comm_name = msg["command_name"]
                if @registerd_command.has_key?(comm_name)
                    begin
                    result = @registerd_command[comm_name].call(*msg["command_args"]["parameter"])
                    response = {:result => "succeed", :info => result, :_ip => @my_ip }
                    rescue
                    response = {:result=>"failed", :info=>"#{$!}:#{$@[0..3]}", :_ip=>@my_ip}.merge(msg)
                    end
                else
                    response = {:result=>"failed", :info=>"#{comm_name} not exists", :_ip=>@my_ip}.merge(msg)
                end
                res = JSON.generate(response)
                @conn.ack(delivery_info.delivery_tag)
                @default_x.publish(res, :routing_key=>prop.reply_to, :correlation_id=>prop.correlation_id)
                info(">>>#{@s_name}|#{comm_name} reply to #{prop.reply_to} #{response}")
            end
        rescue Exception => e
            debug "queue: #{@callback_queue.name} subscribe failed or func call error: #{$!}|#{$@[0..3]}"
            break unless e.to_s.include? ("UNEXPECTED_FRAME")
        end
        end
    end
end
end

