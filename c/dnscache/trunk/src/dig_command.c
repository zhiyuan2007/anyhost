#include <assert.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <dig_command.h>
#include <zip_utils.h>
#include <dig_mem_pool.h>

#define LUA_GET_TABLE_FIELD(L, field, ret) do{\
    lua_pushstring((L), (field)); \
    lua_gettable((L), -2); \
    if (lua_isnil(L, -1)) {lua_pop(L, 1); (ret) = 1;} \
    else (ret) = 0; \
}while(0)

#define LUA_CALL_FUNC(L, func_name, has_arg, arg, ret) do {\
    LUA_GET_TABLE_FIELD(L, func_name, ret); \
    if (ret == 0) { \
        if (lua_isfunction(L, -1) == 0) {\
            lua_pop(L, 1);\
            ret = 1; \
        } else { \
            lua_pushvalue(L, -2); \
            int _param_number = 1; \
            if ((has_arg)) {lua_pushstring(L, (arg)); _param_number = 2;}\
            if (lua_pcall(L, _param_number, 1, 0)) \
                ret = 1;\
            else \
                ret = 0;\
        }\
    }\
}while(0)


struct command_manager
{
    lua_State *L_;
    mem_pool_t *mem_pool_;
};

struct command
{
    struct command_manager *manager_;
    char command_name_[MAX_COMMAND_NAME_LEN];
};


static void stackDump (lua_State *L)
{
    int i = 1;
    int top = lua_gettop(L);
    for (; i <= top; ++i)
    {
        int t = lua_type(L, i);
        printf("<%d=>", i);
        switch (t)
        {
            case LUA_TSTRING:
                printf("’%s’", lua_tostring(L, i));
                break;
            case LUA_TBOOLEAN:
                printf(lua_toboolean(L, i) ? "true" : "false");
                break;
            case LUA_TNUMBER:
                printf("%g", lua_tonumber(L, i));
                break;
            default:
                printf("%s", lua_typename(L, t));
                break;
        }
        printf(">");
    }
    printf("\n");
}



command_manager_t *
command_manager_create(const char *command_spec_file)
{
    lua_State *L = luaL_newstate();                        
    luaL_openlibs(L);        
    char *file_buffer = NULL;

    if (luaL_dofile(L, "dig_command.lua"))    
        goto ERROR;

    lua_getglobal(L, "dig_command");
    if (!lua_istable(L, -1))
        goto ERROR;

    int ret = 0;
    LUA_GET_TABLE_FIELD(L, "command_manager_create", ret);
    if (ret == 1)
        goto ERROR;

    FILE *file = fopen(command_spec_file, "r");
    if (file == NULL)
        goto ERROR;
    fseek (file, 0 , SEEK_END);
    size_t file_size = ftell(file);
    rewind (file);
    file_buffer = (char*) malloc (sizeof(char) * file_size);
    if (file_buffer == NULL) 
        goto ERROR;

    fread (file_buffer, 1, file_size, file);
    fclose(file);


    lua_pushlstring(L, file_buffer, file_size);
    if (lua_pcall(L, 1, 1, 0))                 
        goto ERROR;
    free(file_buffer);

    command_manager_t *manager = malloc(sizeof(command_manager_t));
    manager->L_ = L;
    manager->mem_pool_ = mem_pool_create(sizeof(command_t), 0, true);
    return manager;

ERROR:
    if (file_buffer)
        free(file_buffer);
    lua_close(L);
    return NULL;
}

void 
command_manager_delete(command_manager_t *manager)
{
    lua_close(manager->L_);
    mem_pool_delete(manager->mem_pool_);
    free(manager);
}

command_t *
command_manager_create_command(command_manager_t *manager, 
                               const char *command_json_string)
{
    lua_State *L = manager->L_;

    int ret = 0;
    LUA_CALL_FUNC(L, "create_command", true, command_json_string, ret);
    if (ret != 0)
       return NULL;
    
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return NULL;
    } 

    LUA_CALL_FUNC(L, "get_command_name", false, "", ret);
    if (ret != 0)
    {
        lua_pop(L, 1);
        return NULL;
    }

    if (!lua_isstring(L, -1))
    {
        lua_pop(L, 2);
        return NULL;
    }

    command_t *command = mem_pool_alloc(manager->mem_pool_);
    command->manager_ = manager;
    strncpy(command->command_name_, 
            lua_tostring(L, -1), 
            MAX_COMMAND_NAME_LEN);
    lua_pop(L, 1);
    return command;
}

void
command_manager_delete_command(command_manager_t *manager, command_t *command)
{
    lua_State *L = manager->L_;
    lua_pop(L, 1);
    mem_pool_free(manager->mem_pool_, command);
}

const char *
command_get_name(const command_t *command)
{
    return command->command_name_;
}

int 
command_get_arg(const command_t *command, const char *arg_name)
{
    lua_State *L = command->manager_->L_;
    int ret = 0;
    LUA_CALL_FUNC(L, "get_arg_value", true, arg_name, ret);
    return ret;
}


int 
command_get_arg_string(const command_t *command, 
                       const char *arg_name, 
                       char *value)
{
    lua_State *L = command->manager_->L_;
    if (command_get_arg(command, arg_name))
        return 1;

    if (!lua_isstring(L, -1))
        goto ERROR; 

    if (strlen(lua_tostring(L, -1)) > MAX_COMMAND_VALUE_LEN)
        goto ERROR;

    strcpy(value, lua_tostring(L, -1));
    lua_pop(L, 1);
    return 0;

ERROR:
    lua_pop(L, 1);
    return 1;
}

int command_get_arg_integer(const command_t *command, 
                            const char *arg_name, 
                            int *value)
{
    lua_State *L = command->manager_->L_;
    if (command_get_arg(command, arg_name))
        return 1;

    if (!lua_isnumber(L, -1))
    {
        lua_pop(L, 1);
        return 1;
    }
    
    *value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return 0;
}


