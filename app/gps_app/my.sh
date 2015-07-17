#!/bin/sh

echo -e "ALARM \a\n\t tab"
DATE=`date`
echo "DATE=$DATE"
TESTSTR=`ls`
echo "ls=$TESTSTR"
#VAL="tianjinshi 712"
NAME= TCB
echo "${VAL:-WEIXIN}"
#echo "${VAL:?val is error}"
echo "${VAL:=weixingdaohang}"
echo $VAL
#echo "Hello Please input your name:"

#read NAME
echo "hello $NAME!"
echo "please input a="
read a

echo "please input b="
read b

c=`expr $a + $b`
 
echo "a+b=$c"
if [ $a == $b ]
then
echo "val a == b"
fi

if [ $a != $b ]    
then               
echo "val a != b"
fi 

if [ "$a" -gt "$b" ]
then 
echo "val a > b"
fi

if [ "$a" -lt "$b" ] 
then 
echo "val a < b"
fi


if [ "$a" -ge "$b" ]                 
then                                 
echo "val a >= b"                     
fi

if [ "$a" -le "$b" ]                  
then                                 
echo "val a <= b"                     
fi 


if [ "$a" -le 10 -o "$b" -gt 100 ]                 
then                                 
echo "a<10 or b>100"                    
fi 

COMMAND="ls -al"

if [ "$a" -ge 10 ]                 
then
{
                                 
echo "val a >= b"
a=9
echo "Change a=$a"
}                    
fi                                   
  
echo "running..."
./my_app1 $a $COMMAND


