# 介绍
利用CE的DBK驱动加载未签名驱动


# 加载未签名驱动的原理
https://bbs.kanxue.com/thread-277919.htm


# 项目构成
包含两个项目:

1.CECheater项目生成的是lua53-64.dll，用于替换CheatEngine导入的同名dll

2.MyDriver项目生成的是MyDriver.sys，仅用于测试，可以替换成您想要加载的未签名驱动


# 编译
visual studio 2022 + x64 config（低版本的visual studio应该也是可以的）


# 部署
1.如果要直接使用部署结果，bin64.7z文件夹中提供了两种不同的方式加载未签名驱动MyDriver.sys，在“bin64\READMD.md”中有具体描述，您可以选择其中一种方式后替换掉MyDriver.sys即可

2.如果要自己编译源码，则编译完后将CECheater项目生成的lua53-64.dll替换掉bin64里原来的lua53-64.dll就可以了


# 运行
以管理员权限运行bin64.7z里的richstuff-x86_64.exe，进程会导入lua53-64.dll，这个dll会加载richstuffk64.sys，之后利用richstuffk64.sys提供的功能将MyDriver.sys映射到内存中，修复其RVA和导入表，并运行该驱动


# 支持平台
x64 Windows 7, 8.1 and 10
