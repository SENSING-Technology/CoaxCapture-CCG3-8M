一.  driver insmod
1. 把 ./ko/#uname/  目录中的xdma.ko拷贝至  /lib/module/#uname 目录下
   注: uname 可以通过在linux的terminal中执行 uname -r 获取
2. 在 /lib/module/#uname 目录下执行命令 depmod
3. 把 ./ko/rules.d 下面所有的文件拷贝到 /etc/udev/rules.d/ 目录下
4. 继续执行命令  modprobe xdma

二. test cases
1. v1.0 测试用例在test目录中，参见test目录中的readme
2. v2.0 测试用例在test2目录中，参见test2目录中的readme

三. samples
1: compile the samples
cd ./sample  && mkdir build
cd ./build
cmake .. && make

2: execute the samples
2.1 print -b {0|1} -w {1920} -h {1080}
2.2 print2 -w {1920} -h {1080}  -W {1280} -H {960}
2.3 view -b {0|1} -w {1920} -h {1080}  
2.4 view2 -w {1920} -h {1080}  -W {1280} -H {960}

-c:	 channels mask on board 0(0x3F default)
       0x1~0x3F   bitset 1 indicates a camera link
-C:	 channels mask on board 1(0x3F default)
       0x1~0x3F   bitset 1 indicates a camera link
-g:	 channels mask on board 0(0x3F default)
       0x1~0x3F   bitset 1 indicates a camera link
-G:	 channels mask on board 1(0x3F default)
       0x1~0x3F   bitset 1 indicates a camera link

-M:	   trigger mode   cmdnum
       freerun              0
       seconds-justified    1
-w     board 0 width(1920 default)
-h     board 0 height(1080 default)
-W     board 1 width(1280 default)
-H     board 1 height(960 default)
-X     view display region X value(1 default)
-Y     view display region Y value(1 default)
-s:    save image type(8 12 16 100 raw)
-T:    print time stamp
-P:    input frame pixel format (0:UYVY 1:Gray16le ) 
-o:    display
-l:    count

./view4 -c 1 -P 0 -s 1

raw12 priview:
./view -c 2 -P 1 -o 1

