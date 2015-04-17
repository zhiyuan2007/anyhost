module Beacon
module GRID
    class Member
        include RB::Serializable
        column :ip, :string
        column :member_name, :string
        column :services_str, :string

        def name=(new_name)
            @name = new_name
            db_update(:member_name)
        end

        def member_name
            @name
        end

        def services_str
            services.map{|ser| ser.name}.join(",")
        end

        def self.deserialize_obj(data)
            m = Member.new(data[:ip], data[:member_name])
            data[:services_str].split(",").each do |service|
                m.add_service(service)
            end
            m
        end
    end

    class Group
        include RB::Serializable
        has_many Member
        column :group_name, :string

        def group_name
            @name
        end

        def name=(new_name)
            @name = new_name
            db_update(:group_name)
        end

        def self.deserialize_obj(data)
            group = self.new(data[:group_name])
            data[:members].each do |member| 
                group._add_member(Member.deserialize(member))
            end
            group
        end
    end

    class MemberManager
        def self.deserialize
            return MemberManager.new if Group.db_all.empty? 
            Group.db_all.inject(MemberManager.new(false)) do |member_manager, group_data|
                member_manager._add_group(Group.deserialize(group_data))
                member_manager
            end
        end
    end
end
end
