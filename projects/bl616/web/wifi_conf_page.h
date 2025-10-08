#ifndef __WIFI_CONF_PAGE_H
#define __WIFI_CONF_PAGE_H

/**
 * @brief
 * html压缩代码
 *
 * 在线压缩html网站
 * https://c.runoob.com/front-end/47/
 *
 * 配置页面html代码
 * 修改步骤：
 * 1.修改conf_page.html
 * 2.用浏览器打开conf_page.html预览效果直到效果满意为止
 * 3.把html文件中的"替换成\"
 * 4.打开html在线压缩网站压缩后替换define内容
 */
#define WIFI_CONF_HTML  "<!DOCTYPE html><html><head><title>WIFI配网</title><meta name=\"viewport\"content=\"width=device-width,inital-scale=1\"></head><style type=\"text/css\">.input{display:block;margin-top:10px}.input span{width:100px;float:left;float:left;height:36px}.input input{height:30px;width:200px}.btn{width:120px;height:35px;background-color:#16a085;border:0px;color:#ffffff;margin-top:150px;margin-left:100px;border-radius:10px;box-sizing:border-box}</style><body><form method=\"POST\"action=\"configwifi\"><label class=\"input\"><span>WiFi名称</span><input type=\"text\"name=\"ssid\" required></label><label class=\"input\"><span>WiFi密码</span><input type=\"password\"name=\"pass\"></label><input class=\"btn\"type=\"submit\"></form></body></html>"

 /**
  * loading效果加载页面
  */
#define HTML_LOADING    "<!DOCTYPE html><html lang=\"en\"class=\" -webkit-\"><head><meta http-equiv=\"Content-Type\"content=\"text/html; charset=UTF-8\"><title>配置中...</title><style>body{background:#fff}i{height:2em;width:2em;border-radius:100%;background:#e74c3c;display:block;margin:10em auto;position:absolute;left:50%;animation:spin 2s ease infinite}i:before,i:after{content:'';display:block;position:absolute;height:inherit;width:inherit;background:inherit;border-radius:inherit;animation:spin 2s ease infinite}i:before{left:-2.3em}i:after{left:2.3em}@keyframes spin{0%{top:0;transform:rotate(0deg)}50%{top:-4em;transform:rotate(-180deg)}100%{top:0;transform:rotate(-360deg)}}</style><body translate=\"no\"><i></i></body></html>"

#endif
