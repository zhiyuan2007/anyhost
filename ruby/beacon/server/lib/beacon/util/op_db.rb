require 'sequel'

module Beacon
module DB
    class MYDB
        def initialize(db_path)
           @DB = Sequel.sqlite db_path
           _create_asyn_result_store_table
        end

        def _create_asyn_result_store_table
            return if @DB.table_exists?(:asyn_result)
            @DB.create_table :asyn_result do
                primary_key :id
                String :uuid
                String :name
                String :command_id
                String :parameter
                String :result
                String :info
                String :client
            end
        end

        def insert_result(args)
            items = @DB[:asyn_result]
            items.insert(:name => args["command_name"], :uuid=>args["uuid"], :command_id=> args["command_id"],\
                         :parameter=> args["command_args"].to_s, :result=> args["result"], :info=>args["info"].to_s, :client=>args["_ip"])
        end

        def get_result(uuid)
            items = @DB[:asyn_result]
            items.where(:uuid=>uuid).all
        end
    end
end
end

if $0 == __FILE__
    aa = Beacon::DB::MYDB.new("./dbtest.db")
    aa.create_asyn_result_store_table
    aa.insert_result("command_name"=>"max", "uuid"=>"123421", "command_id"=>100, "command_args"=>{'a'=>10, 'b'=>20}, "status"=>"failed", "result"=>"NON")
    puts aa.get_result("123421")
end
