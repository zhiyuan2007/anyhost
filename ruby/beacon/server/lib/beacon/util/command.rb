module Beacon
    class Command
        attr_accessor :name, :args
        def initialize(name, args)
            @name = name
            @args = args
            @args["syn"] = "yes" unless @args.has_key?("syn")
            @syn = args["syn"]
        end

        def syn
            @syn == "yes" 
        end
    
        def to_json
            {"command_name" => @name, "command_args" => @args}
        end
    end
end
