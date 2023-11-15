/*
 * @Author: Leo
 * @Date: 2023-07-30 11:19:47
 * @Description:
 */


#ifndef HIPER_ENV_H
#define HIPER_ENV_H


#include "mutex.h"
#include "singleton.h"

#include <map>
#include <vector>

namespace hiper {

class Env {
public:
    typedef RWMutex RWMutexType;

    // 初始化，包括记录程序名称与路径，解析命令行选项和参数
    bool init(int argc, char** argv);

    void add(const std::string& key, const std::string& val);

    bool has(const std::string& key);

    void del(const std::string& key);

    std::string get(const std::string& key, const std::string& default_value = "");

    void addHelp(const std::string& key, const std::string& desc);

    void removeHelp(const std::string& key);

    void printHelp();

    /**
     * @brief 获取exe完整路径，从/proc/$pid/exe的软链接路径中获取，参考readlink(2)
     */
    const std::string& getExe() const { return exe_; }

    /**
     * @brief 获取当前路径，从main函数的argv[0]中获取，以/结尾
     * @return
     */
    const std::string& getCwd() const { return cwd_; }

    /**
     * @brief 设置系统环境变量，参考setenv(3)
     */
    bool setEnv(const std::string& key, const std::string& val);

    /**
     * @brief 获取系统环境变量，参考getenv(3)
     */
    std::string getEnv(const std::string& key, const std::string& default_value = "");

    /**
     * @brief 提供一个相对当前的路径path，返回这个path的绝对路径
     * @details 如果提供的path以/开头，那直接返回path即可，否则返回getCwd()+path
     */
    std::string getAbsolutePath(const std::string& path) const;

    /**
     * @brief 获取工作路径下path的绝对路径
     */
    std::string getAbsoluteWorkPath(const std::string& path) const;

    /**
     * @brief 获取配置文件路径，配置文件路径通过命令行-c选项指定，默认为当前目录下的conf文件夹
     */
    std::string getConfigPath();

private:
    // Mutex
    RWMutexType mutex_;
    // 存储程序的自定义环境变量
    std::map<std::string, std::string> args_;
    // 存储帮助选项与描述
    std::vector<std::pair<std::string, std::string>> helps_;
    // 程序名，也就是argv[0]
    std::string program_;
    // 程序完整路径名，也就是/proc/$pid/exe软链接指定的路径
    std::string exe_;
    // 当前路径，从argv[0]中获取
    std::string cwd_;
};

typedef hiper::Singleton<Env> EnvMgr;

}   // namespace hiper

#endif
