require "bunny"
require "thread"
require 'json'
require 'securerandom'

module Beacon
    class RPCException < RuntimeError; end

    class SynchronizeClient
        attr_reader :m, :c
        attr_accessor:corr_id
        attr_accessor :response, :answers
        include RB::Log
        set_log_category "beacon_syn"

        DEFAULT_TIMEOUT = 600
        def initialize(connection, queue_name)
            @m = Mutex.new
            @c = ConditionVariable.new
            @timeout = DEFAULT_TIMEOUT
            @answers = 1
            @response = []
            that = self
            @queue = connection.queue(queue_name, true)
            @queue.subscribe do |delivery_info, properties, payload|
                if properties.correlation_id == that.corr_id
                    that.response << JSON.load(payload) 
                end
                if that.response.length == that.answers
                    that.m.synchronize{that.c.signal}
                end
            end
        end

        def send_and_wait(exchange, queue_name, msg)
            @corr_id == SecureRandom.uuid
            @response.clear
            exchange.publish(JSON.generate(msg),
                             :routing_key    => queue_name,
                             :correlation_id => @corr_id,
                             :reply_to       => @queue.name)

            @m.synchronize{@c.wait(@m, @timeout)}
            raise RPCException.new("command #{msg} timeout, routing_key: #{queue_name}") if @response.empty?
            rtn = exchange.type.to_s == "direct" ? @response[0] : @response
            JSON.generate(rtn)
        end

        def delete_queue
            @queue.delete
        end

        def timeout= (val)
            @timeout = val.to_i
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
