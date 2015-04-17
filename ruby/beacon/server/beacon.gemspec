Gem::Specification.new do |s| 
    s.name = "beacon"
    s.version = "0.0.1"
    s.date = "2013-09-17"
    s.authors = ["Knet DNS"]
    s.email = "td_ddi@knet.cn"
    s.summary = "communication library"
    s.description = "based on rabbitmq, provide basic communication between hosts"
    s.files = Dir["lib/*.rb"]
    s.files += Dir["lib/beacon/*.rb"]
    s.files += Dir["lib/beacon/util/*.rb"]
    s.files += Dir["lib/beacon/grid/*.rb"]
    s.files += Dir["lib/beacon/bin/*"]
    s.files += Dir["lib/beacon/service/*.rb"]
    s.files += Dir["beacon.gemspec"]
    s.bindir = "bin"
    s.executables << "beacon" 
    s.executables << "beacon_sender" 
    s.executables << "beacon_test" 
    s.executables << "beacon_gmgr" 
    s.require_paths << 'lib'
    s.add_dependency('roadbike', '>= 0.0.1')
    s.add_dependency('bunny', '>= 0.10.2')
end

