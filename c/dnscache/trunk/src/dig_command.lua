local json = require('json')

local base = _G

module("dig_command")


local ArgSpec = {
    create = function (self, spec_table) 
        o = spec_table
        base.setmetatable(o, self)
        self.__index = self
        return o
    end,

    get_arg_name = function (self)
                    return self.item_name
                end,

    get_arg_type = function (self)
                    return self.item_type
                end,

    arg_is_optional = function (self)
                        return self.item_optional
                    end,

    get_arg_default_val = function (self)
                        base.assert(self:arg_is_optional(), "only optional arg has default value")
                        return self.item_default
                    end
}



local CommandSpec = {
    create = function (self, command_spec)
        base.setmetatable(command_spec, self)
        self.__index = self 
        return command_spec
    end,

    get_command_name = function (self)
        return self.command_name
    end,

    get_command_description = function (self)
        return self.command_description
    end,

    get_arg_specs = function (self)
        if not self.arg_specs then
            local arg_specs = {}
            for k,v in base.pairs(self.command_args) do
                arg_specs[v.item_name] = ArgSpec:create(v)
            end
            self.arg_specs = arg_specs 
        end
        return self.arg_specs
    end,

    get_arg_spec = function (self, arg_name) 
        specs = self:get_arg_specs()
        return specs[arg_name]
    end
}


local Command = {

    create = function(self, command_json_string, command_server)
        is_valid_json_format, command_json = base.pcall(json.decode, command_json_string)
        if not is_valid_json_format then
            return nil
        end

        command_name = command_json.command[1]
        command_args = command_json.command[2]
        command_spec = command_server:get_command_spec(command_name)
        if command_spec == nil then
            base.print("command spec isn't found")
            return nil
        end

        command = {}
        command.name = command_name
        for arg_name, arg_spec in base.pairs(command_spec:get_arg_specs()) do
            arg_value = command_args[arg_name]
            if arg_value == nil then
                if arg_spec:arg_is_optional() == false then 
                    base.print("arg isn't optional")
                    return nile
                else
                    arg_value = arg_spec:get_arg_default_val()
                end
            end

            arg_type = arg_spec:get_arg_type()
            if arg_type == "Integer" then
                arg_value = tonumber(arg_value)
                if arg_value == nil then 
                    base.print("arg value isn't integer")
                    return nil 
                end
            end
            command[arg_name] = arg_value
        end
        base.setmetatable(command, self)
        self.__index = self
        return command
    end,

    get_arg_value = function (self, arg_name)
        return self[arg_name]
    end,

    get_command_name = function (self)
        return self.name
    end,

    print_command = function (self)
        for k,v in base.pairs(self) do
            base.print(k .. "=>" .. v)
        end
    end
}

local CommandManager = {
    create = function (self, specs_json_string)
        local server = {command_specs = {}}

        is_valid_json_format, command_specs = base.pcall(json.decode, specs_json_string)
        if not is_valid_json_format then
            return nil
        end

        command_spec_list = command_specs.command_specs
        for k,v in base.pairs(command_spec_list) do
            server.command_specs[v.command_name] = CommandSpec:create(v)
        end

        base.setmetatable(server, self)
        self.__index = self
        return server
    end,

    get_command_spec = function (self, command_name)
        return self.command_specs[command_name]
    end,

    create_command = function(self, command_json_string)
        return Command:create(command_json_string, self)
    end

}


function command_manager_create(command_spec_json)
    return CommandManager:create(command_spec_json)
end



