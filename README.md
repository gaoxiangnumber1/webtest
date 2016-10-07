**发表完博客后，更新博文链接**
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

##博文讲解
 0. [Github项目webtest简介](http://blog.csdn.net/gaoxiangnumber1)
 1. [解析命令行参数-ParseArguments](http://blog.csdn.net/gaoxiangnumber1)
 2. [生成Http请求报文-BuildRequest](http://blog.csdn.net/gaoxiangnumber1)
 3. [输出测试参数-PrintConfigure](http://blog.csdn.net/gaoxiangnumber1)
 4. [测试服务器状态-TestServerState](http://blog.csdn.net/gaoxiangnumber1)
 5. [实际测试-TestMain](http://blog.csdn.net/gaoxiangnumber1)

##关于
 - Blog：http://blog.csdn.net/gaoxiangnumber1
 - E-mail：gaoxiangnumber1@126.com 欢迎您对我的项目/博客提出任何建议和批评。
 - 如果您觉得本项目不错或者对您有帮助，请赏颗星吧！谢谢您的阅读！
