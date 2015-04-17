require "forwardable"

module Beacon
module GRID
    class GridException < RuntimeError; end
    TOKENIZER = "."

    class GridManager
        include RB::Log
        extend Forwardable
        set_log_category "beacon_grid"
        def_delegators :@rpc_client, :insert_group_member_service
        def_delegators :@mb_mgr, :get_group_by_name, :get_group_by_member_ip
        def_delegators :@mb_mgr, :get_group_members, :get_member_by_name, :get_member_ip_by_id

        attr_reader :rpc_server, :rpc_client
        DEFAULT_GROUP = "default"

        def initialize(mq_server='127.0.0.1', db_path)
            @mb_mgr = MemberManager.deserialize
            while true
                begin
                    @rpc_client = RPCClient.new(self, mq_server, db_path)
                    @rpc_server = RPCServer.new(self, mq_server)
                rescue Exception => e
                    debug "init grid manager failed #{$!}, #{$@[0..10]}"
                    break unless e.to_s.include? ("UNEXPECTED_FRAME")
                end
                break if @rpc_server && @rpc_client
            end
        end

        def all_exchanges
            @rpc_client.all_info_inspect
        end

        def get_member_systeminfo(group, member_ip)
            group_send(group, member_ip, "SystemInfo", "systeminfo", {})
        end

        def get_asyn_result(id)
            @rpc_client.get_result(id)
        end

        def group_send(gname, mb_id, sname, command_name, msg)
            raise GridException.new("group: #{gname} not exists") unless @mb_mgr.has_group?(gname)
            debug "send: #{command_name} to #{gname}|#{mb_id}|#{sname} params: #{msg}"
            if mb_id == "*"
                member_ip = "*"
            else
                member = @mb_mgr.get_member_by_name(gname, mb_id)
                raise GridException.new("#{mb_id} not exists in group: #{gname}") unless member
                member_ip = member.ip
            end
            request = Command.new(command_name, msg)
            if request.syn
                response = @rpc_client.group_send_cmd(gname, member_ip, sname, request.to_json)
            else
                response = @rpc_client.group_send_asyn_cmd(gname, member_ip, sname, request.to_json)
            end
            debug "#{sname}|#{command_name} response: #{response}"
            response
        end

        def create_group(gname)
            raise GridException.new("can't create group: #{gname}") if gname == DEFAULT_GROUP
            raise GridException.new("group: #{gname} already exists") if @mb_mgr.has_group?(gname)
            @rpc_client.create_group(gname)
            @mb_mgr.create_group(gname)
        end

        def move_to_group(old_group, gname, params)
            raise GridException.new("group: #{old_group} not exists") unless @mb_mgr.has_group?(old_group)
            if !params.has_key?("id_list") || params["id_list"].empty?
                raise GridException.new("group: #{gname} already exists") if @mb_mgr.has_group?(gname)
                begin
                    @rpc_client.rename_group(old_group, gname)
                rescue RPCClientException
                    debug "Exception in group rename #{$!}, #{$@[0..10]}"
                ensure
                    @mb_mgr.rename_group(old_group, gname)
                end
            else
                raise GridException.new("group: #{gname} not exists") unless @mb_mgr.has_group?(gname)
                params["id_list"].each do |id|
                    member = @mb_mgr.get_member_by_name(old_group, id)
                    raise GridException.new("member: #{id} not in group: #{gname}") unless member
                    begin
                        @rpc_client.move_group_member(old_group, gname, member.ip)
                    rescue RPCClientException
                        debug "Exception in member move #{$!}, #{$@[0..10]}"
                    ensure
                        @mb_mgr.move_group_member(old_group, gname, member.ip)
                    end
                end
            end
        end

        def delete_group(gname)
            raise GridException.new("group: #{gname} not exists") unless @mb_mgr.has_group?(gname)
            @mb_mgr.del_group(gname)
            @rpc_client.delete_group(gname)
        end

        def groups
            @mb_mgr.groups
        end
        
        def update_group_member(old_group, old_id, new_group, new_id)
            raise GridException.new("group: #{new_group} not exists") unless @mb_mgr.has_group?(new_group)
            raise GridException.new("group: #{old_group} not exists") unless @mb_mgr.has_group?(old_group)
            raise GridException.new("member: #{old_id} not exists") unless @mb_mgr.get_member_by_name(old_group, old_id)
            @mb_mgr.update_group_member(old_group, old_id, new_id)
            if old_group != new_group
                params = {'id_list' => [new_id]}
                move_to_group(old_group, new_group, params)
            end
        end

        def insert_group_member(gname, server_ip, service_list)
            raise GridException.new("group: #{gname} not exists") unless @mb_mgr.has_group?(gname)
            service_list.each do |service|
                begin
                    @rpc_client.insert_group_member_service(gname, server_ip, service)
                rescue RPCClientException
                ensure
                    @mb_mgr.get_group_by_name(gname).add_service_to_member(server_ip, service)
                end
            end
        end

        def delete_group_member(group, mb_id)
            raise GridException.new("group: #{group} not exists") unless @mb_mgr.has_group?(group)
            raise GridException.new("mb:#{mb_id} not exists") unless @mb_mgr.get_member_by_name(group, mb_id)
            begin
                member = @mb_mgr.delete_group_member(group, mb_id)
            rescue RPCClientException
            ensure
                @rpc_client.delete_group_member(group, member.ip)
            end
        end

        def delete_group_member_service(group, mb_id, sname)
            raise GridException.new("group: #{group} not exists") unless @mb_mgr.has_group?(group)
            @rpc_client.delete_group_member_service(group, mb_id, sname)
            @mb_mgr.delete_group_member_service(group, mb_id, sname)
        end
        
        def get_queue_msg_count(mb_id, ser)
            raise GridException.new("#{mb_id}|#{ser} not exists") if !@mb_mgr.mb_and_ser_exists?(mb_id, ser)
            @rpc_server.get_q_msg_cnt(get_member_ip_by_id(mb_id), ser)
        end

        def purge_queue(mb_id, ser)
            raise GridException.new("#{mb_id}|#{ser} not exists") if !@mb_mgr.mb_and_ser_exists?(mb_id, ser)
            @rpc_client.purge_queue(get_member_ip_by_id(mb_id), ser)
            @rpc_server.purge_queue(get_member_ip_by_id(mb_id), ser)
        end
    end
end
end
