# 下载镜像和软件
***
## 下载镜像

```
https://mirrors.tuna.tsinghua.edu.cn/ubuntu-releases/18.04/
```

## 下载VMware

### 下载链接

```
https://download3.vmware.com/software/wkst/file/VMware-workstation-full-16.0.0-16894299.exe
```
	idm fdm 直接粘贴都可以下载
	
### VMware激活码

随便一个都可以激活
```
ZF3R0-FHED2-M80TY-8QYGC-NPKYF
```
```
YF390-0HF8P-M81RQ-2DXQE-M2UT6  
```
```
ZF71R-DMX85-08DQY-8YMNC-PPHV8
```

# 创建虚拟机

![image.png](https://gitee.com/huweizdt/pic-go/raw/master/image/202304151124026.png)

![image.png](https://gitee.com/huweizdt/pic-go/raw/master/image/202304151125797.png)

![image.png](https://gitee.com/huweizdt/pic-go/raw/master/image/202304151128903.png)

如果蓝屏windows功能这里开启这两项

![image.png](https://gitee.com/huweizdt/pic-go/raw/master/image/202304151135787.png)

# 更换源等

## 换源

+ 复制粘贴
```
sudo gedit /etc/apt/sources.list
```
+ 删掉sources.list中的所有,粘贴下面的上去,并且保存
```
deb http://mirrors.aliyun.com/ubuntu/ bionic main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ bionic main restricted universe multiverse

deb http://mirrors.aliyun.com/ubuntu/ bionic-security main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ bionic-security main restricted universe multiverse

deb http://mirrors.aliyun.com/ubuntu/ bionic-updates main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ bionic-updates main restricted universe multiverse

# deb http://mirrors.aliyun.com/ubuntu/ bionic-proposed main restricted universe multiverse
# deb-src http://mirrors.aliyun.com/ubuntu/ bionic-proposed main restricted universe multiverse

deb http://mirrors.aliyun.com/ubuntu/ bionic-backports main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ bionic-backports main restricted universe multiverse

```
+ 更新源
```
sudo apt update
sudo apt upgrade
```



# 安装环境

## GCC G++

```
sudo apt install gcc g++
```
### 测试是否安装成功
```cpp
#include<iostream>
using namespace std;
int main(){
	cout<<"hello world"<<endl;
	return 0;
}
```
![image.png](https://gitee.com/huweizdt/pic-go/raw/master/image/202304151215027.png)

## 安装MathGL

```
sudo apt install build-essential
sudo apt install libmgl-dev
```

测试是否可用（可以忽略这一步）
创建一个test.cpp文件

```c++
#include <mgl2/mgl.h>

 int main() {
      mglGraph gr;
      gr.Title("MathGL Demo");
      gr.SetOrigin(0, 0);
      gr.SetRanges(0, 10, -2.5, 2.5);
      gr.FPlot("sin(1.7*2*pi*x) + sin(1.9*2*pi*x)", "r-2");
      gr.Axis();
     gr.Grid();
     gr.WriteFrame("mgl_example.png");
 }
```
编译
```
g++ test.cpp -o test -lmgl -lmgl-wnd
```

```
./test
```
文件夹中生成了png
![image.png](https://gitee.com/huweizdt/pic-go/raw/master/image/202304141853477.png)

## 安装GMP

+ 第一步
```
sudo apt-get install libgmp-dev
sudo apt-get install m4
```
+ 第二步
```
wget https://gmplib.org/download/gmp/gmp-6.2.1.tar.xz
```
+ 第三步
```
tar xvf gmp-6.2.1.tar
```
+ 第四步
```
# 进入gmp目录
cd gmp-6.2.1
# 设置安装路径前缀，建议设置为/usr
./configure --prefix=/usr  --enable-cxx
# 开始编译安装
make
make check
sudo make install
```
### 测试是否安装成功
```cpp
#include <gmpxx.h>
#include <iostream>
#include <stdio.h>
using namespace std;
int main()
{
        mpz_t a, b, c;
        mpz_init(a);
        mpz_init(b);
        mpz_init(c);
        printf("========= Input a and b => Output a + b =========\n");
        printf("[-] a = ");
        gmp_scanf("%Zd", a);
        printf("[-] b = ");
        gmp_scanf("%Zd", b);
        mpz_add(c, a, b);
        gmp_printf("[+] c = %Zd\n",c);
        return 0;
}

```
***
```
# 编译testGMP.cpp
g++ testGMP.cpp -o testGMP -lgmp -lm
# 没有报错的话就说明安装成功～运行即可
./testGMP

```
![image.png](https://gitee.com/huweizdt/pic-go/raw/master/image/202304151307378.png)

## 安装GLPK

+ 下载
```
sudo apt install libglpk-dev
```
## 下载TinyXML-2

### 先下载git

```
sudo apt install git

//配置git

git config --global user.name "huwei"   //用自己的名字或者随便你想用什么
git config --global user.email "huweizdt@163.com" //用自己的email或者你自己确定
```

### 下载TinyXML-2 

```
git clone https://github.com/leethomason/tinyxml2.git
解压出来
cd tinyxml2
make
./xmltest
```

全都通过

![](https://gitee.com/huweizdt/pic-go/raw/master/image/202304151939060.png)

## 安装mysql

```
sudo apt-get install mysql-server
apt-get isntall mysql-client
sudo apt-get install libmysqlclient-dev
```
设置免密登录
```
sudo gedit /etc/mysql/mysql.conf.d/mysqld.cnf
```
修改 [ mysqld ]
```
[mysqld]
#
# * Basic Settings
#
user            = mysql
pid-file        = /var/run/mysqld/mysqld.pid
socket          = /var/run/mysqld/mysqld.sock
port            = 3306
basedir         = /usr
datadir         = /var/lib/mysql
tmpdir          = /tmp
lc-messages-dir = /usr/share/mysql
skip-external-locking
skip-grant-tables //增加这个
```
重启mysql
```
service mysql restart
```
