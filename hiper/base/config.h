/*
 * @Author: Leo
 * @Date: 2023-07-30 10:55:23
 * @Description:
 */

#ifndef HIPER_CONFIG_H
#define HIPER_CONFIG_H


#include "log.h"
#include "mutex.h"
#include "util.h"

#include <boost/lexical_cast.hpp>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/yaml.h>

namespace hiper {


class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;

    ConfigVarBase(const std::string& name, const std::string& description = "")
        : name_(name)
        , description_(description)
    {
        std::transform(name_.begin(), name_.end(), name_.begin(), ::tolower);
    }

    virtual ~ConfigVarBase() {}

    const std::string& getName() const { return name_; }

    const std::string& getDescription() const { return description_; }

    virtual std::string toString() = 0;

    virtual bool fromString(const std::string& val) = 0;

    virtual std::string getTypeName() const = 0;

protected:
    /// 配置参数的名称
    std::string name_;
    /// 配置参数的描述
    std::string description_;
};


template<class F, class T> class LexicalCast {
public:
    T operator()(const F& v) { return boost::lexical_cast<T>(v); }
};

template<class T> class LexicalCast<std::string, std::vector<T>> {
public:
    std::vector<T> operator()(const std::string& v)
    {
        YAML::Node              node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream       ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str())); // 类对象函数调用
        }
        return vec;
    }
};

template<class T> class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T>& v)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T> class LexicalCast<std::string, std::list<T>> {
public:
    std::list<T> operator()(const std::string& v)
    {
        YAML::Node            node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream     ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T> class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T>& v)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


template<class T> class LexicalCast<std::string, std::set<T>> {
public:
    std::set<T> operator()(const std::string& v)
    {
        YAML::Node           node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream    ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

/**
 * @brief 类型转换模板类片特化(std::set<T> 转换成 YAML String)
 */
template<class T> class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T>& v)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


template<class T> class LexicalCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string& v)
    {
        YAML::Node                     node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream              ss;
        for (size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};


template<class T> class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T>& v)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


template<class T> class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string& v)
    {
        YAML::Node                        node = YAML::Load(v);
        typename std::map<std::string, T> vec;
        std::stringstream                 ss;
        for (auto it : node) {
            ss.str("");
            ss << it.second;
            vec.insert(std::make_pair(it.first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template<class T> class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T>& v)
    {
        YAML::Node node(YAML::NodeType::Map);
        for (auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T> class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string& v)
    {
        YAML::Node                                  node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream                           ss;
        for (auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};


template<class T> class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T>& v)
    {
        YAML::Node node(YAML::NodeType::Map);
        for (auto& i : v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief 配置参数模板子类,保存对应类型的参数值
 * @details T 参数的具体类型
 *          FromStr 从std::string转换成T类型的仿函数
 *          ToStr 从T转换成std::string的仿函数
 *          std::string 为YAML格式的字符串
 */
template<class T, class FromStr = LexicalCast<std::string, T>,
         class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
    typedef RWMutex                                                     RWMutexType;
    typedef std::shared_ptr<ConfigVar>                                  ptr;
    typedef std::function<void(const T& old_value, const T& new_value)> on_change_cb;

    ConfigVar(const std::string& name, const T& default_value, const std::string& description = "")
        : ConfigVarBase(name, description)
        , val_(default_value)
    {}


    // 将参数值转换成yaml style-String
    std::string toString() override
    {
        try {
            RWMutexType::ReadLock lock(mutex_);
            return ToStr()(val_);
        }
        catch (std::exception& e) {
            LOG_ERROR(LOG_ROOT()) << "ConfigVar::toString exception " << e.what()
                                  << " convert: " << TypeToName<T>() << " to string"
                                  << " name=" << name_;
        }
        return "";
    }

    // 将yaml style-String转换成参数值
    bool fromString(const std::string& val) override
    {
        try {
            setValue(FromStr()(val));
        }
        catch (std::exception& e) {
            LOG_ERROR(LOG_ROOT()) << "ConfigVar::fromString exception " << e.what()
                                  << " convert: string to " << TypeToName<T>() << " name=" << name_
                                  << " - " << val;
        }
        return false;
    }

    /**
     * @brief 获取当前参数的值
     */
    const T getValue()
    {
        RWMutexType::ReadLock lock(mutex_);
        return val_;
    }

    /**
     * @brief 设置当前参数的值
     * @details 如果参数的值有发生变化,则通知对应的注册回调函数
     */
    void setValue(const T& v)
    {
        {
            RWMutexType::ReadLock lock(mutex_);
            if (v == val_) {
                return;
            }
            for (auto& i : cbs_) {
                i.second(val_, v);
            }
        }
        RWMutexType::WriteLock lock(mutex_);
        val_ = v;
    }

    /**
     * @brief 返回参数值的类型名称(typeinfo)
     */
    std::string getTypeName() const override { return TypeToName<T>(); }

    uint64_t addListener(on_change_cb cb)
    {
        static uint64_t        s_fun_id = 0;
        RWMutexType::WriteLock lock(mutex_);
        ++s_fun_id;
        cbs_[s_fun_id] = cb;
        return s_fun_id;
    }

    void delListener(uint64_t key)
    {
        RWMutexType::WriteLock lock(mutex_);
        cbs_.erase(key);
    }

    on_change_cb getListener(uint64_t key)
    {
        RWMutexType::ReadLock lock(mutex_);
        auto                  it = cbs_.find(key);
        return it == cbs_.end() ? nullptr : it->second;
    }

    /**
     * @brief 清理所有的回调函数
     */
    void clearListener()
    {
        RWMutexType::WriteLock lock(mutex_);
        cbs_.clear();
    }

private:
    RWMutexType mutex_;
    T           val_;
    // 变更回调函数组, uint64_t key,要求唯一，一般可以用hash
    std::map<uint64_t, on_change_cb> cbs_;
};


class Config {
public:
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    typedef RWMutex RWMutexType;

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value,
                                             const std::string& description = "")
    {
        RWMutexType::WriteLock lock(GetMutex());

        auto it = GetDatas().find(name);
        if (it != GetDatas().end()) {
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
            if (tmp) {
                LOG_INFO(LOG_ROOT()) << "Lookup name=" << name << " exists";
                return tmp;
            }
            else {
                LOG_ERROR(LOG_ROOT())
                    << "Lookup name=" << name << " exists but type not " << TypeToName<T>()
                    << " real_type=" << it->second->getTypeName() << " " << it->second->toString();
                return nullptr;
            }
        }

        if (name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") != std::string::npos) {
            LOG_ERROR(LOG_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }

        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
        GetDatas()[name] = v;
        return v;
    }

    template<class T> static typename ConfigVar<T>::ptr Lookup(const std::string& name)
    {
        RWMutexType::ReadLock lock(GetMutex());

        auto it = GetDatas().find(name);
        if (it == GetDatas().end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    static void LoadFromYaml(const YAML::Node& root);

    static void LoadFromConfDir(const std::string& path, bool force = false);

    static ConfigVarBase::ptr LookupBase(const std::string& name);

    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

    static void printdata(){
        auto dataaa = GetDatas();
        for(auto& i : dataaa){
            std::cout << i.first << " - " << i.second->toString() << std::endl;
        }
    }

private:
    static ConfigVarMap& GetDatas()
    {
        static ConfigVarMap s_datas;
        return s_datas;
    }


    static RWMutexType& GetMutex()
    {
        static RWMutexType s_mutex;
        return s_mutex;
    }
};

}   // namespace hiper

#endif
