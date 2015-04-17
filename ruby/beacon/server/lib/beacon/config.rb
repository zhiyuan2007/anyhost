require 'fileutils'

module Beacon
module Config
    def self.init(config_file)
        instance_eval(File.read(config_file))
        _build_folder_environment
    end

    def self._build_folder_environment
        _make_sure_dir(self.get("home_dir"))
        _make_sure_dir(self.get("log_dir"))
        _make_sure_dir(self.get("db_dir"))
    end

    def self._make_sure_dir(dir)
        FileUtils.mkdir_p(dir) unless File.exist?(dir)
    end

    CONFIG_ITERM = ["home_dir", 
                    "log_dir", 
                    "db_dir", 
                    "rabbitmq_server", 
                    "nic_name", 
                    "user",
                    "password",
                    "log_to_stdout"]

    CONFIG_ITERM.each do |config_iterm|
        self.instance_eval <<-CONFIG_METHOD
            def #{config_iterm}(val); @#{config_iterm} = val; end
        CONFIG_METHOD
    end

    def self.get(config_iterm)
        self.instance_variable_get(("@"+config_iterm).to_sym)
    end

    def self.[](config_iterm)
        self.get(config_iterm)
    end
    
end
end

