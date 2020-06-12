stty -F /dev/ttyACM0 1200
sleep 5
/home/baozhu/.arduino15/packages/Seeeduino/tools/bossac/1.8.0-48-gb176eee/bossac -i -d --port=ttyACM0 -U -i --offset=0x4000 -w -v edge-impulse.ino.Seeeduino.samd.seeed_wio_terminal.bin -R
