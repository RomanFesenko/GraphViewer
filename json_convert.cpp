#include <algorithm>
#include <regex>
#include <tuple>

#include "json11.hpp"
#include "json_convert.h"

using std::string;
using std::vector;


const std::regex reg_identifier=std::regex("[_[:alpha:]][_[:alnum:]]*");

string FromJson(const string&str,CDataPool&pool)
{
  using namespace json11;
  auto fail=[&](const std::string&err)
  {
    assert(!err.empty());
    pool.Clear();
    return err;
  };
  auto get_const=[](const Json&json,string&err)->
  std::pair<string,float>
  {
      bool is_valid=json.is_object()&&
                    json.object_items().size()==1&&
                    json.object_items().begin()->second.is_number();
      if(!is_valid)
      {
          err="unexpected object in 'constants'";
          return {};
      }
      string name=json.object_items().begin()->first;
      if(!std::regex_match(name.begin(),name.end(),reg_identifier))
      {
          err=name+" is not valid identifier";
          return {};
      }
      float val=json.object_items().begin()->second.number_value();
      err.clear();
      return {name,val};
  };

  auto get_function=[](const Json&json,string&err)->
  std::tuple<string,vector<string>,string>
  {
      const Json::shape shape={{"name",Json::STRING},
                               {"args",Json::ARRAY},
                               {"body",Json::STRING}};
      if(!json.has_shape(shape,err))
      {
          err+=", in 'functions'";
          return {};
      }
      string name=json["name"].string_value();
      string body=json["body"].string_value();
      vector<string> args;
      for(const auto&arg:json["args"].array_items())
      {
          if(!arg.is_string())
          {
              err="'args' must be an array of strings";
              return {};
          }
          args.push_back(arg.string_value());
      }
      err.clear();
      return {name,args,body};
  };
  string err;
  pool.Clear();
  auto res=Json::parse(str,err);
  if(res.is_null()) return "Parse error:"+err;
  if(!res.is_object()) return "json must be an object";
  const auto&constants=res["constants"];
  if(constants.is_null()) return "'constants' array must be present";
  if(!constants.is_array()) return "'constants' must be array";
  const auto&functions=res["functions"];
  if(functions.is_null()) return "'functions' array must be present";
  if(!functions.is_array()) return "'functions' must be array";
  for(const auto&v:constants.array_items())
  {
      auto const_=get_const(v,err);
      if(!err.empty()) return fail(err);
      if(!pool.RegisterConstant(const_.first,const_.second))
        return fail("Name conflict with "+const_.first);
  }
  for(const auto&v:functions.array_items())
  {
      auto funcs=get_function(v,err);
      if(!err.empty()) return fail(err);
      auto res=pool.RegisterFunction(std::get<0>(funcs),
                                     std::get<1>(funcs),
                                     std::get<2>(funcs));
      if(!res) return fail("Parse function:"+res.error().detail());
  }
  return {};
}

void ToJson(std::string&str,const CDataPool&pool)
{
  using namespace json11;
  vector<Json> constants;
  const auto&cts=pool.Constants();
  for(std::size_t i=0;i<cts.Size();++i)
  {
      //auto j=Json(cts[i].m_value);
      auto obj=Json::object({{cts[i].name(),cts[i].m_value}});
      constants.push_back(obj);
  }
  vector<Json> functions;
  const auto&fns=pool.TopolocicalSortFuncs();
  for(int i=fns.size()-1;i>=0;--i)
  {
      Json::array args;
      for(const auto&arg:fns[i]->args()) args.push_back(arg);
      functions.push_back(Json::object({{"name",fns[i]->name()},
                                       {"args",args},
                                       {"body",fns[i]->body()}}));
  }
  str= Json(Json::object({{"constants",constants},
                          {"functions",functions}})).dump();
}
