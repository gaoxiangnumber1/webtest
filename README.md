# webtest

##简介
 - webtest是一个基于webbench改进的网站测压工具。相比webbench：注释全面，容易理解；逻辑清晰，每一步都有博文讲解。
 - 支持平台：Linux。
 - 开发语言：C语言。

##使用
 - 安装：`make && sudo make install`
 - 卸载：`sudo make uninstall`

##功能
 - 支持用户自由设定客户端（client）个数、测试时间。默认为1个client，测试30秒。
 - 支持代理服务器（proxy server）；默认只支持http 1.1。
 - 支持http 4种方法：GET、HEAD、OPTIONS、TRACE。

##使用示例
```
$ make && sudo make install #安装
gcc -std=c99 -g -Wall -o webtest.o -c webtest.c
gcc -std=c99 -g -Wall -o webtest webtest.o
[sudo] password for xiang: 
Installed in /usr/local/sbin
Thanks for your use webtest-Xiang Gao.
$ webtest -h #使用教程
Visit Codes on Github: https://github.com/gaoxiangnumber1/webtest
webtest [option] URL
  -c|--clients <n>         Run <n> HTTP clients at once. Default 1.
  -f|--force               Don't wait for reply from server.
  -h|--help                This information.
  -p|--proxy <server:port> Use proxy server for request.
  -r|--reload              Send reload request - Pragma: no-cache.
  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.
  --get                    Use GET request method.
  --head                   Use HEAD request method.
  --options                Use OPTIONS request method.
  --trace                  Use TRACE request method.
$ webtest http://blog.csdn.net/gaoxiangnumber1 #默认配置测试：1个client、测试30秒
Visit Codes on Github: https://github.com/gaoxiangnumber1/webtest
Testing: GET http://blog.csdn.net/gaoxiangnumber1 using HTTP/1.1
With 1 client(s), running 30 second(s).
Speed = 1 pages/sec, 13847 bytes/sec.
Requests: 9 success, 0 failed.
$ webtest -c 100 -t 20 http://blog.csdn.net/gaoxiangnumber1 # 100个client、测试20秒
Visit Codes on Github: https://github.com/gaoxiangnumber1/webtest
Testing: GET http://blog.csdn.net/gaoxiangnumber1 using HTTP/1.1
With 100 client(s), running 20 second(s).
Speed = 11 pages/sec, 353735 bytes/sec.
Requests: 222 success, 0 failed.
$ webtest -c 100 -f http://blog.csdn.net/gaoxiangnumber1 # 100个client、测试30秒、不读取server的reply
Visit Codes on Github: https://github.com/gaoxiangnumber1/webtest
Testing: GET http://blog.csdn.net/gaoxiangnumber1 using HTTP/1.1
With 100 client(s), running 30 second(s), early socket close.
Speed = 15 pages/sec, 0 bytes/sec.
Requests: 471 success, 0 failed.
$ webtest --head -c 100 http://blog.csdn.net/gaoxiangnumber1 #HEAD方法，默认为GET
Visit Codes on Github: https://github.com/gaoxiangnumber1/webtest
Testing: HEAD http://blog.csdn.net/gaoxiangnumber1 using HTTP/1.1
With 100 client(s), running 30 second(s).
Speed = 14 pages/sec, 3965 bytes/sec.
Requests: 435 success, 4 failed.
$ webtest --options -c 100 http://blog.csdn.net/gaoxiangnumber1 #OPTIOPNS方法
Visit Codes on Github: https://github.com/gaoxiangnumber1/webtest
Testing: OPTIONS http://blog.csdn.net/gaoxiangnumber1 using HTTP/1.1
With 100 client(s), running 30 second(s).
Speed = 0 pages/sec, 36073 bytes/sec.
Requests: 20 success, 3 failed.
$ webtest --trace -c 100 http://blog.csdn.net/gaoxiangnumber1 #TRACE方法
Visit Codes on Github: https://github.com/gaoxiangnumber1/webtest
Testing: TRACE http://blog.csdn.net/gaoxiangnumber1 using HTTP/1.1
With 100 client(s), running 30 second(s).
Speed = 6 pages/sec, 1969 bytes/sec.
Requests: 178 success, 6 failed.
$ make uninstall #卸载
Uninstall webtest. Thanks for your use webtest-Xiang Gao.
```

##博文讲解
 1. [解析命令行参数-ParseArguments](http://blog.csdn.net/gaoxiangnumber1/article/details/52769035)
 2. [生成Http请求报文-BuildRequest](http://blog.csdn.net/gaoxiangnumber1/article/details/52775144)
 3. [输出测试参数-PrintConfigure](http://blog.csdn.net/gaoxiangnumber1/article/details/52775165)
 4. [测试服务器状态-TestServerState](http://blog.csdn.net/gaoxiangnumber1/article/details/52775179)
 5. [实际测试-TestMain](http://blog.csdn.net/gaoxiangnumber1/article/details/52775190)

##关于
 - Blog：http://blog.csdn.net/gaoxiangnumber1
 - E-mail：gaoxiangnumber1@126.com 欢迎您对我的项目/博客提出任何建议和批评。
 - 如果您觉得本项目不错或者对您有帮助，请赏颗星吧！谢谢您的阅读！
