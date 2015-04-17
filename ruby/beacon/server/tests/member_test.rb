require "#{File.dirname(__FILE__)}/helper"
FileUtils.rm_rf("*.db")
RB::DB.init("members.db")

class TestMember < Test::Unit::TestCase
    include Beacon::GRID

    def setup
        Member.db_clear
        Group.db_clear
    end

    def test_create_member
        m = Member.new("1.1.1.1")
        assert_equal(m.name, "1.1.1.1")
    end

    def test_modify_service
        m = Member.new("1.1.1.1")
        assert(m.services.empty?)
        m.add_service("good")
        assert_equal(m.services.length, 1)

        assert_raise MemberException do
            m.add_service("good")
        end
        assert_equal(m.services.length, 1)
        
        assert_raise MemberException do
            m.del_service("ft")
        end
        m.del_service("good")
        assert(m.services.empty?)
    end

    def test_seriable
        m = Member.new("1.1.1.1")
        m.add_service("s1")
        m.add_service("s2")
        m.db_save

        m = Member.deserialize(Member.db_first)
        assert_equal(m.name, "1.1.1.1")
        assert_equal(m.services.length, 2)
        assert(m.services.include?("s1"))
        assert(m.services.include?("s2"))
    end
end

class GroupTest < Test::Unit::TestCase
    include Beacon::GRID

    def setup
        Group.db_clear
    end

    def test_create_group
        group = Group.new("n1")
        assert_equal(group.name, "n1")
        assert(group.members.empty?)
    end

    def test_add_member
        group = Group.new("n1")
        group.db_save
        group.add_member("1.1.1.1", "n2")
        assert_raise MemberException do
            group.add_member("2.2.2.2", "n2")
        end
        assert_equal(group.members.length, 1)

        assert_raise MemberException do
            group.del_member("2.2.2.2")
        end
        group.del_member("1.1.1.1") 
        assert_equal(group.members.length, 0)
        group.add_member("2.2.2.2", "n3")
        assert_equal(group.members.length, 1)
        group.del_member("n3") 
        assert_equal(group.members.length, 0)
    end

    def test_seriable
        group1 = Group.new("n1")
        group2 = Group.new("n2")
        group1.db_save
        group1.add_member("1.1.1.1", "m1")
        group1.add_member("2.2.2.2")
        group2.db_save
        group2.add_member("3.3.3.3")
        group2.add_member("2.2.2.2", "m2")
        group1.add_service_to_member("m1", "s1")
        group1.add_service_to_member("m1", "s2")
        group1.add_service_to_member("2.2.2.2", "s3")
        assert_raise MemberException do
            group2.move_member_to("m2", group1)
        end
        assert_equal(group1.members.length, 2)
        assert_equal(group2.members.length, 2)
        group2.move_member_to("3.3.3.3", group1)
        assert_equal(group1.members.length, 3)
        assert_equal(group2.members.length, 1)
       
        group = Group.deserialize(Group.db_first)
        assert_equal(group.members.length, 3)
        member = group.get_member("m1")
        assert_equal(member.services.length, 2)
        member = group.get_member("2.2.2.2")
        assert_equal(member.services.length, 1)
    end
end


class MemberManagerTest < Test::Unit::TestCase
    include Beacon::GRID

    def setup
        Group.db_clear
    end

    def test_create_group
        member_manager = MemberManager.new
        assert_equal(member_manager.groups.length, 1)
        member_manager.create_group("g1")
        assert_equal(member_manager.groups.length, 2)
        assert_raise MemberException do
            member_manager.create_group("default")
        end
        assert member_manager.has_group?("g1")
        member_manager.del_group("g1")
        assert_raise MemberException do
            member_manager.del_group("g1")
        end
        member_manager.del_group("default")
        assert_equal(member_manager.groups.length, 1)
    end

    def test_seriable
        member_manager = MemberManager.deserialize
        assert(member_manager.get_group_by_name(MemberManager::DEFAULT_GROUP))
        setup

        member_manager = MemberManager.new
        member_manager.create_group("g1")
        member_manager.create_group("g2")
        member_manager.create_group("g3")
        member_manager.del_group("default")
        member_manager.del_group("g3")

        group = member_manager.get_group_by_name("g2")
        group.add_member("2.2.2.2")
        group.add_service_to_member("2.2.2.2", "s1")
        member_manager.rename_group("g2", "g3")

        group1 = member_manager.get_group_by_name("g1")
        group1.add_member("3.3.3.3", "m1")
        member_manager.move_group_member("g1", "g3", "3.3.3.3")

        5.times do 
            member_manager = MemberManager.deserialize
            assert_equal(member_manager.groups.length, 3)
            group = member_manager.get_group_by_name("g3")
            assert_equal(group.members.length, 2)
            member = group.get_member("2.2.2.2")
            assert(member.services.include?("s1"))
        end
    end
end
