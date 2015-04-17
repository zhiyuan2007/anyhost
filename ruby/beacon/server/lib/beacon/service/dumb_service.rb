require 'beacon'

module Beacon
module Service
    class DumbService
        def initialize
            @interface = Beacon::Interface.new('Dumb')
            @interface.register_command("max", method(:max))
            @interface.register_command("min", method(:min))
            @interface.register_command("pj", method(:pass_json))
            @interface.register_command("hw", method(:helloword))
            @interface.register_command("echo", method(:echo))
            @interface.register_command("ok", method(:ok?))
        end

        def max(a, b)
            raise "Invalid paramerer a" if a.class == "String"
            raise "Invalid paramerer b" if b.class == "String"
            a > b ? a : b
        end

        def min(a, b=20)
            raise "Invalid paramerer a" if a.class == "String"
            raise "Invalid paramerer b" if b.class == "String"
            a > b ? b : a
        end

        def pass_json(key1, key2)
            key1.merge(key2)
        end

        def helloword
            "hello world"
        end

        def echo(para)
            para
        end

        def ok?(p=false)
            p
        end

        def run
            @interface.run_command
        end

        def self.run
            service = DumbService.new
            service.run
        end
    end
end
end
