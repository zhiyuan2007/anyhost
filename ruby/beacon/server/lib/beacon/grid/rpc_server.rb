require "bunny"
require "json"
require 'securerandom'

module Beacon
module GRID
    class HeartbeatService
        attr_accessor :name, :binding_client, :response_client
        def initialize(name, conn)
            @name = name
            @conn = conn
            @uuid = nil
            @binding_client = []
            @response_client = []
            @exchange = @conn.fanout(@name)
            @reply_to = @name + "_heartbeat_exclusive_reply_to"
        end

        def send_heartbeat_cmd
            command = {:command_name => 'heartbeat', :command_args => {}}
            @response_client.clear
            @uuid = SecureRandom.uuid
            @exchange.publish(JSON.generate(command), :correlation_id => @uuid, :reply_to => @reply_to)
        end

        def start_heartbeat_reply
            @conn.queue(@reply_to, true).subscribe(:block => false) do |delivery_info, properties, payload|
                msg = JSON.load(payload)
                @response_client << msg["_ip"] if properties[:correlation_id] == @uuid
            end
        end

        def add_heartbeat_member(server_ip)
            unless @binding_client.include?(server_ip)
                @binding_client << server_ip 
                heartbeat_queue = server_ip + TOKENIZER + @name + TOKENIZER + 'heartbeat'
                @conn.queue(heartbeat_queue).bind(@exchange)
                start_heartbeat_reply if @binding_client.length == 1
            end
        end
    end

    class RPCServerException < RuntimeError; end
    class RPCServer
        include RB::Log
        set_log_category "beacon_rpc_server"
        HEARTBEAT_INTERVAL = 5 
        TOKENIZER = '.'
        DEFAULT_GROUP = "default"
        G_NAME = "beacon"
        G_PASSWORD = "beacon"
        INIT_QUEUE = "beacon_init_queue"

        def initialize(grid_manager, hostname)
            @conn = Beacon::Connection.new(hostname)
            @default_x = @conn.default_exchange
            @grid_manager = grid_manager
            @services = []
            desieralize_heartbeat
            start(INIT_QUEUE)
            start_heartbeat_check
        end

        def desieralize_heartbeat
            @grid_manager.groups.each do |group|
                group.members.each do |member|
                    member.services.each { |service| add_service_heartbeat(service.name, member.ip) }
                end
            end
        end

        def _deal_heatbeat_response
            @services.each do |service|
                @grid_manager.groups.each do |group|
                    group.members.each do |member|
                        if service.response_client.include?(member.ip)
                            member.online
                            member.get_service_by_name(service.name).online
                        end
                    end
                end
                (service.binding_client - service.response_client).each{ |e|
                    @grid_manager.groups.each do |group|
                        member = group.members.find { |member| member.ip == e}
                        member.get_service_by_name(service.name).offline if member
                        break if member
                    end
                }
            end
        end

        def _send_heartbeat_cmd
            @services.each { |service| service.send_heartbeat_cmd }
        end

        def start_heartbeat_check
            Thread.new do
                while true
                    _send_heartbeat_cmd
                    sleep HEARTBEAT_INTERVAL
                    _deal_heatbeat_response
                end
            end
        end

        def get_service_by_name(service_name)
            @services.find {|ser| ser.name == service_name}
        end

        def add_service_heartbeat(service_name, server_ip)
            service = get_service_by_name(service_name)
            unless service
                service = HeartbeatService.new(service_name, @conn) 
                @services << service
            end
            service.add_heartbeat_member(server_ip)
        end
        
        def to_symblize(hash)
            Hash[hash.map{|k, v| [k.to_sym, v]}]
        end

        def start(queue_name)
            @init_queue = @conn.queue(queue_name, true)
            @init_queue.subscribe(:block => false) do |delivery_info, properties, payload|
                debug "receive msg: #{payload} in queue:#{INIT_QUEUE}"
                msg = to_symblize(JSON.load(payload))
                if msg[:command_name] == "init"
                    if !msg[:command_args].has_key?("user") || !msg[:command_args].has_key?("password") ||
                        msg[:command_args]["user"] != G_NAME || msg[:command_args]["user"] != G_PASSWORD
                        response = {:result => "failed", :info => "user or password not correct"}
                    else
                        server_ip = msg[:command_args]["server"]
                        service = msg[:command_args]["service"]
                        add_service_heartbeat(service, server_ip)
                        begin
                            group = @grid_manager.get_group_by_member_ip(server_ip)
                            group.add_service_to_member(server_ip, service)
                            @grid_manager.insert_group_member_service(group.name, server_ip, service)
                        rescue
                            debug "#{$!} #{$@[0..10]}"
                        end
                        queue = server_ip + TOKENIZER + service
                        heartbeat_queue = server_ip + TOKENIZER + service + TOKENIZER + 'heartbeat'
                        response = {:result => "succeed", \
                                    :info => {:queue => queue, :heartbeat_queue => heartbeat_queue}}
                    end
                end
                debug "response from init queue was: #{response}"
                @default_x.publish(JSON.generate(response), :routing_key => properties.reply_to,
                                   :correlation_id => properties.correlation_id)
            end
        end

        def get_q_msg_cnt(ip, service)
            queue_name = ip + TOKENIZER + service + TOKENIZER + 'heartbeat'
            if @conn.queue_exists?(queue_name)
                @conn.queue(queue_name).message_count
            else
                raise RPCClientException.new("queue: #{queue_name} not exists")
            end
        end

        def purge_queue(ip, service)
            queue_name = ip + TOKENIZER + service + TOKENIZER + 'heartbeat'
            if @conn.queue_exists?(queue_name)
                @conn.queue(queue_name).purge
            else
                raise RPCClientException.new("queue: #{queue_name} not exists")
            end
        end

        def stop
            @conn.close
        end
    end
end
end
