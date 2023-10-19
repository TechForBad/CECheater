# 介绍
利用CE的DBK驱动加载未签名驱动

# 加载未签名驱动的原理
https://bbs.kanxue.com/thread-277919.htm

# 项目构成
包含两个项目:
* CECheater项目生成的是lua53-64.dll，用于替换CheatEngine导入的同名dll
* MyDriver项目生成的是MyDriver.sys，仅用于测试，可以替换成您想要加载的未签名驱动

# 编译
CECheater项目的编译配置为“C++17 + vs2022 + x64 config”，编译完后将生成的lua53-64.dll替换掉bin64里原来的lua53-64.dll就可以了

# 运行
文件夹bin64.7z里提供了最终部署结果，需要以管理员权限运行，提供了以下两种不同的方式加载未签名驱动MyDriver.sys
* 将MyDriver.sys映射到内存中，修复其RVA和导入表，之后由当前进程直接运行驱动的入口点代码
```
richstuff-x86_64.exe -load_by_shellcode .\\MyDriver.sys
```
* 将MyDriver.sys映射到内存中，修复其RVA和导入表，之后调用IoCreateDriver来加载驱动，会创建驱动对象“\\FileSystem\\<驱动文件名>”（此处为“\\FileSystem\\MyDriver”），并由系统进程运行驱动的入口点代码
```
richstuff-x86_64.exe -load_by_driver .\\MyDriver.sys
```

# 支持平台
x64 Windows 7, 8.1 and 10
