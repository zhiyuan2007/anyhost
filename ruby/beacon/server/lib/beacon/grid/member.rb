module Beacon
module GRID
    class MemberException < RuntimeError; end

    class Service
        include Comparable
        attr_reader :name
        attr_accessor :state
        def <=> (another)
            self.name <=> another.name
        end
        def initialize(name)
            @name = name
            online
        end
        def online; @state = :online; end
        def offline; @state = :offline; end
    end

    class Member
        include Comparable
        include RB::Log
        set_log_category "beacon_member"

        attr_reader :ip, :state
        attr_reader :services
        attr_accessor :name 
        def <=>(another)
            self.name <=> another.name
        end

        def initialize(ip, name = nil)
            @ip = ip
            @name = name || ip
            @services = []
            online
        end

        def add_service(service)
            raise MemberException.new("service #{service} already added") if get_service_by_name(service)
            ser = Service.new(service)
            @services << ser
            db_update(:services_str)
            ser
        end

        def del_service(service)
            ser = get_service_by_name(service)
            raise MemberException.new("service #{service} doesn't exists") unless ser
            @services.delete(ser)
            db_update(:services_str)
            ser
        end

        def get_service_by_name(service_name)
            @services.find{|ser| ser.name == service_name}
        end

        def all_services
            @services.map { |ser| { "name"=> ser.name, "state" => ser.state} }
        end
        
        def online; @state = :online; end
        def offline; @state = :offline; end
    end
    
    class Group 
        include RB::Log
        set_log_category "beacon_member"

        include Comparable
        attr_accessor :members, :name
        def <=>(another)
            self.name <=> another.name
        end

        def initialize(group_name)
            raise MemberException.new("group name can't nil") unless group_name
            @name = group_name
            @members = []
        end

        def clear_member
            @members.each(&:db_destroy)
            @members.clear
        end

        def add_member(ip, name = nil)
            raise MemberException.new("node with #{ip} already added") if _get_member_by_ip(ip)
            raise MemberException.new("node with #{name} already added") if name && _get_member_by_name(name)
            _add_member(Member.new(ip, name))
        end

        def move_member_to(member_id, target_group)
            member = get_member(member_id)
            raise MemberException.new("no node with #{member_id} exists") unless member
            raise MemberException.new("node with ip #{member.ip} already exists in group #{target_group.name}") if target_group._get_member_by_ip(member.ip)
            raise MemberException.new("node with name #{member.name} already exists in group #{target_group.name}") if target_group._get_member_by_name(member.name)
            _del_member(member)
            target_group._add_member(member)
        end

        def _add_member(member)
            @members << member
            unless member.db_id
                member.db_save
                member.db_save_ownership(self)
            end
            member
        end

        def to_s
            @members.inject("") do |info, member|
                info += "name: #{member.name} ip: #{member.ip} "
                member.services.each {|ser| info += "service: #{ser.name}" }
                info += "\n"
            end
        end

        def add_service_to_member(id, service)
            member = get_member(id) || add_member(id, id)
            member.add_service(service)
        end

        def del_service_from_member(id, service)
            member = get_member(id)
            raise MemberException.new("member with #{id} does't exists")  unless member
            member.del_service(service)
        end

        def del_member(member_id)
            member = get_member(member_id)
            raise MemberException.new("member #{member_id} doesn't exists") unless member
            _del_member(member)
        end

        def _del_member(member)
            @members.delete(member)
            member.db_destroy
            member
        end

        def get_member(member_id)
            _get_member_by_ip(member_id) || _get_member_by_name(member_id)
        end

        def _get_member_by_ip(ip)
            @members.find {|member| member.ip == ip}
        end

        def _get_member_by_name(name)
            @members.find {|member| member.name == name}
        end

    end

    class MemberManager
        DEFAULT_GROUP = "default"
        attr_reader :groups

        include RB::Log
        set_log_category "beacon_member"

        def initialize(create_default_group = true)
            @groups = []
            create_group(DEFAULT_GROUP) if create_default_group
        end 

        def create_group(group_name)
            raise MemberException.new("group #{group_name} already added") if has_group?(group_name)
            group = Group.new(group_name)
            _add_group(group)
        end 

        def _add_group(group)
            @groups << group
            group.db_save
            group
        end 

        def rename_group(old_name, new_name)
            raise MemberException.new("group: #{old_name} can't be rename") if old_name == DEFAULT_GROUP
            target_group = get_group_by_name(old_name)
            raise MemberException.new("group: #{old_name} doesn't exists , can't be rename") unless target_group
            raise MemberException.new("group: #{new_name} alreay exists , can't be rename") if get_group_by_name(new_name)
            target_group.name = new_name
        end

        def del_group(group_name)
            raise MemberException.new("group #{group_name}  does't exists") unless has_group?(group_name)
            group = get_group_by_name(group_name)
            if group_name == DEFAULT_GROUP 
                group.clear_member 
            else
                @groups.delete(group)
                group.db_destroy
            end
        end 

        def move_group_member(old_group_name, new_group_name, member_id)
            old_group = get_group_by_name(old_group_name)
            raise MemberException.new("group #{old_group_name} not exists") unless old_group
            target_group = get_group_by_name(new_group_name)
            raise MemberException.new("group #{new_group_name} not exists") unless target_group
            debug "move member: #{member_id} to group: #{new_group_name} from group: #{old_group_name}"
            old_group.move_member_to(member_id, target_group)
            debug "move success, members in group: #{old_group_name} was #{old_group.to_s}\nin group: #{new_group_name} was #{target_group.to_s}"
        end

        def get_group_by_name(group_name)
            @groups.find{|g| g.name == group_name}
        end 

        def has_group?(group_name)
            not get_group_by_name(group_name).nil?
        end 

        def mb_and_ser_exists?(member_id, service)
            @groups.each{ |group|
                group.members.each{|member| 
                    member.services.each {|ser|
                        return true if ser.name == service && member.name == member_id
                    }
                }
            }
            false
        end

        def update_group_member(group, old_id, new_id)
            get_member_by_name(group, old_id).name = new_id
        end

        def delete_group_member(group, member_id)
            get_group_by_name(group).del_member(member_id)
        end

        def delete_group_member_service(group, member_id, service_name)
            get_group_by_name(group).del_service_from_member(member_id, service_name)
        end

        def get_member_ip_by_id(id)
            @groups.each{ |group| group.members.each{|member| return member.ip if member.name == id}}
            nil
        end

        def get_group_members(group_name)
            get_group_by_name(group_name).members
        end

        def get_group_by_member_ip(member_ip)
            group = @groups.find{ |group_| group_.members.find {|member| member.ip == member_ip}}
            group ? group : get_group_by_name(DEFAULT_GROUP)
        end

        def get_member_by_name(group_name, member_name)
            group = get_group_by_name(group_name)
            raise GridException.new("group #{group_name} does't exists") unless group
            group.get_member(member_name)
        end

    end
end
end
