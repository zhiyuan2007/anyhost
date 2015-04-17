require "bunny"
require 'set'
require 'json'
require 'thread'
require 'timeout'
require 'securerandom'

module Beacon
module GRID
    class RPCClientException < RuntimeError; end
    class MQGroup 
        include RB::Log
        set_log_category "beacon_rpc_client"
        attr_accessor :members, :name, :exchange
        def initialize(group_name, conn)
            raise RPCClientException.new("group name can't nil") unless group_name
            @name = group_name
            @members = []
            @conn = conn 
            @exchange = @conn.topic(@name)
        end

        def real_queue_name(name, service)
            name + Beacon::GRID::TOKENIZER + service
        end
    
        def add_member(ip, service)
            real_qname = real_queue_name(ip, service)
            raise RPCClientException.new("member: #{real_qname} already exists") if get_member(ip, service)
            @members << {:ip => ip , :service => service} 
            @conn.queue(real_qname).bind(@exchange, :routing_key=>"#.#{service}")
        end

        def get_member_by_service(ip, service)
            if ip == "*"
                @members.select{|member| member[:service] == service}
            else
                @members.select{|member| member[:ip] == ip && member[:service] == service}
            end
        end

        def get_member(ip, service)
            @members.find {|member| member[:ip] == ip && member[:service] == service}
        end

        def get_member_owned_service(ip)
            @members.select{|member| member[:ip] == ip }
        end

        def del_member(ip, service)
            real_qname = real_queue_name(ip, service)
            raise RPCClientException.new("member: #{real_qname} not exist") unless get_member(ip, service)
            @conn.queue(real_qname).unbind(@exchange, :routing_key=>"#.#{service}")
            @members.delete_if {|elem| elem[:ip] == ip && elem[:service] == service}
        end

        def move_member_to_another_group(group, ip)
            get_member_owned_service(ip).each {|elem|
                group.add_member(elem[:ip], elem[:service])
                del_member(elem[:ip], elem[:service])
            }
        end

        def rename_to_new_group(new_group) 
            @members.each {|elem| 
                new_group.add_member(elem[:ip], elem[:service])
            }
            delete
        end

        def delete_member_by_ip(ip)
            get_member_owned_service(ip).each {|elem|
                del_member(elem[:ip], elem[:service])
            }
        end

        def delete
            @exchange.delete
        end

        def to_s
            "group: #{@name} members: #{@members}"
        end
    end

    class RPCClient 
        include RB::Log
        set_log_category "beacon_rpc_client"
        attr_reader :groups
        DEFAULT_GROUP = "default"
        DEFAULT_TIMEOUT = 3
        SYNC_QUEUE = "beacon_grid_rpc_response_queue"
        ASYNC_QUEUE = "beacon_grid_asyn_rpc_response_queue"

        def initialize(grid_manager, hostname, db_path)
            @conn = Beacon::Connection.new(hostname)
            @syn_sender = SynchronizeClient.new(@conn, SYNC_QUEUE)
            @asyn_sender = AsynchronizeClient.new(@conn, ASYNC_QUEUE, db_path)
            @default_x = @conn.default_exchange
            @groups = []
            _desieralize_mq(grid_manager)
        end

        def _desieralize_mq(grid_manager)
            grid_manager.groups.each do |group|
                create_group(group.name)
                group.members.each do |member|
                    member.services.each { |service|
                        insert_group_member_service(group.name, member.ip, service.name)
                    }
                end
            end
        end

        def get_result(uuid)
            @asyn_sender.get_result(uuid)
        end

        def _get_group_by_name(name)
            @groups.find {|group| group.name == name}
        end

        def create_group(name)
            raise RPCClientException.new("group #{name} exist") if _get_group_by_name(name)
            @groups << MQGroup.new(name, @conn)
        end

        def insert_group_member_service(group_name, member_ip, service)
            debug "insert into group: #{group_name}, member_ip: #{member_ip}, service: #{service}"
            group =  _get_group_by_name(group_name)
            raise RPCClientException.new("group #{group_name} not exists") unless group
            group.add_member(member_ip, service)
        end

        def group_send_cmd(group_name, member_ip, service, msg)
            group =  _get_group_by_name(group_name)
            raise RPCClientException.new("group #{group_name} not exists") unless group
            queue_name = group.real_queue_name(member_ip, service)
            @syn_sender.answers = group.get_member_by_service(member_ip, service).length
            response = @syn_sender.send_and_wait(group.exchange, "#.#{service}", msg) if member_ip == "*"
            response = @syn_sender.send_and_wait(@default_x, queue_name, msg) if member_ip != "*"
            response
        end

        def group_send_asyn_cmd(group_name, member_ip, service, msg)
            group =  _get_group_by_name(group_name)
            raise RPCClientException.new("group #{group_name} not exists") unless group
            queue_name = group.real_queue_name(member_ip, service)
            response = @asyn_sender.send_without_waiting(group.exchange,"#.#{service}", msg) if member_ip=="*"
            response = @asyn_sender.send_without_waiting(@default_x, queue_name, msg) if member_ip != "*"
            response
        end

        def delete_group(group_name)
            group = _get_group_by_name(group_name)
            raise RPCClientException.new("group #{group_name} not exists") unless group
            group.delete
            @groups.delete(group) if group_name != DEFAULT_GROUP
        end

        def delete_group_member_service(group_name, member_ip, service)
            debug "delete from group: #{group_name}, member_ip: #{member_ip}, service: #{service}"
            group =  _get_group_by_name(group_name)
            raise RPCClientException.new("group #{group_name} service:#{service} not exists") unless group
            group.del_member(member_ip, service)
        end

        def move_group_member(old_gname, new_gname, member_ip)
            debug "member #{member_ip} move to group: #{new_gname} from old group: #{old_gname}"
            old_group = _get_group_by_name(old_gname)
            raise RPCClientException.new("group #{old_gname} not exists") unless old_group
            new_group = _get_group_by_name(new_gname)
            raise RPCClientException.new("group #{new_gname} not exists") unless new_group
            old_group.move_member_to_another_group(new_group, member_ip)
            info "old #{old_gname}|#{old_group.members}, new #{new_gname}|#{new_group.members}}"
        end

        def rename_group(old_gname, new_gname)
            debug "rename group new group: #{new_gname} , old group: #{old_gname}"
            raise RPCClientException.new("group: #{old_gname} can't be reanme") if old_gname == DEFAULT_GROUP
            old_group = _get_group_by_name(old_gname)
            raise RPCClientException.new("group #{old_gname} not exists") unless old_group
            new_group = _get_group_by_name(new_gname)
            raise RPCClientException.new("group #{new_gname} already exists") if new_group
            new_group = MQGroup.new(new_gname, @conn)
            old_group.rename_to_new_group(new_group)
            @groups << new_group
            @groups.delete(old_group)
            info "now after rename all groups: #{all_info_inspect}"
        end

        def delete_group_member(group_name, member_ip)
            debug "delete member: #{member_ip} from group: #{group_name}"
            group = _get_group_by_name(group_name)
            raise RPCClientException.new("group #{group_name} not exists") unless group
            group.delete_member_by_ip(member_ip)
        end

        def get_q_msg_cnt(ip, service)
            queue_name = ip + Beacon::GRID::TOKENIZER + service
            if @conn.queue_exists?(queue_name)
                @conn.queue(queue_name).message_count
            else
                raise RPCClientException.new("queue: #{queue_name} not exists")
            end
        end

        def purge_queue(ip, service)
            queue_name = ip + Beacon::GRID::TOKENIZER + service
            if @conn.queue_exists?(queue_name)
                @conn.queue(queue_name).purge
            else
                raise RPCClientException.new("queue: #{queue_name} not exists")
            end
        end

        def all_info_inspect
            {"info" => @groups.inject(""){|info, group| info += "#{group.to_s}\n" }}
        end

        def stop
            @conn.close
        end
    end
end
end
