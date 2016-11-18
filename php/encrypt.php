<?php
class NpEncrypt {
    public static function encrypt($input, $key) {
        $size = mcrypt_get_block_size(MCRYPT_RIJNDAEL_128, MCRYPT_MODE_ECB);
        $input = NpEncrypt::pkcs5_pad($input, $size);
        $td = mcrypt_module_open(MCRYPT_RIJNDAEL_128, '', MCRYPT_MODE_ECB, '');
        $iv = mcrypt_create_iv (mcrypt_enc_get_iv_size($td), MCRYPT_RAND);
        #$iv = "1234567890123456";
        mcrypt_generic_init($td, $key, $iv);
        $data = mcrypt_generic($td, $input);
        mcrypt_generic_deinit($td);
        mcrypt_module_close($td);
        $data = NpEncrypt::url_safe_base64_encode($data);
        return $data;
    }

    private static function pkcs5_pad ($text, $blocksize) {
        $pad = $blocksize - (strlen($text) % $blocksize);
        return $text . str_repeat(chr($pad), $pad);
    }

    public static function decrypt($sStr, $sKey) {
        $decrypted= mcrypt_decrypt(
            MCRYPT_RIJNDAEL_128,
            $sKey,
            NpEncrypt::url_safe_base64_decode($sStr),
            MCRYPT_MODE_ECB
        );

        $dec_s = strlen($decrypted);
        $padding = ord($decrypted[$dec_s - 1]);
        $decrypted = substr($decrypted, 0, -$padding);
        return $decrypted;
    }

    public static function url_safe_base64_encode($string) {
        #First base64 encode
        $data = base64_encode($string);

        #Base64 strings can end in several = chars. These need to be translated into a number
        $no_of_eq = substr_count($data, "=");
        $data = str_replace("=", "", $data);
        $data = $data.$no_of_eq;

        #Then replace all non-url safe characters
        $data = str_replace(array('+', '/'), array('-', '_'), $data);
        return $data;
    }

    public static function url_safe_base64_decode($string) {
        $no_of_eq = substr($string, -1);
        $string = substr($string, 0, -1);
        $string .= str_repeat('=', $no_of_eq);
        $string = str_replace(array('-','_'),array('+','/'),$string);
        $data = base64_decode($string);

        return $data;
    }

    public static function gen_key() {
        $hostname = php_uname('n');
        $ts = time();
        $rnd = mt_rand(1, PHP_INT_MAX);
        return substr(md5($hostname . $ts . $rnd), mt_rand(0, 15), 16);
    }
}


$salt = 'af44f40d1741325f';
#$key = NpEncrypt::gen_key();
#$key = "3aca73518f049c5f";
$key = "a2050b5430a2ad15";
echo "key:{$key}\n";
$data = "com.snda.wifilocating_3023_3089.patch2";
echo "data:$data\n";
$value = NpEncrypt::encrypt($data , $key . $salt);
#$value = "asFyKLbBkpO7ssgO3rvRLauKj7Oc2NAMNGFnq3zIY+H0WeBEWcqwlgJWDEYW+Trd0";
echo "ciphertext:{$value}\n";
#echo NpEncrypt::decrypt($value, $key . $salt);
