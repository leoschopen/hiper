#include "../hiper/base/hiper.h"

#include <boost/lexical_cast.hpp>
#include <string>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/node/type.h>
using namespace std;

// base cast T to F
template<class T, class F> class TypeCast {
public:
    F operator()(const T& v) { return boost::lexical_cast<F>(v); }
};

// partial spec cast vector<T> to yaml string
template<class T> class TypeCast<vector<T>, string> {
public:
    string operator()(const vector<T>& v)
    {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& i : v) {
            // node.push_back(YAML::Load(boost::lexical_cast<string>(i)));
            node.push_back(YAML::Load(TypeCast<T, string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T, class FromStr = TypeCast<string, T>, class ToStr = TypeCast<T, string>>
class ConfigVar {
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    ConfigVar(const T& default_val)
        : val_(default_val)
    {}

    string   toString() { return ToStr()(val_); }
    void     fromString(const string& str) { val_ = FromStr()(str); }
    const T& getValue() const { return val_; }

private:
    T val_;
};

int main() {
    vector<int> vec = { 1, 2, 3, 4, 5 };
    ConfigVar<vector<int>>::ptr vec_var(new ConfigVar<vector<int>>(vec));
    cout << vec_var->toString() << endl;
}