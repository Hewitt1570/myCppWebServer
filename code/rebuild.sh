##########################################################################
# File Name: rebuild.sh
# Author: amoscykl
# mail: amoscykl980629@163.com
# Created Time: 2020年04月01日 星期三 18时33分05秒
#########################################################################
#!/bin/zsh
PATH=/home/edison/bin:/home/edison/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/work/tools/gcc-3.4.5-glibc-2.3.6/bin
export PATH 
make cleanall
make
