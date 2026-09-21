@REM Use the packed image: it carries the APP integrity/version header.
@dfu-util -a 0 -E 1 -s 0x80020000:leave -D build/output/akaLinkPro_App_pack.bin
