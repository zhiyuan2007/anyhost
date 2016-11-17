mc_ecb    = require "resty.ecb_mcrypt"
my_ecb   = mc_ecb:new()

local function url_safe_base64_decode(str) 
    ngx.log(ngx.INFO, "safe origin:" .. str)
    local num_of_eq = tonumber(string.sub(str, -1))
    if not num_of_eq then
         ngx.log(ngx.ERR, "cipertext base64_encoded format error")
         return nil
    end
    str = string.sub(str, 0, -2)
    str = str .. string.rep("=",num_of_eq)
    local ori_str = string.gsub(str, "-", "+")
    ori_str = string.gsub(ori_str, "_", "/")
    ngx.log(ngx.INFO, "unsafe origin:" .. ori_str)
    return ngx.decode_base64(ori_str)
end

function rsub_string(str, dim)
    local ts = string.reverse(str)
    local _, i = string.find(ts, dim)
    local m = string.len(ts) - i + 1
    return string.sub(str, m+1), string.sub(str, 1, m)
end

function mytrim(s)
    return (s:gsub("^%s*(.-)%s*$", "%1"))
end

if ngx.var.arg_sign then
    --local key = "3aca73518f049c5f"
    local key = ngx.var.arg_sign
    local salt = "af44f40d1741325f"
    local secret_key = key .. salt
    local ciphertext, other_uri = rsub_string(ngx.var.uri, "/")
    if ciphertext == "/" then
       ngx.log(ngx.ERR, "uri error: " .. ngx.var.uri)
       ngx.exit(200)
    end
    ngx.log(ngx.INFO, "ciphertext: " .. ciphertext)
    local dec_base64str = url_safe_base64_decode(ciphertext)
    if not dec_base64str then
       ngx.log(ngx.ERR, "base64 decode failed") 
       ngx.exit(500)
    end
    local filename = my_ecb:decrypt(secret_key, dec_base64str)
    filename = mytrim(filename)
    local real_uri = other_uri ..filename 
    local real_args = nil
    local is_args = true
    local b, e = string.find(ngx.var.query_string, "sign="..ngx.var.arg_sign .. "&")
    if b and e then
        real_args = string.gsub(ngx.var.query_string, "sign="..ngx.var.arg_sign .."&", "") 
    else
        local b, e = string.find(ngx.var.query_string, "&sign="..ngx.var.arg_sign)
        if b and e then
           real_args = string.gsub(ngx.var.query_string, "&sign="..ngx.var.arg_sign, "") 
        else
           real_args = string.gsub(ngx.var.query_string, "sign="..ngx.var.arg_sign, "") 
           is_args = false
        end
    end
    ngx.log(ngx.INFO, "real uri " .. real_uri.. " real args " .. real_args)
    local m = ngx.re.match(real_uri, "^/[mn]qapk/([^/]*)/(.*\\.apk)", "o") 
    if not m then
        ngx.log(ngx.INFO, "this uri is not end with apk" .. ngx.var.uri);
        ngx.exit(508)
    end
    ngx.var.mid = m[1] 
    ngx.var.filename = m[2]
    if is_args then
        ngx.req.set_uri_args(real_args) 
    end 
    ngx.req.set_uri(real_uri, false)
else
    ngx.log(ngx.ERR, "not found sign, normal request uri:" .. ngx.var.uri)
    ngx.exit(509)
end
