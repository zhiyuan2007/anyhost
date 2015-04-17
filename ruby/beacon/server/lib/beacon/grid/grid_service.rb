require 'beacon'
require 'sinatra/base'
require 'simpleidn'
require 'json'

load 'json/ext.rb'

module Beacon
    class GRIDServiceError < RuntimeError; end

    class GRIDService < Sinatra::Base
        include Beacon::WebServiceHelper
        include RB::Log
        set_log_category "beacon_grid"

        before do
            content_type :json
            parse_parameter
        end

        after do
            headers "Access-Control-Allow-Origin" => "*", "Access-Control-Max-Age" => "1728000", 
                                     "Access-Control-Allow-Methods" => "GET, POST, PUT"
            info "#{request.url} #{request.request_method} #{request.ip} #{response.status} #{self.params}"
        end

        class << self
            attr_accessor :grid_manager
        end

        not_found do
            {:error => "does't match any url"}.to_json
        end 

        post "/service" do
            return {"result" => "service already inited"}.to_json if grid_manager
            init_db(File.join(Config.get("db_dir"), "grid.db"))
            set_grid_manager(Beacon::GRID::GridManager.new(Config.get("rabbitmq_server"), 
                                                          File.join(Config.get("db_dir"), "asyn_result.db")))
            {"result" => "succeed"}.to_json
            Signal.trap("TERM") do
                 debug "get signal Terminating..."
                 grid_manager.rpc_server.stop
                 grid_manager.rpc_client.stop
                 set_grid_manager(nil)
                 exit(1)
            end
        end

        delete "/service" do
            with_service_running do
                grid_manager.rpc_server.stop
                grid_manager.rpc_client.stop
                set_grid_manager(nil)
                {"result" => "succeed"}.to_json
            end
        end

        get "/results/:id" do
			with_service_running do
                grid_manager.get_asyn_result(params["id"]).to_json
			end
        end
        get "/exchanges" do
            with_service_running do
                grid_manager.all_exchanges.to_json
            end
        end

        #################  MemberManagement  #################
        get "/groups" do
            with_service_running do
                params["need_info"] = false if !params.has_key?("need_info") || params["need_info"] == "false"
                grid_manager.groups.sort.map{|group| _group_info(group,  need_info = params["need_info"])}.to_json
            end
        end

        get "/groups/:name" do
            with_service_running do
                params["need_info"] = false if !params.has_key?("need_info") || params["need_info"] == "false"
                _group_info(grid_manager.get_group_by_name(params["name"]),  need_info = params["need_info"]).to_json
            end
        end

        post "/groups" do
            with_service_running do
                grid_manager.create_group(params["name"])
                _group_info(grid_manager.get_group_by_name(params["name"]), false).to_json
            end
        end

        put "/groups/:old_name" do 
            with_service_running do
                grid_manager.move_to_group(params["old_name"], params["name"], get_useful_params(params))
                _group_info(grid_manager.get_group_by_name(params["name"]), false).to_json
            end
        end

        post "/groups/:name/members/:member/services/:service/commands/:command" do
            with_service_running do
               grid_manager.group_send(params["name"], params["member"], params["service"], params["command"], get_useful_params(params))
            end
        end

        delete "/groups/:name" do
            with_service_running do
                grid_manager.delete_group(params["name"])
            end
        end

        get "/groups/:name/members/:member_id" do
            with_service_running do
                params["need_info"] = false if !params.has_key?("need_info") || params["need_info"] == "false"
                _member_info(params["name"], params["member_id"], params["need_info"]).to_json
            end
        end

        post "/groups/:name/members" do
            with_service_running do
                grid_manager.insert_group_member(params["name"], params["member_id"], params["service_list"])
                _member_info(params["name"], params["member_id"], false).to_json
            end
        end

        put "/groups/:name/members/:member_id" do
            with_service_running do
                grid_manager.update_group_member(params["name"], params["member_id"], params["new_group"], params["new_id"])
                #_member_info(params["name"], params["member_id"], false).to_json
            end
        end

        delete "/groups/:name/members/:member_id" do
            with_service_running do
                grid_manager.delete_group_member(params["name"], params["member_id"])
            end
        end

        delete "/groups/:name/members/:member_id/services/:service" do
            with_service_running do
                grid_manager.delete_group_member_service(params["name"], params["member_id"], params["service"])
            end
        end


        get "/operation/count/members/:member_id/services/:service" do
            with_service_running do
                grid_manager.get_queue_msg_count(params["member_id"], params["service"])
            end
        end

        post "/operation/purge/members/:member_id/services/:service" do
            with_service_running do
                grid_manager.purge_queue(params["member_id"], params["service"])
            end
        end

        private
        def grid_manager
            self.class.grid_manager
        end

        def set_grid_manager(manager)
            self.class.grid_manager = manager
        end

        def with_service_running(&block)
            safe_run do
                raise GRIDServiceError.new("service isn't init") unless self.class.grid_manager
                yield
            end
        end

        def get_useful_params(params)
            ["splat", "captures"].each { |param| params.delete(param) }
            params
        end

        def parse_parameter
            return unless (request.env["CONTENT_TYPE"] || "").include?(JSON_CONTENT_TYPE)
            request.body.rewind          
            query_data = ""            
            loop do                  
                data = request.body.read                
                break if data.empty?                
                query_data += data            
            end             
            return if query_data.empty? 
            json_params = JSON.parse(query_data) rescue (raise REQUEST_BODY_JSON_FORMAT_ERROR)
            self.params = json_params.merge(params) 
        end 

        def _member_info(group_name, member_name, need_info = false)
            member = grid_manager.get_member_by_name(group_name, member_name)
            return {"error" => "member: #{member_name} not exists"} unless member
            base_info = {
                "id" => member.name,
                "name" => member.name,
                "ip" => member.ip,
                "group" => group_name,
                "state" => member.state.to_s,
                "services" => member.all_services
            }
            return base_info unless need_info
            begin
                result = grid_manager.get_member_systeminfo(group_name, member.ip)
                base_info.merge!(JSON.load(result)["info"])
            rescue
                info "#{$!} #{$@[0..10]}"
                base_info["state"] = "error"
            end
            base_info["state"] = "warning" if (base_info["cpu_usage"].to_f > 0.6 || base_info["memory_usage"].to_f > 0.6 ||
                                               base_info["disk_usage"].to_f > 0.6 || base_info["cpu_temp"].to_i > 80)
            base_info
        end

        def _group_info(group, need_info = false)
            return {"error" => "group: #{group} not exists"} unless group
            members = grid_manager.get_group_members(group.name)
            members_info = members.sort.map{|member| _member_info(group.name, member.name, need_info)}
            {    
                "id" => group.name,
                "name" => group.name,
                "members" => members_info
            }    
        end  

    end
end
