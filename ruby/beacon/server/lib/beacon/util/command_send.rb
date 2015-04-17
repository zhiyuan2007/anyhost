#require 'curb'
require 'httparty'
module Beacon
    class MYCURL
        def initialize(server)
            @server = server
            @url = "http://#{@server}:5678"
        end

        def group_create(name)
            curl_send_json(@url + "/groups", {"name"=>name}, "post")
        end

        def group_move(old_group, new_group, node_id)
            params = {"name"=>new_group, "id_list"=>[node_id]}
            curl_send_json(@url + "/groups/#{old_group}", params, "put")
        end
    
        def group_delete(group)
            HTTParty.delete(@url + "/groups/#{group}")
        end

        def group_rename(old_group, new_group)
            params = {"name"=>new_group, "id_list"=>[]}
            curl_send_json(@url + "/groups/#{old_group}", params, "put")
        end
    
        def group_send_cmd(group, member, service, command, params)
            curl_send_json(@url + "/groups/#{group}/members/#{member}/services/#{service}/commands/#{command}", params, "post")
        end
    
        def get_result(uuid)
            HTTParty.get(@url + "/results/#{uuid}")
        end
    
        def get_groups(need_info = false)
            HTTParty.get(@url + "/groups?need_info=#{need_info}")
        end

        def get_group_members(group_name, need_info =false)
            HTTParty.get(@url + "/groups/#{group_name}?need_info=#{need_info}")
        end

        def get_group_member(group_name, member_id, need_info = false)
            HTTParty.get(@url + "/groups/#{group_name}/members/#{member_id}?need_info=#{need_info}")
        end

        def get_members(group_name)
            HTTParty.get(@url + "/groups/#{group_name}")
        end

        def get_member_from_group(group, ip)
            HTTParty.get(@url + "/groups/#{group}/members/#{ip}")
        end

        def insert_group_member(group_name, member_id, service_list)
            params = {"service_list"=>service_list, "member_id"=>member_id}
            curl_send_json(@url + "/groups/#{group_name}/members", params, "post")
        end

        def update_group_member(old_group, old_id, new_group, new_id)
            params = {"new_group"=> new_group, "new_id"=>new_id}
            curl_send_json(@url + "/groups/#{old_group}/members/#{old_id}", params, "put")
        end

        def delete_group_member(group, id)
            HTTParty.delete(@url + "/groups/#{group}/members/#{id}")
        end

        def curl_send_json(url, param, method, timeout=600)
            param = param.to_json if param && !param.kind_of?(String)

            body = {:body => param, :timeout => timeout,
                    :headers => {'Content-Type' => 'application/json', 'Accept' => 'application/json'}}
            if method.downcase == "post"
                HTTParty.post(url, body)
            elsif method.downcase == "put"
                HTTParty.put(url, body)
            end
        end
    end

    class MiddleRestException < Exception; end
    class MiddleRest 
        attr_accessor :serv_ip
        DEF_MSGQ_IP = "127.0.0.1"
        def initialize(serv_ip = DEF_MSGQ_IP)
            @serv_ip = serv_ip
            @my_curl = Beacon::MYCURL.new(@serv_ip)
        end

        def add_group(group_name)
            result = @my_curl.group_create(group_name)
            raise MiddleRestException.new(result) unless result.success?
            result.to_json
        end
 
        def del_group(group_name)
            members = @my_curl.get_members(group_name).to_json
            members = JSON.load(members)["members"]
            raise MiddleRestException.new("not members") unless members
            members.each{|member|
               result = @my_curl.group_move(group_name, "default",  member["ip"])
               raise MiddleRestException.new(result) unless result.success?
            }
            result = @my_curl.group_delete(group_name)
            raise MiddleRestException.new(result) unless result.success?
            result.to_json
        end

        def del_group!(group_name)
            result = @my_curl.group_delete(group_name)
            raise MiddleRestException.new(result) unless result.success?
            result.to_json
        end

        def add_node(group_name, node_ip)
            result = @my_curl.group_move("default", group_name, node_ip)
            raise MiddleRestException.new(result) unless result.success?
            result.to_json
        end

        def del_node(group_name, node_ip)
            result = @my_curl.group_move(group_name, "default", node_ip)
            raise MiddleRestException.new(result) unless result.success?
            result.to_json
        end

        def rename_group(old_group, new_group)
            result = @my_curl.group_rename(old_group, new_group)
            raise MiddleRestException.new(result) unless result.success?
            result.to_json
        end

        def get_nodes(need_info = false)
            @my_curl.get_groups(need_info).to_json
        end

        def get_group_nodes(group, need_info = false)
            @my_curl.get_group_members(group, need_info).to_json
        end

        def get_group_node(group, ip, need_info = false)
            @my_curl.get_group_member(group, ip, need_info).to_json
        end

        def get_node_status(group, ip)
            result = @my_curl.get_member_from_group(group, ip)
            raise MiddleRestException.new(result) unless result.success?
            result.to_json
        end

        def update_group_member(old_group, old_id, new_group, new_id)
            result = @my_curl.update_group_member(old_group, old_id, new_group, new_id)
            raise MiddleRestException.new(result) unless result.success?
            result.to_json
        end

        def delete_group_member(group, id)
            result = @my_curl.delete_group_member(group, id)
            raise MiddleRestException.new(result) unless result.success?
            result.to_json
        end

        def insert_group_member(group, id, service_list)
            result = @my_curl.insert_group_member(group, id, service_list)
            raise MiddleRestException.new(result) unless result.success?
            result.to_json
        end
 
        def is_servered?(ip)
            result = JSON.load(@my_curl.get_groups.to_s)
            result.each { |group|
                group["members"].each {|member|
                    return true if member["ip"] == ip
                }
            }
            false
        end
        def get_asyn_result(id)
            result = @my_curl.get_result(id)
            raise MiddleRestException.new(result) unless result.success?
            result.to_json
        end
 
        def send_cmd_to_group(cmd_name, cmd_params, group_name, service_name, is_sync = 1)
            is_sync == 1 ? syn = "yes" : syn = "no"
            params = {"parameter"=>cmd_params}.merge({"syn"=>syn})
            result = @my_curl.group_send_cmd(group_name, "*", service_name, cmd_name, params)
            raise MiddleRestException.new(result) unless result.success?
            result.to_json
        end

        def send_cmd_to_node(cmd_name, cmd_params, group_name, node_ip,  service_name, is_sync = 1)
            is_sync == 1 ? syn = "yes" :  syn="no"
            params = {"parameter"=>cmd_params}.merge({"syn"=>syn})
            result = @my_curl.group_send_cmd(group_name, node_ip, service_name, cmd_name, params)
            raise MiddleRestException.new(result) unless result.success?
            result.to_json
        end

        def get_total_qps(member = "*")
            total_qps = 0
            JSON.load(get_nodes).each {|group|
                result = send_cmd_to_node('qps', [], group["id"], member, "Stats", 1) 
                qps_list = JSON.load(result)
                qps_list.is_a?(Array) or qps_list = [qps_list]
                qps_list.each {|qps|
                    total_qps += qps["info"].to_i
                }
            }
            total_qps
        end

    end
end
