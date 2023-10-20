/*
 * @Author: Leo
 * @Date: 2023-10-10 10:25:51
 * @Description: 
 */
#include <gtest/gtest.h>
#include <sstream>
#include <yaml-cpp/yaml.h>

TEST(YamlTest, BasicParsing) {
    const std::string yaml = R"(
        name: John Smith
        age: 33
        occupation: Software Engineer
    )";

    YAML::Node node = YAML::Load(yaml);

    EXPECT_EQ(node["name"].as<std::string>(), "John Smith");
    EXPECT_EQ(node["age"].as<int>(), 33);
    EXPECT_EQ(node["occupation"].as<std::string>(), "Software Engineer");
}

TEST(YamlTest, NestedParsing) {
    const std::string yaml = R"(
        name: John Smith
        age: 33
        occupation:
            title: Software Engineer
            company: Acme Inc.
    )";

    YAML::Node node = YAML::Load(yaml);

    EXPECT_EQ(node["name"].as<std::string>(), "John Smith");
    EXPECT_EQ(node["age"].as<int>(), 33);
    EXPECT_EQ(node["occupation"]["title"].as<std::string>(), "Software Engineer");
    EXPECT_EQ(node["occupation"]["company"].as<std::string>(), "Acme Inc.");
}

TEST(YamlTest, SequenceParsing) {
    const std::string yaml = R"(
        - John Smith
        - Jane Doe
        - Bob Johnson
    )";

    YAML::Node node = YAML::Load(yaml);

    EXPECT_EQ(node[0].as<std::string>(), "John Smith");
    EXPECT_EQ(node[1].as<std::string>(), "Jane Doe");
    EXPECT_EQ(node[2].as<std::string>(), "Bob Johnson");
}


void yaml_test(){
    YAML::Node node;
    node["hi"] = "hello";
    node["name"] = "wzz";
    std::stringstream ss;
    ss << node;
    std::cout << ss.str();
}


int main(){
    // testing::InitGoogleTest();
    // return RUN_ALL_TESTS();

    yaml_test();
}