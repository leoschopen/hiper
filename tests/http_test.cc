/*
 * @Author: Leo
 * @Date: 2023-09-15 10:31:33
 * @Description: 
 */
#include "../hiper/base/hiper.h"
#include <iostream>
#include <string>

void test_str_method(){
    std::string method = "POST";
    hiper::http::HttpMethod m = hiper::http::StringToHttpMethod(method);
    std::cout << (int)m << std::endl;
    std::cout << hiper::http::HttpMethodToString(m) << std::endl;
}

void test_http_request() {
    hiper::http::HttpRequest req;
    req.setMethod(hiper::http::HttpMethod::GET);
    req.setVersion(0x11);
    req.setPath("/search");
    req.setQuery("q=url+%E5%8F%82%E6%95%B0%E6%9E%84%E9%80%A0&oq=url+%E5%8F%82%E6%95%B0%E6%9E%84%E9%80%A0+&aqs=chrome..69i57.8307j0j7&sourceid=chrome&ie=UTF-8");
    req.setHeader("Accept", "text/plain");
    req.setHeader("Content-Type", "application/x-www-form-urlencoded");
    req.setHeader("Cookie", "yummy_cookie=choco; tasty_cookie=strawberry");
    req.setBody("title=test&sub%5B1%5D=1&sub%5B2%5D=2"); // title=test&sub[1]=1&sub[2]=2

    req.dump(std::cout);

    std::cout << std::endl;
    std::cout << "===================================" <<std::endl;

    std::cout << std::endl;
    std::cout << "q " << req.getParam("q") << std::endl;
    std::cout << "title " << req.getParam("title") << std::endl;
    std::cout << "sub[1] " << req.getParam("sub[1]") << std::endl;
    std::cout << "sub[2] " << req.getParam("sub[2]") << std::endl;
    std::cout << "Accept " << req.getHeader("Accept") << std::endl;
    std::cout << "yummy_cookie " << req.getCookie("yummy_cookie") << std::endl;
    std::cout << "tasty_cookie " << req.getCookie("tasty_cookie") << std::endl;
    std::cout << std::endl;
}

void test_http_response() {
    hiper::http::HttpResponse rsp;
    rsp.setStatus(hiper::http::HttpStatus::OK);
    rsp.setHeader("Content-Type", "text/html");
    rsp.setBody("<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<title>hello world</title>"
                "</head>"
                "<body><p>hello world</p></body>"
                "</html>");
    rsp.setCookie("kookie1", "value1", 0, "/");
    rsp.setCookie("kookie2", "value2", 0, "/");
    rsp.dump(std::cout);
}


int main(){
    // 打印 http method
    // std::cout << int(hiper::http::HttpMethod::GET) << std::endl;
    // test_str_method();

    test_http_request();
    test_http_response();
    return 0;
}