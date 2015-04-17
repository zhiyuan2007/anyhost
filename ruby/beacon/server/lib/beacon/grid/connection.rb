require "bunny"
require "forwardable"

module Beacon
    class Connection
        extend Forwardable
        def_delegators :@channel, :topic, :fanout, :direct, :default_exchange, :ack
        def_delegators :@connection, :queue_exists?, :exchange_exists?

        def initialize(host_name)
            @connection = Bunny.new({:host => host_name})
            @connection.start
            @channel = @connection.create_channel
        end

        def queue(name, exclusive = false)
            @channel.queue(name, {:exclusive => exclusive, :durable => false})
        end

        def stop
            @channel.close
            @connection.close
        end
    end
end

if $0 == __FILE__
    con = Beacon::Connection.new("127.0.0.1")
    puts con.direct("test_direct")
    puts con.fanout("test_fanout")
    puts con.queue_exists?("test_queue")
    puts con.exchange_exists?("SystemInfo")
    puts con.direct("test_direct").delete
    puts con.fanout("test_fanout").delete
end
