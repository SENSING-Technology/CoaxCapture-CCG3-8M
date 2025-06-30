
import os,sys

checkname = sys.argv[1]
framediv = int(sys.argv[2])
diffcount = framediv*40000 + 35000

fp = open(checkname)
oldtime=0
while 1:    
    lines = fp.readline()
    if not lines:
        break;
    newtime = long(lines)
    if newtime - oldtime > diffcount:
        print("############# time: %ld, difftime: %ld"%(newtime, (newtime - oldtime)))
    else:
        print("************* time: %ld, difftime: %ld"%(newtime, (newtime - oldtime)))
    oldtime = newtime 
