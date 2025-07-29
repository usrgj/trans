#include <fstream>
#include <iostream>
#include <unitree/idl/ros2/String_.hpp>
#include <unitree/robot/channel/channel_subscriber.hpp>
#include <unitree/robot/g1/audio/g1_audio_client.hpp>
#include <string>
#include "/home/gj/lib_cache/c++/json/json.hpp"

#define AUDIO_SUBSCRIBE_TOPIC "rt/audio_msg"
#define GROUP_IP "239.168.123.161"

using namespace std;
using json = nlohmann::json;

int sock;
string output = "", input = "";
json _data = json::parse(ifstream ("../json/keywords.json"));

bool myFind(const string& fa_str, const string& sub_str) {
  if (fa_str.find(sub_str) != string::npos) {return true;}
  return false;
}

bool findInArray(const string& fa_str, nlohmann::basic_json<>& sub_arr) {
  for (auto i : sub_arr) {
    if (fa_str.find(i) != string::npos) {return true;}
  }
  return false;
}

void productAnswer() {

  if (input == "结束录音。") {output = "结束录音";return;}

  for (auto i = _data.begin(); i != _data.end(); i++) {
    auto tmp = i.value()["trigger"]; //关键词组
    size_t size = tmp.size();

    for (auto j : tmp) {//遍历关键词
      bool match = false;
      if (j.is_string()) {match = myFind(input, j);}//是字符串
      else if (j.is_array()) {match = findInArray(input,j);}//是数组

      if (!match) {break;}//只要有一个关键词无法匹配,则退出这组关键词

      //遍历到最后一个关键词,且成功匹配,则生成输出
      if (j==tmp[size-1]) {output += i.value()["text"];}
    }

    if (!output.empty()) {break;}//该行保证只能匹配一个输出结果，否则遍历所有关键词组

  }
}

void asr_handler(const void *msg) {
  std_msgs::msg::dds_::String_ *resMsg = (std_msgs::msg::dds_::String_ *)msg;
  const std::string s = resMsg->data();
  int begin = s.find("text") + 7;
  int end = s.find("angle") - 3;

  if (begin < end){
    std::string text(s.begin() + begin, s.begin() + end);
    cout << text << endl;//debug

    if (!text.empty()) { //避免input被赋值为空
      input = text;
      productAnswer();//仅当有输入时才生成输出
    }
  }
}


int main(int argc, char const *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: audio_client_example [NetWorkInterface(eth0)]"
              << std::endl;
    exit(0);
  }

  //初始化客户端
  unitree::robot::ChannelFactory::Instance()->Init(0, argv[1]);
  unitree::robot::g1::AudioClient client;
  client.Init();
  client.SetTimeout(10.0f);

  /*ASR message Example*/
  unitree::robot::ChannelSubscriber<std_msgs::msg::dds_::String_> subscriber(
      AUDIO_SUBSCRIBE_TOPIC);
  subscriber.InitChannel(asr_handler);


  while (1) {
    sleep(1);  // wait for asr message
    cout << "waiting"<< endl;//debug
    cout << output << endl;//debug

    if (!output.empty()) {
      if (output == "结束录音"){break;}//当得到结束录音时，结束程序
      cout << "sleep5"<< endl;//debug
      client.TtsMaker(output,0);
      unitree::common::Sleep(5);
      output.clear();
    }
  }

  return 0;
}