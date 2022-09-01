#!/usr/bin/env python3

import sys
import os
import subprocess

MODE_2_LIST = [
    {
        "name": "RSA2048_SHA256",
        "encrypt": False,
        "rsa": 2048,
        "sha": 256,
        "otp": "1030A0_RSA2048_SHA256.json"
    },
    {
        "name": "RSA2048_SHA256_o1",
        "encrypt": True,
        "option": 1,
        "rsa": 2048,
        "sha": 256,
        "otp": "1030A0_RSA2048_SHA256_o1.json"
    },
    {
        "name": "RSA2048_SHA256_o2_pub",
        "encrypt": True,
        "option": 2,
        "rsa_aes": "test_soc_private_key_2048.pem",
        "rsa": 2048,
        "sha": 256,
        "otp": "1030A0_RSA2048_SHA256_o2_pub.json"
    },
    {
        "name": "RSA2048_SHA256_o2_priv",
        "encrypt": True,
        "option": 2,
        "rsa_aes": "test_soc_public_key_2048.pem",
        "rsa": 2048,
        "sha": 256,
        "otp": "1030A0_RSA2048_SHA256_o2_priv.json"
    },
    {
        "name": "RSA3072_SHA384",
        "encrypt": False,
        "rsa": 3072,
        "sha": 384,
        "otp": "1030A0_RSA3072_SHA384.json"
    },
    {
        "name": "RSA3072_SHA384_o1",
        "encrypt": True,
        "option": 1,
        "rsa": 3072,
        "sha": 384,
        "otp": "1030A0_RSA3072_SHA384_o1.json"
    },
    {
        "name": "RSA3072_SHA384_o2_pub",
        "encrypt": True,
        "option": 2,
        "rsa_aes": "test_soc_private_key_3072.pem",
        "rsa": 3072,
        "sha": 384,
        "otp": "1030A0_RSA3072_SHA384_o2_pub.json"
    },
    {
        "name": "RSA3072_SHA384_o2_priv",
        "encrypt": True,
        "option": 2,
        "rsa_aes": "test_soc_public_key_3072.pem",
        "rsa": 3072,
        "sha": 384,
        "otp": "1030A0_RSA3072_SHA384_o2_priv.json"
    },
    {
        "name": "RSA4096_SHA512",
        "encrypt": False,
        "rsa": 4096,
        "sha": 512,
        "otp": "1030A0_RSA4096_SHA512.json"
    },
    {
        "name": "RSA4096_SHA512_o1",
        "encrypt": True,
        "option": 1,
        "rsa": 4096,
        "sha": 512,
        "otp": "1030A0_RSA4096_SHA512_o1.json"
    },
    {
        "name": "RSA4096_SHA512_o2_pub",
        "encrypt": True,
        "option": 2,
        "rsa_aes": "test_soc_private_key_4096.pem",
        "rsa": 4096,
        "sha": 512,
        "otp": "1030A0_RSA4096_SHA512_o2_pub.json"
    },
    {
        "name": "RSA4096_SHA512_o2_priv",
        "encrypt": True,
        "option": 2,
        "rsa_aes": "test_soc_public_key_4096.pem",
        "rsa": 4096,
        "sha": 512,
        "otp": "1030A0_RSA4096_SHA512_o2_priv.json"
    },
]

root_dir = ""


def run_shell(cmd):
    print(cmd)
    my_env = os.environ.copy()
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, cwd=root_dir, env=my_env)
    p.communicate()
    if p.returncode != 0:
        raise TypeError


if __name__ == "__main__":
    root_dir = sys.argv[1]
    image_path = root_dir+'/build/zephyr/zephyr.bin'
    uart_image_path = root_dir+'/build/zephyr/uart_zephyr.bin'
    key_folder = root_dir+'/boards/arm/ast1030_evb/key'
    aes_key_path = root_dir+'/boards/arm/ast1030_evb/key/test_aes_key.bin'
    run_shell('mkdir -p ./build/sb_bin')

    for mode2 in MODE_2_LIST:
        dest_folder = './build/sb_bin/{}'.format(mode2['name'])
        run_shell('mkdir -p {}'.format(dest_folder))
        alg = 'RSA{}_SHA{}'.format(mode2['rsa'], mode2['sha'])

        otp_config = '{}/boards/arm/ast1030_evb/otp_config/{}'.format(
            root_dir, mode2['otp'])
        run_shell('otptool make_otp_image\
        {}\
        --key_folder {}\
        --output_folder {}\
        '.format(otp_config, key_folder, dest_folder))


        for kid in [0, 1, 2]:
            sign_key_path = '{}/test_oem_dss_private_key_{}_{}.pem'.format(
                key_folder, mode2['rsa'], kid)
            output = '{}/sec_zephyr_{}.bin'.format(dest_folder, kid)
            if mode2['encrypt']:
                if mode2['option'] == 1:
                    run_shell('socsec make_secure_bl1_image\
			        --soc 1030\
                    --algorithm AES_{}\
                    --bl1_image {}\
                    --output {}\
                    --rsa_sign_key {}\
                    --aes_key {}\
                    --key_in_otp\
                    '.format(alg, image_path, output, sign_key_path, aes_key_path))
                else:
                    rsa_aes_path = '{}/{}'.format(key_folder, mode2['rsa_aes'])
                    run_shell('socsec make_secure_bl1_image\
			        --soc 1030\
                    --algorithm AES_{}\
                    --bl1_image {}\
                    --output {}\
                    --rsa_sign_key {}\
                    --aes_key {}\
                    --rsa_aes {}\
                    '.format(alg, image_path, output, sign_key_path, aes_key_path, rsa_aes_path))
            else:
                run_shell('socsec make_secure_bl1_image\
			    --soc 1030\
                --algorithm {}\
                --bl1_image {}\
                --output {}\
                --rsa_sign_key {}\
                '.format(alg, image_path, output, sign_key_path))

            otp_image = dest_folder+'/otp-all.image'
            run_shell('socsec verify\
                --soc 1030A0\
                --sec_image {}\
                --otp_image {}\
                '.format(output, otp_image))

            gen_uart_script = root_dir + \
                '/boards/arm/ast1030_evb/tools/gen_uart_booting_image.sh'
            uart_output = '{}/sec_uart_zephyr_{}.bin'.format(dest_folder, kid)
            run_shell('{} {} {}'.format(gen_uart_script, output, uart_output))

        run_shell('cp {} {}'.format(image_path, dest_folder))
        uart_output = '{}/uart_zephyr.bin'.format(dest_folder)
        run_shell('{} {} {}'.format(gen_uart_script, image_path, uart_output))

