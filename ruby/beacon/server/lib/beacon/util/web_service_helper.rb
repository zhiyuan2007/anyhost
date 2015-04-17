module Beacon
    module WebServiceHelper
        JSON_CONTENT_TYPE = "application/json"

        class HTTPException < RuntimeError; end

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
            json_params = JSON.parse(query_data) rescue (raise HTTPException.new("http header json format error"))
            self.params = json_params.merge(params) 
        end 

        def post_file_content            
            content = nil             
            params.each do |k, v|                
                if v === Hash && v[:name] == k                    
                    content = v[:tempfile].read
                    break
                end
            end
            content
        end

        def safe_run(&block)
            begin
                ret = block.call or "\"ok\"" 
                (ret.kind_of? String) ? ret : ret.to_json
            rescue => e
                puts "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
                puts "#{$@[0..20]}"
                puts "error info: #{e.to_s}"
                puts "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
                halt 421, {"error" => e.to_s}.to_json
            end
        end

        def init_db(db_path)
            db_dir = File.dirname(db_path)
            FileUtils.mkdir_p(db_dir) unless File.exist?(db_dir)
            RB::Serializable.init_storage_path(db_path)
        end
    end
end
