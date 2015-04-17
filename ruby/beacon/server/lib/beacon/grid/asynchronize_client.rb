require "bunny"
require "thread"
require 'json'
require 'securerandom'
require "forwardable"

module Beacon
    class RPCException < RuntimeError; end

    class AsynchronizeClient
        include RB::Log
        extend Forwardable
        set_log_category "beacon_rpc_asyn"
        def_delegator :@db, :insert_result   
        def initialize(connection, queue_name, db_path)
            @db = DB::MYDB.new(db_path)
            @queue = connection.queue(queue_name, true)
            @queue.subscribe do |delivery, properties, payload|
                @db.insert_result(JSON.load(payload).merge({"uuid"=>properties.correlation_id}))
                debug "get response: #{payload} in asyn queue and store it into db"
            end 
        end

        def send_without_waiting(exchange, routing_key, msg)
            begin
            corr_id = SecureRandom.uuid
            msg[:uuid] = corr_id
            exchange.publish(JSON.generate(msg),
                             :routing_key    => routing_key,
                             :correlation_id => corr_id,
                             :reply_to       => @queue.name)
            rescue
                return {:result => "failed", :info => "#{$!}"}
            end
            JSON.generate({:result => "succeed", :id => corr_id})
        end

        def get_result(id)
            @db.get_result(id)
        end
    end
end


if $0 == __FILE__
    client   = Beacon::RPCBase.new("127.0.0.1")
    100.times do |i|
        puts " [x] Requesting fib(30)"
        response = client.exec_cmd('p_rpc_queue', 6)
        puts " [#{i}] Got #{response}"
    end
end
